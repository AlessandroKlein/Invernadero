#pragma once
// Estación meteorológica externa (secciones 244-246).
// El ESP32 realiza la petición HTTP a un URL que publica JSON y extrae solo las
// magnitudes configuradas. Los nombres de campo (temp, hum, ane, pluv, ...) son
// configurables porque cada estación publica un esquema propio y puede enviar
// más datos de los que el ESP32 necesita.

#include <Arduino.h>
#include "core/Types.hpp"

namespace gh {

class WeatherStation {
public:
  void begin(const SystemConfig& cfg) { reconfigure(cfg); }
  void reconfigure(const SystemConfig& cfg);
  void loop();   // consulta periódica según weatherIntervalMs

  bool enabled() const { return cfg_.weatherEnabled; }
  bool available() const { return ok_; }
  float temperature() const { return temp_; }
  float humidity() const { return hum_; }
  float windSpeed() const { return wind_; }
  float rain() const { return rain_; }
  float pressure() const { return pressure_; }
  float light() const { return light_; }
  uint32_t lastFetchMs() const { return lastFetchMs_; }
  String lastError() const { return lastError_; }
  String toJson() const;

private:
  SystemConfig cfg_;
  bool ok_ = false;
  float temp_ = NAN, hum_ = NAN, wind_ = NAN, rain_ = NAN;
  float pressure_ = NAN, light_ = NAN;
  uint32_t lastFetchMs_ = 0;
  String lastError_;

  bool fetch();
};

} // namespace gh
