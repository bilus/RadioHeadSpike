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


#define SERVER_ADDRESS 2

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
unsigned long RESTART_AFTER = 10;

byte channel = 2;
const byte MAX_CHANNEL = 126; 

Message message; // Don't put this on the stack.

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
          channel = (channel % MAX_CHANNEL) + 1;
          message.data.restartParams.channel = channel;
          if (!manager.sendtoWait((byte *) &message, sizeof(message), from))  // FIXME: Extract into a method/function operating on a Message. Duplication below.
          {
            Serial.println("sendtoWait failed");
          }
          manager.init();
          driver.setChannel(channel);
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

