#ifndef DLY_MESSAGE_H
#define DLY_MESSAGE_H

struct Message
{
  enum Type
  {
    OK = 0,
    ERROR,
    HELLO,
    PING,
    PONG
  };
  
  union Data
  {
    unsigned long pingTime;
    unsigned long pongTime;
    struct RestartParams
    {
      byte channel;
      byte dataRate;
      byte power;
    };
    
    RestartParams restartParams; 
  };
  
  byte type;
  Data data;
  
  typedef uint8_t From;
  
  bool receiveThrough(RHReliableDatagram& manager, From* from)
  {
    if (manager.available())
    {
      uint8_t len = sizeof(*this);
      if (manager.recvfromAck((byte *) this, &len, from))
      {
        if (len == sizeof(*this))
        {
          // FIXME: If you comment out the following lines, it'll considerably slow down. Why?
          Serial.print("got request from : 0x");
          Serial.print(*from, HEX);
          Serial.print(": ");
          Serial.println(this->type);
          return true;
        }
        else
        {
          Serial.println("Error: unexpected length of the received message.");
        }
      }
    }
        
    return false;
  }
  
  bool sendThrough(RHReliableDatagram& manager, const From& to)
  {
    if (manager.sendtoWait((byte *) this, sizeof(*this), to))
    {
      return true;
    }
    else
    {
      Serial.println("sendtoWait failed");
    }
   
    return false;
  }
};

#endif