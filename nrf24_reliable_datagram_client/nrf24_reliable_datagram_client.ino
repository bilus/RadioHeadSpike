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

#define CLIENT_ADDRESS 5
#define SERVER_ADDRESS 2

// Singleton instance of the radio driver
RH_NRF24 driver(9);
// RH_NRF24 driver(8, 7);   // For RFM73 on Anarduino Mini

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);


void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
//  driver.setChannel(2);
  Timer.start();
}

uint8_t data[] = "Hello Worl2!";
// Dont put this on the stack:
uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];

unsigned long numTotal = 0;            // Total number of send attempts.
unsigned long numSuccess = 0;          // Number of successful sendToWait calls.
unsigned long numReply = 0;            // Number of successful recvfromAckTimeout calls.


unsigned long minPingTime = ULONG_MAX; // Minimum ping time in ms.
unsigned long maxPingTime = 0;         // Maximum ping time in ms.
unsigned long totalPingTime = 0;       // Total ping time in ms; used to calculate the average ping time.

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

  
  Serial.println("================================================================================");
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

  Serial.print("(in ");
  Serial.print(Timer.elapsed() - start);
  Serial.println("ms)");
}

const int PRINT_STATS_EVERY = 100;

void loop()
{
  //Serial.println("Sending to nrf24_reliable_datagram_server");
  ++numTotal;
  
  const unsigned long sentT = millis();
  
  // Send a message to manager_server
  if (manager.sendtoWait((byte *)&sentT, sizeof(sentT), SERVER_ADDRESS))
  // if (manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS))
  {
    ++numSuccess;
    // Now wait for a reply from the server
    uint8_t len = sizeof(buf);
    uint8_t from;   
    if (manager.recvfromAckTimeout(buf, &len, 2000, &from))
    {
      ++numReply;
      
      const unsigned long receivedT = *(unsigned long *) buf;
      const unsigned long currentT = millis();
      // Serial.print("got reply from : 0x");
      // Serial.print(from, HEX);
      // Serial.print(": ");
      // Serial.println(currentT - receivedT);
      updatePingTimes(currentT - receivedT);
      // Serial.println((char*)buf);
    }
    else
    {
      // Serial.println("No reply, is nrf24_reliable_datagram_server running?");
    }
  }
  else
  {
    // Serial.println("sendtoWait failed");
  }
  
  delay(10);

  {
    TimerClass::Pause pause;
    
    if (0 == (numTotal % PRINT_STATS_EVERY))
    {
      printStats();
    }
  }
}

