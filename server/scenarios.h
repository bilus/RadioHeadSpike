#ifndef DLY_SCENARIOS_H
#define DLY_SCENARIOS_H

#include "tuning.h"

void scenario0(Message::Data::TuningParams& p)
{
  // The radio is configured by default to Channel 2, 2Mbps, 0dBm power.
  // This scenario is implicitly run at the beginning.
  p.channel = 2;
  p.dataRate = RH_NRF24::DataRate2Mbps;
  p.power = RH_NRF24::TransmitPower0dBm;
}
void scenario1(Message::Data::TuningParams& p)
{
  p.channel = 2;
  p.dataRate = RH_NRF24::DataRate250kbps;
  p.power = RH_NRF24::TransmitPower0dBm;
}
void scenario2(Message::Data::TuningParams& p)
{
  p.channel = 2;
  p.dataRate = RH_NRF24::DataRate2Mbps;
  p.power = RH_NRF24::TransmitPowerm18dBm;
}
void scenario3(Message::Data::TuningParams& p)
{
  p.channel = 2;
  p.dataRate = RH_NRF24::DataRate2Mbps;
  p.power = RH_NRF24::TransmitPower0dBm;
  // p.channel = 90;
  // p.dataRate = RH_NRF24::DataRate250kbps;
  // p.power = RH_NRF24::TransmitPower0dBm;
}

unsigned short theCurrentScenario = 0;
#define SCENARIO_COUNT 4
void (*theScenarios[SCENARIO_COUNT]) (Message::Data::TuningParams&) = {&scenario0, &scenario1, &scenario2, &scenario3};

inline const unsigned long getScenarioCount()
{
  return SCENARIO_COUNT;
}

void applyCurrentScenario(RH_NRF24& driver, RHDatagram& manager)
{
  Message::Data::TuningParams p;
  (*theScenarios[theCurrentScenario])(p);
  tune(p, driver, manager);
}

void applyCurrentScenario(Message::Data::TuningParams& p)
{
  (*theScenarios[theCurrentScenario])(p);  
}

void nextScenario()
{
  theCurrentScenario = (theCurrentScenario + 1) % getScenarioCount();
}

#endif