#ifndef DLY_MESSAGE_H
#define DLY_MESSAGE_H

#define RECEIVE_TIMEOUT 2000

// Message exchanged between server & client.
// Consists of Type and payload defined by Data.
// Even though it's a struct it contains messages to send & receive itself.

struct Message
{
  enum Type
  {
    ERROR = 0,
    OK,
    HELLO,
    WELCOME,
    WORK,
    PING,
    PONG,
    TUNE,
    QUERY,
    REPORT
  };
  
  union Data
  {
    unsigned long pingTime;
    unsigned long pongTime;

    struct TuningParams
    {
      byte channel;
      byte dataRate;
      byte power;
    };
    
    TuningParams tuningParams; 
    
    struct Report
    {
      // Important: To preserve space the types below are different from definitions in client.cpp.
      
      unsigned long numTotal;                 // Total number of send attempts.
      unsigned long numSuccess;               // Number of successful sendToWait calls.
      unsigned long numReply;                 // Number of successful recvfromAckTimeout calls.

      unsigned short avgPingTime;             // Average ping time in ms.
      unsigned short minPingTime;             // Minimum ping time in ms.
      unsigned short maxPingTime;             // Maximum ping time in ms.      
    };
    
    Report report;
  };
  
  byte type;
  Data data;
  
  typedef uint8_t Address;
  
  // Receive the message with a timeout.
  bool receiveThrough(RHReliableDatagram& manager, uint16_t timeout, Address* from)
  {
    // FIXME: Duplication.

    uint8_t len = sizeof(*this);
    if (manager.recvfromAckTimeout((byte *) this, &len, timeout, from))
    {
      if (len == sizeof(*this))
      {
        // FIXME: If you comment out the following line, it'll considerably slow down. Why? What is the best value?
        delay(5);
        // Serial.print("got request from : 0x");
        // Serial.print(*from, HEX);
        // Serial.print(": ");
        // Serial.println(this->type);
        return true;
      }
      else
      {
        Serial.print("Error: unexpected length of the received message. Expected: ");
        Serial.print(sizeof(*this));
        Serial.print(", actual: ");
        Serial.print(len);
        Serial.println(".");
      }
    }
        
    return false;
  }

  // Receive the message without a timeout.
  bool receiveThrough(RHReliableDatagram& manager, Address* from)
  {
    uint8_t len = sizeof(*this);
    if (manager.recvfromAck((byte *) this, &len, from))
    {
      if (len == sizeof(*this))
      {
        // FIXME: If you comment out the following line, it'll considerably slow down. Why? What is the best value?
        delay(5);
        // Serial.print("got request from : 0x");
        // Serial.print(*from, HEX);
        // Serial.print(": ");
        // Serial.println(this->type);
        return true;
      }
      else
      {
        Serial.print("Error: unexpected length of the received message. Expected: ");
        Serial.print(sizeof(*this));
        Serial.print(", actual: ");
        Serial.print(len);
        Serial.println(".");
      }
    }
        
    return false;
  }
  
  // Reliably send the message to a client.
  bool sendThrough(RHReliableDatagram& manager, const Address& to)
  {
    return manager.sendtoWait((byte *) this, sizeof(*this), to);
  }
  
  // Keep broadcasting the message for all devices for approx. the specified number of milliseconds.
  void repeatedlyBroadcast(RHReliableDatagram& manager, const unsigned long approxTimeLimit)
  {
    const unsigned long start = millis();
    while (millis() - start < approxTimeLimit)
    {
      this->sendThrough(manager, RH_BROADCAST_ADDRESS);
      delay(10);
    }
  }
};

#endif