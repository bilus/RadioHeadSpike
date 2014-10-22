#ifndef DLY_HELPERS_H
#define DLY_HELPERS_H

unsigned int theLastPrintStatusTime = 0;
#define PRINT_STATUS_EVERY 2000

void maybePrintStatus(const char* s)
{
  const unsigned int t = millis();
  if (t - theLastPrintStatusTime >= PRINT_STATUS_EVERY)
  {
    Serial.println(s);
    theLastPrintStatusTime = t;
  }
}

#endif