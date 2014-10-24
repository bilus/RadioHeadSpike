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
#include "helpers.h"
#include "tuning.h"

// CLIENT_ADDRESS is #defined externally by the ./deploy script and its dependencies.

#define SERVER_ADDRESS 1 // Needs to match server.cpp

// Singleton instance of the radio driver
RH_NRF24 driver(9);
// RH_NRF24 driver(8, 7);   // For RFM73 on Anarduino Mini

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

// DO NOT put it on the stack!
Message theMessage;

#define RECEIVE_TIMEOUT 2000

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

  Serial.println("==========================================================================");
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

void serializeStats(Message::Data::Report& report)
{
  report.numTotal = numTotal;
  report.numSuccess = numSuccess;
  report.numReply = numReply;
  report.avgPingTime = getAvgPingTime();
  report.minPingTime = minPingTime;
  report.maxPingTime = maxPingTime;
}

////////////////////////////////////////////////////////////////////////////////

enum State
{
  PAIRING,
  WAITING,
  WORKING,
  REPORTING
};

State theState;

////////////////////////////////////////////////////////////////////////////////

void startPairing()
{
  theState = PAIRING;
  printStatus("Pairing...");
}

void startWaiting()
{
  theState = WAITING;
  printStatus("Waiting...");
}

void startWorking()
{
  theState = WORKING;
  printStatus("Working...");
  resetStats();
  Timer.restart();
}

void startReporting()
{
  theState = REPORTING;
  printStatus("Reporting...");
}

void onPairing()
{
  theMessage.type = Message::HELLO;
  Message::Address from;
  if (theMessage.sendThrough(manager, SERVER_ADDRESS)
    && theMessage.receiveThrough(manager, RECEIVE_TIMEOUT, &from)
    && Message::WELCOME == theMessage.type)
  {
    printStatus("Paired.");
    startWaiting();
    return;
  }
  else
  {
    delay(50 + random(100)); // [50, 150)
  }

  maybePrintStatus("Pairing...");
}

void onWaiting()
{
  if (manager.available())
  {
    Message::Address from;
    if (theMessage.receiveThrough(manager, &from) // TODO: Maybe check if from == SERVER_ADDRESS to make it possible for multiple networks to coexist?
      && Message::WORK == theMessage.type) 
    {
      // Wait to allow other clients to receive the WORK message.
      delay(1000 + random(500)); // [1000, 1500)
      startWorking();
      // Do not reply, nobody is waiting for it.
      return;
    }
  }

  maybePrintStatus("Waiting...");
}

void onWorking()
{
  ++numTotal;
  
  theMessage.type = Message::PING;
  theMessage.data.pingTime = millis();
  if (theMessage.sendThrough(manager, SERVER_ADDRESS))
  {
    ++numSuccess;
    
    Message::Address from;
    if (theMessage.receiveThrough(manager, RECEIVE_TIMEOUT, &from))
    {
      ++numReply;
      
      if (Message::PONG == theMessage.type)
      {
        const unsigned long currentT = millis();
        {
          TimerClass::Pause pause;
          
          updatePingTimes(currentT - theMessage.data.pongTime);
          Serial.print("PING ");
          Serial.print(currentT - theMessage.data.pongTime);
          Serial.println("ms.");
        }
      }
      else if (Message::QUERY == theMessage.type)
      {
        startReporting();
        return;
      }
      else if (Message::TUNE == theMessage.type)
      {
        // Duplicated in onReporting().
        tune(theMessage.data.tuningParams, driver, manager);
        startPairing();
        return;
      }
      else
      {
        Serial.print("Error: unexpected type of the received message (");
        Serial.print(theMessage.type);
        Serial.println(").");
      }
    }    
  }
  else
  {
    Serial.println("Error: sendThrough failed."); 
  }
  
  delay(10 + random(5)); // [10, 15)

  maybePrintStatus("Working...");
}

void onReporting()
{
  // FIXME: Client can get stuck here. A timeout?
  
  theMessage.type = Message::REPORT;
  serializeStats(theMessage.data.report);
  if (theMessage.sendThrough(manager, SERVER_ADDRESS)) 
  {
    Message::Address from;
    if (theMessage.receiveThrough(manager, RECEIVE_TIMEOUT, &from)
      && from == SERVER_ADDRESS
      && Message::OK == theMessage.type)
    {
      printStatus("Report delivered.");
      printStats();
      startWaiting();
      return;
    }
    else if (Message::TUNE == theMessage.type)
    {
      // Duplicated in onReporting().
      tune(theMessage.data.tuningParams, driver, manager);
      startPairing();
      return;
    }
    else
    {
      Serial.print("Error: unexpected type of the received message (");
      Serial.print(theMessage.type);
      Serial.println(").");
    }
  }
  
  delay(50 + random(100)); // [50, 150)
  
  maybePrintStatus("Reporting...");
}


////////////////////////////////////////////////////////////////////////////////

void setup() 
{
  Serial.begin(9600);
  Serial.print("Started client ");
  Serial.print(CLIENT_ADDRESS);
  Serial.println(". Welcome!");

  if (manager.init())
  {
    Timer.start();
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
    case WAITING:
      onWaiting();
      break;
    case WORKING:
      onWorking();
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
}
