#ifndef DLY_MESSAGE_H
#define DLY_MESSAGE_H

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
  
  // FIXME: Duplication.
  bool receiveThrough(RHReliableDatagram& manager, uint16_t timeout, Address* from)
  {
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
        Serial.println("Error: unexpected length of the received message.");
      }
    }
        
    return false;
  }

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
        Serial.println("Error: unexpected length of the received message.");
      }
    }
        
    return false;
  }
  
  bool sendThrough(RHReliableDatagram& manager, const Address& to)
  {
    return manager.sendtoWait((byte *) this, sizeof(*this), to);
  }
  
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