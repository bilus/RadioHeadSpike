// Client for the server defined in ../server/server.cpp.

#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>
#include <limits.h>

#include "timer.h"
#include "message.h"
#include "helpers.h"
#include "tuning.h"
#include "stats.h"

// CLIENT_ADDRESS is #defined externally by the ./deploy script and its dependencies.

#define SERVER_ADDRESS 1 // Needs to match server.cpp

// Singleton instance of the radio driver.
RH_NRF24 theDriver(9);

// Class to manage message delivery and receipt, using the driver declared above.
RHReliableDatagram theManager(theDriver, CLIENT_ADDRESS);

// Message transmitted between the server and clients. Reused for sending/receiving.
Message theMessage; // DO NOT put it on the stack!

////////////////////////////////////////////////////////////////////////////////

// The client is a state machine; below are its valid states except for the undefined
// state in setup() after it enters the initial state.

enum State
{
  PAIRING,
  WAITING,
  WORKING,
  REPORTING
};

State theState;

////////////////////////////////////////////////////////////////////////////////

// Enter the PAIRING state. See onPairing() for details.
void startPairing()
{
  theState = PAIRING;
  printStatus("Pairing...");
}

// Enter the WAITING state. See onWaiting() for details.
void startWaiting()
{
  theState = WAITING;
  printStatus("Waiting...");
}

// Enter the WORKING state. See onWorking() for details.
void startWorking()
{
  theState = WORKING;
  printStatus("Working...");
  resetStats();
  Timer.restart();
}

// Enter the REPORTING state. See onReporting() for details.
void startReporting()
{
  theState = REPORTING;
  printStatus("Reporting...");
}

// Handle the PAIRING state.
void onPairing()
{
  // Keep sending HELLO to the server until WELCOME is received.
  // At this point enter the WAITING state.
  
  theMessage.type = Message::HELLO;
  Message::Address from;
  if (theMessage.sendThrough(theManager, SERVER_ADDRESS)
    && theMessage.receiveThrough(theManager, RECEIVE_TIMEOUT, &from)
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

// Handle the WAITING state.
void onWaiting()
{
  // When WORK message is received, wait for 1-1s and enter the WORKING state.
  
  if (theManager.available())
  {
    Message::Address from;
    if (theMessage.receiveThrough(theManager, &from) // TODO: Maybe check if from == SERVER_ADDRESS to make it possible for multiple networks to coexist?
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

// Handle the WORKING state.
void onWorking()
{
  // PING the server and update the stats after receiving PONG.
  // Receiving QUERY switches the client into the REPORTING state
  // while after receiving TUNE, the client reinitializes using the new
  // channel, data rate etc. and switches into the PAIRING state.
  
  ++numTotal;
  
  theMessage.type = Message::PING;
  theMessage.data.pingTime = millis();
  if (theMessage.sendThrough(theManager, SERVER_ADDRESS))
  {
    ++numSuccess;
    
    Message::Address from;
    if (theMessage.receiveThrough(theManager, RECEIVE_TIMEOUT, &from))
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
        tune(theMessage.data.tuningParams, theDriver, theManager);
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

// Handle the REPORTING state.

void onReporting()
{
  // FIXME: Client can get stuck here because there's no timeout.
  
  // Keep sending REPORT messages containing the stats until the server
  // replies OK.
  // A TUNE reply causes the client to reinitialize using the new
  // channel, data rate etc. and switch into the PAIRING state
  
  theMessage.type = Message::REPORT;
  serializeStats(theMessage.data.report);
  if (theMessage.sendThrough(theManager, SERVER_ADDRESS)) 
  {
    Message::Address from;
    if (theMessage.receiveThrough(theManager, RECEIVE_TIMEOUT, &from)
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
      tune(theMessage.data.tuningParams, theDriver, theManager);
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

  if (theManager.init())
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
