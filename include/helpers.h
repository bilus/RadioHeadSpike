#ifndef DLY_HELPERS_H
#define DLY_HELPERS_H

unsigned int theLastPrintStatusTime = 0; 
#define PRINT_STATUS_EVERY 2000

// Print status.
void printStatus(const char* s)
{
  Serial.println(s);
}

// Print s no more often than PRINT_STATUS_EVERY ms.
void maybePrintStatus(const char* s)
{
  const unsigned int t = millis();
  if (t - theLastPrintStatusTime >= PRINT_STATUS_EVERY)
  {
    printStatus(s);
    theLastPrintStatusTime = t;
  }
}

#endif