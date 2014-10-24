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
  WORKING,
  REPORTING
};

State theState;

////////////////////////////////////////////////////////////////////////////////

// How long to wait for devices to pair before switching to WORKING state.
const unsigned long PAIRING_PERIOD = 10 * 1000; 
unsigned long thePairingStartedAt;

// How long to wait before switching to TUNING state to change the transmission parameters.
const unsigned long WORK_PERIOD = 10L * 1000L;     
unsigned long theWorkingStartAt;

// Milliseconds to wait for devices to tune in before switching to PAIRING state.
const unsigned long TUNE_FOR = 10L * 1000L;
unsigned long theTuningStartAt;

// Maximum time to wait for reports from clients.
const unsigned long MAX_REPORTING_TIME = 30L * 1000L;
unsigned long theReportingStartAt;

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

void broadcast(const Message::Type& type)
{
  // The commented out code below sends to each individual device. 
  // The current one uses broadcasts repeated for approx. 1s.
  
  theMessage.type = type;
  theMessage.repeatedlyBroadcast(manager, 1000);  

  // theMessage.type = Message::WORK;
  // for (int i = 0; i < thePairedDeviceCount; ++i)
  // {
  //   theMessage.sendThrough(manager, thePairedDevices[i]); // TODO: What if it fails due to a temporary eror?
  //   // No reply is expected.
  // }
}

////////////////////////////////////////////////////////////////////////////////

void startPairing()
{
  thePairedDeviceCount = 0;
  theState = PAIRING;
  thePairingStartedAt = millis();
  printStatus("Pairing...");
}

void startWorking()
{
  Serial.print("Asking paired devices to start working (");
  Serial.print(thePairedDeviceCount);
  Serial.println(")");

  broadcast(Message::WORK);

  theState = WORKING;
  theWorkingStartAt = millis();
  printStatus("Working...");
}

void startReporting()
{
  theState = REPORTING;
  theReportingStartAt = millis();
  printStatus("Reporting...");
}

void startTuning()
{
  theState = TUNING;
  theTuningStartAt = millis();
  printStatus("Tuning...");
}

// Handle PAIRING state.
void onPairing()
{
  // Wait for WELCOME messages from devices and pair with each new one.
  // After PAIRING_PERIOD, switch to WORKING state.
  
  if (millis() - thePairingStartedAt > PAIRING_PERIOD)
  {
    startWorking();
    return;
  }  

  if (manager.available())
  {
    Message::Address from;
    if (theMessage.receiveThrough(manager, &from))
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

  maybePrintStatus("Pairing...");
}

// Handle WORKING state.
void onWorking()
{  
  // Reply to PING messages with PONG and with ERROR message to anything else.
  // After WORK_PERIOD period, switch to REPORTING state.
  
  if (millis() - theWorkingStartAt > WORK_PERIOD)
  {
    startReporting();
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
        // Serial.println("PING-PONG!");
      }
      else
      {
        theMessage.type = Message::ERROR;
        theMessage.sendThrough(manager, from);
      }
    }  
  }

  maybePrintStatus("Working...");
}

void handleReport(const Message::Address& from, const Message::Data::Report& report)
{
  Message::Data::TuningParams tuningParams;
  applyCurrentScenario(tuningParams);
  Serial.print(tuningParams.channel);
  Serial.print(",");
  Serial.print(tuningParams.dataRate);
  Serial.print(",");
  Serial.print(tuningParams.power);
  Serial.print(",");

  Serial.print(from);
  Serial.print(",");
  Serial.print(report.numTotal);
  Serial.print(",");
  Serial.print(report.numSuccess);
  Serial.print(",");
  Serial.print(report.numReply);
  Serial.print(",");

  Serial.print(report.avgPingTime);
  Serial.print(",");
  Serial.print(report.minPingTime);
  Serial.print(",");
  Serial.print(report.maxPingTime);
  
  Serial.println();
}

// Handle REPORTING state.
void onReporting()
{  
  // Reply to PING messages with PONG and with ERROR message to anything else.
  // After MAX_REPORTING_TIME period, switch to TUNING state.
  
  // TODO: Switch as soon as all paired devices send in their reports.
  if (millis() - theReportingStartAt > MAX_REPORTING_TIME) 
  {
    broadcast(Message::WORK);
    startTuning();
    return;
  }
  
  if (manager.available())
  {
    Message::Address from;
    if (theMessage.receiveThrough(manager, &from))
    {
      if (Message::REPORT == theMessage.type)
      {
        handleReport(from, theMessage.data.report);
        theMessage.type = Message::OK;
        theMessage.sendThrough(manager, from);
      }
      else
      {
        theMessage.type = Message::QUERY;
        theMessage.sendThrough(manager, from);
      }
    }
  }

  maybePrintStatus("Reporting...");
}

// Handle TUNING state.
void onTuning()
{
  // Whenever any client sends us anything, reply with TUNE message, asking the device to tune in.
  // After TUNE_FOR period, switch to PAIRING state. Each client who received TUNE message
  // should already be in PAIRING state.
  
  // Important: This approach works for the echo server; it requires a constant flow of messages
  // from clients to work. With infrequent messages (e.g. button presses), some clients may never 
  // receive the TUNE message and lose connection with the server (e.g. if the sever changes the 
  // channel and they stay on the old one).
  
  const unsigned long now = millis();
  
  // Serial.print("now = ");
  // Serial.println(now);
  // Serial.print("theTuningStartAt = ");
  // Serial.println(theTuningStartAt);
  // Serial.print("TUNE_FOR = ");
  // Serial.println(TUNE_FOR);
  
  if (now - theTuningStartAt > TUNE_FOR)
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
      applyCurrentScenario(theMessage.data.tuningParams);
      theMessage.sendThrough(manager, from);
    }
  }

  maybePrintStatus("Tuning...");
}

////////////////////////////////////////////////////////////////////////////////

void setup() 
{
  Serial.begin(9600);
  Serial.print("Started server ");
  Serial.print(SERVER_ADDRESS);
  Serial.println(". Welcome!");

  if (manager.init())
  {
    applyCurrentScenario(driver, manager);
    startPairing();
  }
  else
  {
    Serial.println("init failed");
  }
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
    case REPORTING:
      onReporting();
      break;
    default:
      Serial.print("Error: invalid state (");
      Serial.print(theState);
      Serial.println(")");
      startPairing();
  }
  
  delay(10);
}

