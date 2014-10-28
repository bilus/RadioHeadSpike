#ifndef DLY_STATS_H
#define DLY_STATS_H

#include "message.h"
#include "timer.h"

unsigned long numTotal = 0;            // Total number of send attempts.
unsigned long numSuccess = 0;          // Number of successful sendToWait calls.
unsigned long numReply = 0;            // Number of successful recvfromAckTimeout calls.


unsigned long minPingTime = ULONG_MAX; // Minimum ping time in ms.
unsigned long maxPingTime = 0;         // Maximum ping time in ms.
unsigned long totalPingTime = 0;       // Total ping time in ms; used to calculate the average ping time.

void resetStats()
{
  numTotal = 0;
  numSuccess = 0;
  numReply = 0;
  minPingTime = ULONG_MAX;
  maxPingTime = 0;
  totalPingTime = 0;
}

void updatePingTimes(unsigned long pingTime)
{
  totalPingTime += pingTime;
  if (minPingTime > pingTime) minPingTime = pingTime;
  if (maxPingTime < pingTime) maxPingTime = pingTime;
}

unsigned long getAvgPingTime()         // Returns the average ping time or ULONG_MAX if no ping info yet.
{
  if (numReply > 0)
  {
    return totalPingTime / numReply;
  }
  else
  {
    return ULONG_MAX;
  }
}

void printStats()
{
  const unsigned long start = Timer.elapsed();

  Serial.println(F("=========================================================================="));
  Serial.print(F("Total:     "));
  Serial.print(numTotal);
  Serial.print(F(" "));
  Serial.print(F("Successes:     "));
  Serial.print(numSuccess);
  Serial.print(F(" "));
  Serial.print(F("Replies:     "));
  Serial.print(numReply);
  Serial.println(F(""));

  const float timeSec = float(start) / 1000;
  Serial.print(F("Total time: "));
  Serial.print(timeSec);
  Serial.println(F("s"));

  Serial.print(F("Total/sec: "));
  Serial.print(numTotal / timeSec);
  Serial.print(F(" "));
  Serial.print(F("Successes/sec: "));
  Serial.print(numSuccess / timeSec);
  Serial.print(F(" "));
  Serial.print(F("Replies/sec: "));
  Serial.print(numReply / timeSec);
  Serial.println(F(""));
  
  Serial.print(F("Avg ping: "));
  Serial.print(getAvgPingTime());
  Serial.print(F("ms "));
  Serial.print(F("Min ping: "));
  Serial.print(minPingTime);
  Serial.print(F("ms "));
  Serial.print(F("Max ping: "));
  Serial.print(maxPingTime);
  Serial.println(F("ms"));

  Serial.print(F("(printed in "));
  Serial.print(Timer.elapsed() - start);
  Serial.println(F("ms)"));
}

void serializeStats(Message::Data::Report& report)
{
  report.numTotal = numTotal;
  report.numSuccess = numSuccess;
  report.numReply = numReply;
  report.avgPingTime = getAvgPingTime();
  report.minPingTime = minPingTime;
  report.maxPingTime = maxPingTime;
}

#endif