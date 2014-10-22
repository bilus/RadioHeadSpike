// nrf24_reliable_datagram_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RHReliableDatagram class, using the RH_NRF24 driver to control a NRF24 radio.
// It is designed to work with the other example nrf24_reliable_datagram_server
// Tested on Uno with Sparkfun WRL-00691 NRF24L01 module
// Tested on Teensy with Sparkfun WRL-00691 NRF24L01 module
// Tested on Anarduino Mini (http://www.anarduino.com/mini/) with RFM73 module
// Tested on Arduino Mega with Sparkfun WRL-00691 NRF25L01 module

#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>
#include <limits.h>

#include "timer.h"
#include "message.h"

#define CLIENT_ADDRESS 5
#define SERVER_ADDRESS 2

// Singleton instance of the radio driver
RH_NRF24 driver(9);
// RH_NRF24 driver(8, 7);   // For RFM73 on Anarduino Mini

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

// DO NOT put it on the stack!
Message message;

unsigned long numTotal = 0;            // Total number of send attempts.
unsigned long numSuccess = 0;          // Number of successful sendToWait calls.
unsigned long numReply = 0;            // Number of successful recvfromAckTimeout calls.


unsigned long minPingTime = ULONG_MAX; // Minimum ping time in ms.
unsigned long maxPingTime = 0;         // Maximum ping time in ms.
unsigned long totalPingTime = 0;       // Total ping time in ms; used to calculate the average ping time.

void resetStats()
{
  numTotal = 0;
  numSuccess = 0;
  numReply = 0;
  minPingTime = ULONG_MAX;
  maxPingTime = 0;
  totalPingTime = 0;
}

void updatePingTimes(unsigned long pingTime)
{
  totalPingTime += pingTime;
  if (minPingTime > pingTime) minPingTime = pingTime;
  if (maxPingTime < pingTime) maxPingTime = pingTime;
}

unsigned long getAvgPingTime()         // Returns the average ping time or ULONG_MAX if no ping info yet.
{
  if (numReply > 0)
  {
    return totalPingTime / numReply;
  }
  else
  {
    return ULONG_MAX;
  }
}




void printStats()
{
  const unsigned long start = Timer.elapsed();

  Serial.println("------------------------------------------------------------------------------==");
  Serial.print("Total:     ");
  Serial.print(numTotal);
  Serial.print(" ");
  Serial.print("Successes:     ");
  Serial.print(numSuccess);
  Serial.print(" ");
  Serial.print("Replies:     ");
  Serial.print(numReply);
  Serial.println("");

  const float timeSec = float(start) / 1000;
  Serial.print("Total time: ");
  Serial.print(timeSec);
  Serial.println("s");

  Serial.print("Total/sec: ");
  Serial.print(numTotal / timeSec);
  Serial.print(" ");
  Serial.print("Successes/sec: ");
  Serial.print(numSuccess / timeSec);
  Serial.print(" ");
  Serial.print("Replies/sec: ");
  Serial.print(numReply / timeSec);
  Serial.println("");
  
  Serial.print("Avg ping: ");
  Serial.print(getAvgPingTime());
  Serial.print("ms ");
  Serial.print("Min ping: ");
  Serial.print(minPingTime);
  Serial.print("ms ");
  Serial.print("Max ping: ");
  Serial.print(maxPingTime);
  Serial.println("ms");

  Serial.print("(printed in ");
  Serial.print(Timer.elapsed() - start);
  Serial.println("ms)");
}

void restart(const unsigned long channel)
{
  printStats();
  Serial.println("================================================================================");
  Serial.println("Restarting.");
  
  const unsigned long initStart = millis();
  manager.init();
  driver.setChannel(channel);
  const unsigned long initEnd = millis();
  
  Serial.print("(Re-init in ");
  Serial.print(initEnd - initStart);
  Serial.println("ms.)");
  
  delay(1000);  // Let the server reinit other arduinos.
  
  Timer.restart();
  resetStats();
  Serial.print("Restarted with ");
  Serial.print("channel = ");
  Serial.println(channel);
}

////////////////////////////////////////////////////////////////////////////////

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
//  driver.setChannel(2);
  Timer.start();
}

////////////////////////////////////////////////////////////////////////////////

void loop()
{
  ++numTotal;
  
  message.type = Message::PING;
  message.data.pingTime = millis();
  
  // Send a message to manager_server
  if (manager.sendtoWait((byte *)&message, sizeof(message), SERVER_ADDRESS))
  {
    ++numSuccess;
    // Now wait for a reply from the server
    uint8_t len = sizeof(message);
    uint8_t from;   
    if (manager.recvfromAckTimeout((byte *) &message, &len, 2000, &from))
    {
      if (len != sizeof(message))
      {
        Serial.println("Error: unexpected length of the received message.");
      }
      else
      {
        ++numReply;

        switch (message.type)
        {
          case Message::PONG:
            {     
              const unsigned long currentT = millis();
              TimerClass::Pause pause;
              updatePingTimes(currentT - message.data.pongTime);
            }
            break;

          case Message::RESTART:
            restart(message.data.restartParams.channel);
            break;
            
          default:
            Serial.print("Error: unexpected type of the received message (");
            Serial.print(message.type);
            Serial.println(").");
        }
      }
    }
    else
    {
      Serial.println("Error: no reply, is nrf24_reliable_datagram_server running?");
    }
  }
  else
  {
    Serial.println("Error: sendtoWait failed");
  }
  
  delay(10);
}

