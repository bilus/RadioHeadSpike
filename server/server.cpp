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

#define SERVER_ADDRESS 1

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

//
// void scenario0(Message& m)
// {
//   // The radio is configured by default to Channel 2, 2Mbps, 0dBm power.
//   // This scenario is implicitly run at the beginning.
//   m.data.restartParams.channel = 2;
//   m.data.restartParams.dataRate = RH_NRF24::DataRate2Mbps;
//   m.data.restartParams.power = RH_NRF24::TransmitPower0dBm;
// }
// void scenario1(Message& m)
// {
//   m.data.restartParams.channel = 2;
//   m.data.restartParams.dataRate = RH_NRF24::DataRate250kbps;
//   m.data.restartParams.power = RH_NRF24::TransmitPower0dBm;
// }
// void scenario2(Message& m)
// {
//   m.data.restartParams.channel = 2;
//   m.data.restartParams.dataRate = RH_NRF24::DataRate2Mbps;
//   m.data.restartParams.power = RH_NRF24::TransmitPowerm18dBm;
// }
// void scenario3(Message& m)
// {
//   m.data.restartParams.channel = 2;
//   m.data.restartParams.dataRate = RH_NRF24::DataRate2Mbps;
//   m.data.restartParams.power = RH_NRF24::TransmitPower0dBm;
//   // m.data.restartParams.channel = 90;
//   // m.data.restartParams.dataRate = RH_NRF24::DataRate250kbps;
//   // m.data.restartParams.power = RH_NRF24::TransmitPower0dBm;
// }
//
// unsigned short currentScenario = 0;
// void (*scenarios[4]) (Message&) = {&scenario0, &scenario1, &scenario2, &scenario3};


//
//
// void sendRestartMessage(uint8_t to)
// {
//   message.type = Message::RESTART;
//   (*scenarios[currentScenario])(message);
//
//   if (!manager.sendtoWait((byte *) &message, sizeof(message), to))  // FIXME: Extract into a method/function operating on a Message. Duplication below.
//   {
//     Serial.println("sendtoWait failed");
//   }
// }
//
// void init()
// {
//   Message m;
//   (*scenarios[currentScenario])(m);
//   // FIXME: Duplication in client.cpp
//   manager.init();
//   driver.setChannel(m.data.restartParams.channel);
//   driver.setRF((RH_NRF24::DataRate)m.data.restartParams.dataRate, (RH_NRF24::TransmitPower) m.data.restartParams.power);
// }
//
// void handleEchoState(uint8_t from)
// {
//   if (RESTART_AFTER == numSinceRestart)
//   {
// #if (MODE == MODE_SCAN_CHANNELS)
//     message.type = Message::RESTART;
//     channel = (channel % MAX_CHANNEL) + 1;
//     message.data.restartParams.channel = channel;
//     if (!manager.sendtoWait((byte *) &message, sizeof(message), from))  // FIXME: Extract into a method/function operating on a Message. Duplication below.
//     {
//       Serial.println("sendtoWait failed");
//     }
//     manager.init();
//     driver.setChannel(channel);
// #else
//     currentScenario = ((currentScenario + 1) % sizeof(scenarios));
//     Serial.print("Scenario ");
//     Serial.println(currentScenario);
//
//     sendRestartMessage(from);
//
//     init();
// #endif
//
//     numSinceRestart = 0;
//   }
//   else
//   {
//     switch (message.type)
//     {
//       case Message::PING:
//         message.type = Message::PONG;
//         // Leave data intact; PING is compatible with PONG.
//         break;
//       default:
//         Serial.println("Error: unexpected type of the received message.");
//         message.type = Message::ERROR;
//     }
//
//     // Send a reply back to the originator client
//     if (!manager.sendtoWait((byte *) &message, sizeof(message), from))
//     {
//       Serial.println("sendtoWait failed");
//     }
//   }
//
// }

////////////////////////////////////////////////////////////////////////////////

unsigned long thePairingStart;
const unsigned long PAIRING_PERIOD = 10 * 1000; // TODO: 30 s

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
  thePairingStart = millis();
  theState = PAIRING;
  thePairedDeviceCount = 0;
}

void startWorking()
{
  Serial.print("Asking paired devices to start working (");
  Serial.print(thePairedDeviceCount);
  Serial.println(")");
  
  for (int i = 0; i < thePairedDeviceCount; ++i)
  {
    theMessage.type = Message::WORK;
    theMessage.sendThrough(manager, thePairedDevices[i]); // TODO: What if it fails due to a temporary eror?
    // No reply is expected.
  }
  theState = WORKING;
}

void onPairing()
{
  if (millis() - thePairingStart > PAIRING_PERIOD)
  {
    startWorking();
    return;
  }
  
  maybePrintStatus("Pairing...");
  
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


void onWorking()
{
  maybePrintStatus("Working...");
  
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

////////////////////////////////////////////////////////////////////////////////

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");
  
  startPairing();
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  //driver.setChannel(90);
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
    default:
      Serial.print("Error: invalid state (");
      Serial.print(theState);
      Serial.println(")");
      startPairing();
  }
  
  delay(10);
}

