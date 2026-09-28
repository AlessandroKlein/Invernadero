#pragma once
// Sensor de temperatura y humedad abstraído (sección 7).
// Un único firmware admite SHT31, AHT10 o AHT20 sin que las reglas dependan
// del modelo físico.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_AHTX0.h>

namespace gh {

class TempHumSensor {
public:
  enum class Kind : uint8_t { SHT31, AHT20 };

  bool begin(Kind kind, uint8_t addr, TwoWire* wire = &Wire);
  bool available() const { return available_; }
  // Lee temperatura (°C) y humedad (%RH). Devuelve false si hay error.
  bool read(float& temp, float& hum);

private:
  Kind kind_ = Kind::SHT31;
  bool available_ = false;
  Adafruit_SHT31 sht_;
  Adafruit_AHTX0 aht_;
};

} // namespace gh
