#pragma once
// Control de iluminación (secciones 29/30): horario + umbral por luz.

#include <Arduino.h>
#include <time.h>
#include "core/Types.hpp"
#include "sensors/SensorManager.hpp"
#include "actuators/ActuatorManager.hpp"

namespace gh {

class LightingController {
public:
  void begin(SensorManager* s, ActuatorManager* a) { sensors_ = s; actuators_ = a; }

  void update(const SystemConfig& cfg) {
    if (!cfg.featureLighting) {
      actuators_->setLight(0, 0);
      actuators_->setLight(1, 0);
      return;
    }
    // Hora local actual.
    time_t now = time(nullptr);
    struct tm ti;
    localtime_r(&now, &ti);
    int hour = ti.tm_hour;

    bool inSchedule = (hour >= cfg.lightStartHour && hour < cfg.lightEndHour);
    float lux = sensors_->lightLux();
    // Iluminación si estamos en horario o si la luz natural es insuficiente.
    bool needLight = inSchedule || (!isnan(lux) && lux < cfg.lightMinLux);

    float intensity = needLight ? cfg.lightIntensity : 0.0f;
    actuators_->setLight(0, intensity);
    actuators_->setLight(1, intensity);
  }

private:
  SensorManager* sensors_ = nullptr;
  ActuatorManager* actuators_ = nullptr;
};

} // namespace gh
