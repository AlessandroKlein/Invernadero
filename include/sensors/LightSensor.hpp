#pragma once
// Sensor de iluminación BH1750 (I²C), sección 11.
// Se abstrae como "LightSensor" para poder sustituirlo por un sensor PAR/PPFD.
// Implementación I2C directa (comandos 0x01 encendido y 0x10 modo continuo).

#include <Arduino.h>
#include <Wire.h>

namespace gh {

class LightSensor {
public:
  bool begin(uint8_t addr = 0x23, TwoWire* wire = &Wire);
  bool available() const { return available_; }
  float readLux();                  // Devuelve lux o NaN si falla

private:
  bool available_ = false;
  uint8_t addr_ = 0x23;
  TwoWire* wire_ = nullptr;
};

} // namespace gh
