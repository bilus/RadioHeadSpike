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

  Serial.println("==========================================================================");
  Serial.print("Total:     ");
  Serial.print(numTotal);
  Serial.print(" ");
  Serial.print("Successes:     ");
  Serial.print(numSuccess);
  Serial.print(" ");
  Serial.print("Replies:     ");
  Serial.print(numReply);
  Serial.println("");

  const float timeSec = float(start) / 1000;
  Serial.print("Total time: ");
  Serial.print(timeSec);
  Serial.println("s");

  Serial.print("Total/sec: ");
  Serial.print(numTotal / timeSec);
  Serial.print(" ");
  Serial.print("Successes/sec: ");
  Serial.print(numSuccess / timeSec);
  Serial.print(" ");
  Serial.print("Replies/sec: ");
  Serial.print(numReply / timeSec);
  Serial.println("");
  
  Serial.print("Avg ping: ");
  Serial.print(getAvgPingTime());
  Serial.print("ms ");
  Serial.print("Min ping: ");
  Serial.print(minPingTime);
  Serial.print("ms ");
  Serial.print("Max ping: ");
  Serial.print(maxPingTime);
  Serial.println("ms");

  Serial.print("(printed in ");
  Serial.print(Timer.elapsed() - start);
  Serial.println("ms)");
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