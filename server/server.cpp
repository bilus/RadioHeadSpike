// An echo server on steroids. It runs compatible clients through a set of predefined configurations (scenarios)
// outputing statistics about their performance for each channel/data rate/power combination.

#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>

#include "message.h"
#include "helpers.h"
#include "scenarios.h"
#include "report.h"
#include "paired_devices.h"

#define SERVER_ADDRESS 1 // Needs to match client.cpp

// Singleton instance of the radio driver
RH_NRF24 theDriver(9);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram theManager(theDriver, SERVER_ADDRESS);

// Message transmitted between the server and clients. Reused for sending/receiving.
Message theMessage; // Don't put this on the stack.

////////////////////////////////////////////////////////////////////////////////

// The server is a state machine. Below is the list of states it may be in. 

// Note: when the device boots, setup() is called and the state is initially undefined.
enum State
{
  PAIRING,      // See startPairing() and onPairing().
  TUNING,       // See startTuning() and onTuning().
  WORKING,      // See startWorking() and onWorking().
  REPORTING     // See startReporting() and onReporting().
};

State theState;

////////////////////////////////////////////////////////////////////////////////

// TODO: We don't nee the individual *StartAt variables, just theStateEnteredAt
// will suffice because there can only be one state at a time.

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

// Send out the message to all clients. The current version uses broadcast
// but if it proves unreliable, the implementation may change to the for-loop
// version which sends to each paired device individually.
void broadcast(const Message::Type& type)
{
  // Broadcast the message repeatly for approx. 2s.
  
  theMessage.type = type;
  theMessage.repeatedlyBroadcast(theManager, 2000);  

  // theMessage.type = Message::WORK;
  // for (int i = 0; i < thePairedDeviceCount; ++i)
  // {
  //   theMessage.sendThrough(theManager, thePairedDevices[i]); // TODO: What if it fails due to a temporary eror?
  //   // No reply is expected.
  // }
}

// Count received pings.
void onPing(const Message::Address& from)
{
  Device::Stats* stats = findPairedDeviceStats(from);
  if (NULL != stats)
  {
    ++stats->numTotal;
  }
  else
  {
    Serial.print("Error: No paired device found (");
    Serial.print(from);
    Serial.println(")");
  }  
}

////////////////////////////////////////////////////////////////////////////////

// Enter the PAIRING state. See onPairing() for details.
void startPairing()
{
  emptyPairedDevices();
  theState = PAIRING;
  thePairingStartedAt = millis();
  printStatus("Pairing...");
}

// Enter the WORKING state. See onWorking for details.
void startWorking()
{
  // Before entering the state, ask clients to enter the WORKING state. See
  // onWorking() in ../client/client.cpp for details.
  
  Serial.print("Asking paired devices to start working (");
  Serial.print(getPairedDeviceCount());
  Serial.println(")");

  broadcast(Message::WORK);

  theState = WORKING;
  theWorkingStartAt = millis();
  printStatus("Working...");
}

// Enter the REPORTING state. See onReporting for details.
void startReporting()
{
  theState = REPORTING;
  theReportingStartAt = millis();
  printStatus("Reporting...");
}

// Enter the TUNING state. See onTuning for details.
void startTuning()
{
  theState = TUNING;
  theTuningStartAt = millis();
  printStatus("Tuning...");
}

// Handle PAIRING state.
void onPairing()
{
  // Wait for HELLO messages from devices and pair with each new one by replying with WELCOME.
  // After PAIRING_PERIOD, switch to WORKING state.
  
  if (millis() - thePairingStartedAt > PAIRING_PERIOD)
  {
    startWorking();
    return;
  }  

  if (theManager.available())
  {
    Message::Address from;
    if (theMessage.receiveThrough(theManager, &from))
    {
      if (Message::HELLO == theMessage.type)
      {
        theMessage.type = Message::WELCOME;

        // TODO: Generate an id.
        if (theMessage.sendThrough(theManager, from))
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
        theMessage.sendThrough(theManager, from);
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
  
  if (theManager.available())
  {
    Message::Address from;
    if (theMessage.receiveThrough(theManager, &from))
    {
      if (Message::PING == theMessage.type)
      {
        onPing(from);
        
        theMessage.type = Message::PONG;
        // The rest stays the same.
        theMessage.sendThrough(theManager, from);
        // Serial.println("PING-PONG!");
      }
      else
      {
        theMessage.type = Message::ERROR;
        theMessage.sendThrough(theManager, from);
      }
    }  
  }

  maybePrintStatus("Working...");
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
  
  if (theManager.available())
  {
    Message::Address from;
    if (theMessage.receiveThrough(theManager, &from))
    {
      if (Message::REPORT == theMessage.type)
      {
        printReport(from, theMessage.data.report);
        theMessage.type = Message::OK;
        theMessage.sendThrough(theManager, from);
      }
      else
      {
        // After switching to REPORTING state, the client will keep PINGing (it'll send at least one 
        // PING) and the server needs to keep up with it to keep stats in sync.
        if (Message::PING == theMessage.type)
        {
          onPing(from);
        }
        
        theMessage.type = Message::QUERY;
        theMessage.sendThrough(theManager, from);
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
    applyCurrentScenario(theDriver, theManager);
    nextScenario();
    startPairing();
    return;
  }

  if (theManager.available())
  {
    Message::Address from;
    if (theMessage.receiveThrough(theManager, &from))
    {
      theMessage.type = Message::TUNE;
      applyCurrentScenario(theMessage.data.tuningParams);
      theMessage.sendThrough(theManager, from);
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

  if (theManager.init())
  {
    applyCurrentScenario(theDriver, theManager);
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

