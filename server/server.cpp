// nrf24_reliable_datagram_server.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging server
// with the RHReliableDatagram class, using the RH_NRF24 driver to control a NRF24 radio.
// It is designed to work with the other example nrf24_reliable_datagram_client
// Tested on Uno with Sparkfun WRL-00691 NRF24L01 module
// Tested on Teensy with Sparkfun WRL-00691 NRF24L01 module
// Tested on Anarduino Mini (http://www.anarduino.com/mini/) with RFM73 module
// Tested on Arduino Mega with Sparkfun WRL-00691 NRF25L01 module

#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>

#include "message.h"
#include "helpers.h"
#include "scenarios.h"

#define SERVER_ADDRESS 1 // Needs to match client.cpp

// Singleton instance of the radio driver
RH_NRF24 driver(9);
// RH_NRF24 driver(8, 7);   // For RFM73 on Anarduino Mini

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, SERVER_ADDRESS);

// unsigned long numSinceRestart = 0;
// unsigned long RESTART_AFTER = 100;
//
// #if (MODE == MODE_SCAN_CHANNELS)
//
// byte channel = 2;
// const byte MAX_CHANNEL = 126;
// #else
//
// #endif
//
Message theMessage; // Don't put this on the stack.

#define RECEIVE_TIMEOUT 2000

enum State
{
  PAIRING,
  TUNING,
  WORKING
};

State theState;

////////////////////////////////////////////////////////////////////////////////

// How long to wait for devices to pair before switching to WORKING state.
const unsigned long PAIRING_PERIOD = 10 * 1000; 
unsigned long thePairingStartedAt;

// How long to wait before switching to TUNING state to change the transmission parameters.
const unsigned long TUNE_AFTER = 10 * 1000;     
unsigned long theWorkingStartAt;

// Milliseconds to wait for devices to tune in before switching to PAIRING state.
const unsigned long TUNE_FOR = 10 * 1000;
unsigned long theTuningStartAt;

////////////////////////////////////////////////////////////////////////////////

#define MAX_PAIRED_DEVICES 64
Message::Address thePairedDevices[MAX_PAIRED_DEVICES];
unsigned short thePairedDeviceCount = 0;

bool addPairedDevice(uint8_t& address)
{
  for (int i = 0; i < thePairedDeviceCount; ++i)
  {
    if (thePairedDevices[i] == address)
      return false;
  }
  
  thePairedDevices[thePairedDeviceCount++] = address;
  return true;
}

////////////////////////////////////////////////////////////////////////////////

void startPairing()
{
  thePairedDeviceCount = 0;
  theState = PAIRING;
  thePairingStartedAt = millis();
}

void startWorking()
{
  Serial.print("Asking paired devices to start working (");
  Serial.print(thePairedDeviceCount);
  Serial.println(")");

  // The code below sends to each individual device. The current one uses broadcast.
  theMessage.type = Message::WORK;
  theMessage.sendThrough(manager, RH_BROADCAST_ADDRESS);
  
  // theMessage.type = Message::WORK;
  // for (int i = 0; i < thePairedDeviceCount; ++i)
  // {
  //   theMessage.sendThrough(manager, thePairedDevices[i]); // TODO: What if it fails due to a temporary eror?
  //   // No reply is expected.
  // }
  theState = WORKING;
  theWorkingStartAt = millis();
}

void startTuning()
{
  theState = TUNING;
  theTuningStartAt = millis();
}

// Handle PAIRING state.
void onPairing()
{
  maybePrintStatus("Pairing...");

  // Wait for WELCOME messages from devices and pair with each new one.
  // After PAIRING_PERIOD, switch to WORKING state.
  
  if (millis() - thePairingStartedAt > PAIRING_PERIOD)
  {
    startWorking();
    return;
  }  
  
  Message::Address from;
  if (theMessage.receiveThrough(manager, RECEIVE_TIMEOUT, &from))
  {
    if (Message::HELLO == theMessage.type)
    {
      theMessage.type = Message::WELCOME;
      // TODO: Generate an id.
      if (theMessage.sendThrough(manager, from))
      {
        if (addPairedDevice(from))
        {
          Serial.print("A new device detected (");
          Serial.print(from);
          Serial.println(").");
        }        
      }
      else
      {
        Serial.println("Error: Sending WELCOME failed.");
      }
    }
    else
    {
      Serial.println("Error: Expecting HELLO.");
      theMessage.type = Message::ERROR;
      theMessage.sendThrough(manager, from);
    }
  }
}

// Handle WORKING state.
void onWorking()
{
  maybePrintStatus("Working...");
  
  // Reply to PING messages with PONG and with ERROR message to anything else.
  // After TUNE_AFTER period, switch to TUNING state.
  
  if (millis() - theWorkingStartAt > TUNE_AFTER)
  {
    startTuning();
    return;
  }
  
  if (manager.available())
  {
    Message::Address from;
    if (theMessage.receiveThrough(manager, &from))
    {
      if (Message::PING == theMessage.type)
      {
        theMessage.type = Message::PONG;
        // The rest stays the same.
        theMessage.sendThrough(manager, from);
        Serial.println("PING-PONG!");
      }
      else
      {
        theMessage.type = Message::ERROR;
        theMessage.sendThrough(manager, from);
      }
    }  
  }
}

// Handle TUNING state.
void onTuning()
{
  maybePrintStatus("Tuning...");

  // Whenever any client sends us anything, reply with TUNE message, asking the device to tune in.
  // After TUNE_FOR period, switch to PAIRING state. Each client who received TUNE message
  // should already be in PAIRING state.
  
  // Important: This approach works for the echo server; it requires a constant flow of messages
  // from clients to work. With infrequent messages (e.g. button presses), some clients may never 
  // receive the TUNE message and lose connection with the server (e.g. if the sever changes the 
  // channel and they stay on the old one).
  
  if (millis() - theTuningStartAt > TUNE_FOR)
  {
    applyCurrentScenario(driver, manager);
    nextScenario();
    startPairing();
    return;
  }

  if (manager.available())
  {
    Message::Address from;
    if (theMessage.receiveThrough(manager, &from))
    {
      theMessage.type = Message::TUNE;
      applyCurrentScenario(theMessage);
      theMessage.sendThrough(manager, from);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");

  applyCurrentScenario(driver, manager);
  
  startPairing();
}

////////////////////////////////////////////////////////////////////////////////

void loop()
{ 
  switch (theState)
  {
    case PAIRING:
      onPairing();
      break;
    case WORKING:
      onWorking();
      break;
    case TUNING:
      onTuning();
      break;
    default:
      Serial.print("Error: invalid state (");
      Serial.print(theState);
      Serial.println(")");
      startPairing();
  }
  
  delay(10);
}

