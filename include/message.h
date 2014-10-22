#ifndef DLY_MESSAGE_H
#define DLY_MESSAGE_H

struct Message
{
  enum Type
  {
    ERROR = 0,
    PING = 1,
    PONG = 2,
    RESTART = 3
  };
  
  union Data
  {
    unsigned long pingTime;
    unsigned long pongTime;
    struct 
    {
      byte channel;
    } restartParams; 
  };
  
  byte type;
  Data data;
};

#endif