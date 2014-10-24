#ifndef DLY_TUNING_H
#define DLY_TUNING_H

void tune(const Message::Data::TuningParams& p, RH_NRF24& driver, RHDatagram& manager)
{
  manager.init();
  driver.setChannel(p.channel);
  driver.setRF((RH_NRF24::DataRate)p.dataRate, (RH_NRF24::TransmitPower) p.power);

  Serial.println("==========================================================================");
  Serial.print("channel = ");
  Serial.print(p.channel);
  Serial.print(" data rate = ");
  Serial.print(p.dataRate);
  Serial.print(" power = ");
  Serial.println(p.power);
  Serial.println("==========================================================================");
}

#endif