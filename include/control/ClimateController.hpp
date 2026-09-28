#pragma once
// Control climático con histéresis (secciones 31/32 y 67).
// Ventilación, calefacción, humidificación y extracción.

#include <Arduino.h>
#include "core/Types.hpp"
#include "sensors/SensorManager.hpp"
#include "actuators/ActuatorManager.hpp"
#include "storage/History.hpp"

namespace gh {

class ClimateController {
public:
  void begin(SensorManager* s, ActuatorManager* a, History* h) { sensors_ = s; actuators_ = a; history_ = h; }

  void update(const SystemConfig& cfg) {
    if (!cfg.featureClimate) return;
    float t = sensors_->temperature();
    float h = sensors_->humidity();
    if (isnan(t) || isnan(h)) return; // No actuar sin lecturas válidas (sección 56)

    // Ventilación por temperatura con histéresis (sección 31/67).
    if (t >= cfg.ventOnTemp) setFans(100);
    else if (t <= cfg.ventOffTemp) setFans(0);

    // Calefacción (opcional).
    if (cfg.featureHeating) {
      if (t <= cfg.tempMin) actuators_->setHeater(100);
      else if (t >= cfg.tempTarget) actuators_->setHeater(0);
    }

    // Humidificación (opcional).
    if (cfg.featureHumidification) {
      if (h <= cfg.humMin) actuators_->setHumidifier(100);
      else if (h >= cfg.humTarget) actuators_->setHumidifier(0);
    }

    // Extracción por humedad alta (sección 32).
    if (h >= cfg.ventHumMax) setExtractors(100);
    else if (h <= cfg.ventHumMax - cfg.humHysteresis) setExtractors(0);
  }

private:
  SensorManager* sensors_ = nullptr;
  ActuatorManager* actuators_ = nullptr;
  History* history_ = nullptr;

  void setFans(float pct) { for (uint8_t i = 0; i < 3; i++) actuators_->setFan(i, pct); }
  void setExtractors(float pct) { for (uint8_t i = 0; i < 2; i++) actuators_->setExtractor(i, pct); }
};

} // namespace gh
