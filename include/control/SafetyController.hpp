#pragma once
// Sistema de seguridad (secciones 25/57): tiene prioridad sobre todo.
// Emergencia > seguridad > manual > automático > programado.

#include <Arduino.h>
#include "core/Types.hpp"
#include "core/PinMap.hpp"
#include "sensors/SensorManager.hpp"
#include "actuators/ActuatorManager.hpp"
#include "storage/History.hpp"

namespace gh {

class SafetyController {
public:
  void begin(SensorManager* s, ActuatorManager* a, History* h) {
    sensors_ = s; actuators_ = a; history_ = h;
    pinMode(pins::EMERGENCY_STOP_PIN, INPUT_PULLUP);
  }

  void update(const SystemConfig& cfg) {
    actuators_->clearSafetyOverrides();

    bool emergency = digitalRead(pins::EMERGENCY_STOP_PIN) == LOW;
    float t = sensors_->temperature();
    bool tempEmergency = !isnan(t) && t >= cfg.tempEmergency;
    bool tankLow = sensors_->floatLow();

    if (emergency) {
      // Parada total: todo a estado seguro.
      actuators_->allSafeState();
      history_->add(3, "PARADA DE EMERGENCIA");
      return;
    }

    if (tempEmergency) {
      // Temperatura crítica: apagar calefacción/bomba, ventilación máxima.
      actuators_->setSafetyOverride(ActuatorRole::HEATER, 0, 0);
      actuators_->setSafetyOverride(ActuatorRole::PUMP, 0, 0);
      actuators_->setSafetyOverride(ActuatorRole::HUMIDIFIER, 0, 0);
      for (uint8_t i = 0; i < 3; i++) actuators_->setSafetyOverride(ActuatorRole::FAN, i, 100);
      for (uint8_t i = 0; i < 2; i++) actuators_->setSafetyOverride(ActuatorRole::EXTRACTOR, i, 100);
      history_->add(3, "TEMP CRÍTICA: calefacción OFF, ventilación ON");
    }

    if (tankLow) {
      // Tanque vacío: bloquear la bomba (sección 36).
      actuators_->setSafetyOverride(ActuatorRole::PUMP, 0, 0);
      history_->add(2, "TANQUE VACÍO: bomba bloqueada");
    }
  }

private:
  SensorManager* sensors_ = nullptr;
  ActuatorManager* actuators_ = nullptr;
  History* history_ = nullptr;
};

} // namespace gh
