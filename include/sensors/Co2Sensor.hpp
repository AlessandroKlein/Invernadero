#pragma once
// Sensor de CO₂ SCD40/SCD41 (NDIR, I²C), sección 13.
// Implementación I2C directa para evitar dependencias externas.
// Opcional: si no está habilitado, toda la lógica relacionada se desactiva.

#include <Arduino.h>
#include <Wire.h>

namespace gh {

class Co2Sensor {
public:
  bool begin(uint8_t addr = 0x62, TwoWire* wire = &Wire);
  bool available() const { return available_; }
  void startPeriodicMeasurement();  // Inicia medición continua
  // Lee CO₂ (ppm), temperatura (°C) y humedad (%RH). False si hay error.
  bool readMeasurement(uint16_t& co2, float& temp, float& hum);

private:
  bool available_ = false;
  uint8_t addr_ = 0x62;
  TwoWire* wire_ = nullptr;
  void writeCommand(uint16_t cmd);
  static uint8_t crc8(const uint8_t* data, size_t len);
};

} // namespace gh
