#ifndef DLY_REPORT_H
#define DLY_REPORT_H

#include "scenarios.h"
#include "message.h"
#include "paired_devices.h"

// Print a report from a client plus information about the current scenario.
void printReport(const Message::Address& from, const Message::Data::Report& report)
{
  Message::Data::TuningParams tuningParams;
  applyCurrentScenario(tuningParams);
  Serial.print(tuningParams.channel);
  Serial.print(",");
  Serial.print(tuningParams.dataRate);
  Serial.print(",");
  Serial.print(tuningParams.power);
  Serial.print(",");

  Serial.print(from);
  Serial.print(",");
  Serial.print(report.numTotal);
  Serial.print(",");
  Serial.print(report.numSuccess);
  Serial.print(",");
  Serial.print(report.numReply);
  Serial.print(",");

  Serial.print(report.avgPingTime);
  Serial.print(",");
  Serial.print(report.minPingTime);
  Serial.print(",");
  Serial.print(report.maxPingTime);
  Serial.print(",");

  Device::Stats* stats = findPairedDeviceStats(from);
  if (stats != NULL)
  {
    Serial.print(stats->numTotal);
  }
  else
  {
    Serial.print("-1"); // Oops!
  }
  
  Serial.println();
}

#endif