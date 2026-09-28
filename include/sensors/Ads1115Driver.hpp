#pragma once
// ADC externo de 16 bits ADS1115 (I²C) para sensores analógicos (sección 10).
// Se usa para humedad de suelo y pH. Admite hasta 4 canales por chip.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

namespace gh {

class Ads1115Driver {
public:
  bool begin(uint8_t addr, TwoWire* wire = &Wire);
  bool available() const { return available_; }
  // Lectura en crudo de un canal single-ended (0..3).
  int16_t readRaw(uint8_t channel);
  // Lectura en voltios (según la ganancia configurada).
  float readVoltage(uint8_t channel);
  void setGain(adsGain_t gain);

private:
  Adafruit_ADS1115 ads_;
  bool available_ = false;
  uint8_t addr_ = 0x48;
  TwoWire* wire_ = nullptr;
};

} // namespace gh
