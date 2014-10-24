#ifndef DLY_TIMER_H
#define DLY_TIMER_H

// A timer with one millisecond resolution.

class TimerClass
{
public:
  
  // Start the timer.
  void start()
  {
    myStartMillis = millis();
    myPausesMillis = 0;
  }
  
  // Reset the timer.
  void restart()
  {
    start();
  }

  // Return the number of elapsed ms minus all pauses (see Pause).
  unsigned long elapsed()
  {
    // Serial.print("Pausing for: ");
    // Serial.print(myPausesMillis);
    // Serial.println("ms");
    return millis() - myStartMillis - myPausesMillis;
  }

  // Lets you exclude some portions of code using RAII.

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