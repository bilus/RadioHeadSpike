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

#define MODE_SCAN_CHANNELS 1
#define MODE_SCENARIOS 2
#define MODE MODE_SCENARIOS

#define SERVER_ADDRESS 1

// Singleton instance of the radio driver
RH_NRF24 driver(9);
// RH_NRF24 driver(8, 7);   // For RFM73 on Anarduino Mini

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, SERVER_ADDRESS);

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  //driver.setChannel(90);
}

unsigned long numSinceRestart = 0;
unsigned long RESTART_AFTER = 100;

#if (MODE == MODE_SCAN_CHANNELS)

byte channel = 2;
const byte MAX_CHANNEL = 126; 
#else

#endif

Message message; // Don't put this on the stack.

void scenario0(Message& m)
{
  // The radio is configured by default to Channel 2, 2Mbps, 0dBm power.
  // This scenario is implicitly run at the beginning.
  m.data.restartParams.channel = 2;
  m.data.restartParams.dataRate = RH_NRF24::DataRate2Mbps;
  m.data.restartParams.power = RH_NRF24::TransmitPower0dBm;  
}
void scenario1(Message& m)
{
  m.data.restartParams.channel = 2;
  m.data.restartParams.dataRate = RH_NRF24::DataRate250kbps;
  m.data.restartParams.power = RH_NRF24::TransmitPower0dBm;
}
void scenario2(Message& m)
{
  m.data.restartParams.channel = 2;
  m.data.restartParams.dataRate = RH_NRF24::DataRate2Mbps;
  m.data.restartParams.power = RH_NRF24::TransmitPowerm18dBm;
}
void scenario3(Message& m)
{
  m.data.restartParams.channel = 2;
  m.data.restartParams.dataRate = RH_NRF24::DataRate2Mbps;
  m.data.restartParams.power = RH_NRF24::TransmitPower0dBm;  
  // m.data.restartParams.channel = 90;
  // m.data.restartParams.dataRate = RH_NRF24::DataRate250kbps;
  // m.data.restartParams.power = RH_NRF24::TransmitPower0dBm;
}

unsigned short currentScenario = 0;
void (*scenarios[4]) (Message&) = {&scenario0, &scenario1, &scenario2, &scenario3};

void loop()
{ 
  if (manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(message);
    uint8_t from;
    if (manager.recvfromAck((byte *) &message, &len, &from))
    {
      if (len != sizeof(message))
      {
        Serial.println("Error: unexpected length of the received message.");
      }
      else
      {
        // FIXME: If you comment out the following lines, it'll considerably slow down. Why?
        Serial.print("got request from : 0x");
        Serial.print(from, HEX);
        Serial.print(": ");
        Serial.println(message.type);
        
        ++numSinceRestart;
        
        if (RESTART_AFTER == numSinceRestart)
        {      
          message.type = Message::RESTART;
#if (MODE == MODE_SCAN_CHANNELS)
          channel = (channel % MAX_CHANNEL) + 1;
          message.data.restartParams.channel = channel;
          if (!manager.sendtoWait((byte *) &message, sizeof(message), from))  // FIXME: Extract into a method/function operating on a Message. Duplication below.
          {
            Serial.println("sendtoWait failed");
          }
          manager.init();
          driver.setChannel(channel);
#else
          currentScenario = ((currentScenario + 1) % sizeof(scenarios));
          Serial.print("Scenario ");
          Serial.println(currentScenario);
          (*scenarios[currentScenario])(message);
          
          if (!manager.sendtoWait((byte *) &message, sizeof(message), from))  // FIXME: Extract into a method/function operating on a Message. Duplication below.
          {
            Serial.println("sendtoWait failed");
          }
          
          // FIXME: Duplication in client.cpp
          manager.init();
          driver.setChannel(message.data.restartParams.channel);
          driver.setRF((RH_NRF24::DataRate)message.data.restartParams.dataRate, (RH_NRF24::TransmitPower) message.data.restartParams.power);
#endif
          
          numSinceRestart = 0;
        }
        else
        {
          switch (message.type)
          {
            case Message::PING:
              message.type = Message::PONG;
              // Leave data intact; PING is compatible with PONG.
              break;
            default:
              Serial.println("Error: unexpected type of the received message.");  
              message.type = Message::ERROR;        
          }

          // Send a reply back to the originator client
          if (!manager.sendtoWait((byte *) &message, sizeof(message), from))
          {
            Serial.println("sendtoWait failed");
          }
        }
      }
    }
  }
}

