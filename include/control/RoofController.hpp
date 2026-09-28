#pragma once
// Control de techo y ventanas (secciones 26/68/71).
// Cierra por lluvia o viento (prioridad), abre por temperatura.

#include <Arduino.h>
#include "core/Types.hpp"
#include "sensors/SensorManager.hpp"
#include "actuators/ActuatorManager.hpp"

namespace gh {

class RoofController {
public:
  void begin(SensorManager* s, ActuatorManager* a) { sensors_ = s; actuators_ = a; }

  void update(const SystemConfig& cfg) {
    if (!cfg.featureRoof && !cfg.featureWindows) return;
    float t = sensors_->temperature();
    bool rain = (sensors_->rainRate() > 0.01f);   // lluvia activa
    float wind = sensors_->windSpeed();

    bool closeByWeather = (cfg.roofCloseOnRain && rain) ||
                          (cfg.roofCloseOnWind && !isnan(wind) && wind > cfg.windMaxSpeed);

    if (cfg.featureRoof) {
      if (closeByWeather) actuators_->setRoofClose(true);
      else if (!isnan(t) && t >= cfg.roofOpenTemp) actuators_->setRoofOpen(true);
      else if (!isnan(t) && t <= cfg.roofCloseTemp) actuators_->setRoofClose(true);
    }
    // Las ventanas exteriores siguen una lógica análoga.
    if (cfg.featureWindows) {
      if (closeByWeather) actuators_->setWindowClose(true);
      else if (!isnan(t) && t >= cfg.roofOpenTemp) actuators_->setWindowOpen(true);
      else if (!isnan(t) && t <= cfg.roofCloseTemp) actuators_->setWindowClose(true);
    }
  }

private:
  SensorManager* sensors_ = nullptr;
  ActuatorManager* actuators_ = nullptr;
};

} // namespace gh
