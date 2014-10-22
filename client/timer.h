#ifndef DLY_TIMER_H
#define DLY_TIMER_H

class TimerClass
{
public:
  
  void start()
  {
    myStartMillis = millis();
    myPausesMillis = 0;
  }
  
  void restart()
  {
    start();
  }

  unsigned long elapsed()
  {
    // Serial.print("Pausing for: ");
    // Serial.print(myPausesMillis);
    // Serial.println("ms");
    return millis() - myStartMillis - myPausesMillis;
  }

  class Pause
  {
  public:
  
    Pause()
    {
      myPauseStartMillis = millis();
    }
  
    ~Pause();
    
  protected:
    
    unsigned long myPauseStartMillis;
  };
  
protected:
  
  unsigned long myStartMillis;
  unsigned long myPausesMillis;
  
};

TimerClass Timer; // Beware of including in more than one .cpp file in the same project, each may end up with a separate timer.

inline TimerClass::Pause::~Pause()
{
  Timer.myPausesMillis += millis() - myPauseStartMillis;
}


#endif