#include "control/IrrigationController.hpp"

namespace gh {

void IrrigationController::log(uint8_t sev, const char* msg) {
  if (history_) history_->add(sev, msg);
}

void IrrigationController::stopPump() {
  actuators_->setPump(0);
  actuators_->setValve(currentZone_, 0);
}

bool IrrigationController::startNextZone(const SystemConfig& cfg) {
  // Recorrer las zonas desde la actual buscando una que necesite riego.
  for (uint8_t i = 0; i < cfg.actValves; i++) {
    uint8_t z = (currentZone_ + i) % cfg.actValves;
    float soil = sensors_->soilMoisture(z);
    // Solo regar si el sensor es válido y el suelo está seco.
    if (!isnan(soil) && soil < cfg.soilMin) {
      currentZone_ = z;
      actuators_->setValve(z, 100);      // abrir válvula
      actuators_->setPump(100);          // encender bomba
      state_ = State::PUMP_START;
      stateStartMs_ = millis();
      irrigationStartMs_ = millis();
      log(0, "Riego iniciado");
      return true;
    }
  }
  return false;
}

void IrrigationController::update(const SystemConfig& cfg) {
  if (!cfg.featureIrrigation || !cfg.actPump) { stopPump(); state_ = State::IDLE; return; }

  uint32_t now = millis();

  switch (state_) {
    case State::IDLE: {
      // Intenta iniciar el riego de la primera zona que lo necesite.
      if (!startNextZone(cfg)) { /* sin zonas que regar */ }
      break;
    }
    case State::PUMP_START: {
      // Esperar el tiempo de verificación de caudal tras arrancar la bomba.
      if (now - stateStartMs_ >= cfg.flowCheckDelayMs) {
        float flow = sensors_->flowRate();
        if (flow < cfg.flowMin) {
          // Bomba sin caudal -> detener y alarmar (sección 36).
          stopPump();
          log(3, "ALARMA: bomba sin caudal");
          state_ = State::IDLE;
        } else if (flow > cfg.flowMin * 5.0f) {
          // Caudal excesivo -> posible rotura de tubería.
          stopPump();
          log(3, "ALARMA: caudal excesivo");
          state_ = State::IDLE;
        } else {
          log(0, "Caudal OK");
          state_ = State::IRRIGATING;
        }
      }
      break;
    }
    case State::IRRIGATING: {
      float soil = sensors_->soilMoisture(currentZone_);
      bool reachedTarget = (!isnan(soil) && soil >= cfg.soilTarget);
      bool timeout = (now - irrigationStartMs_ >= cfg.irrigationMaxTimeMs);
      // Si el tanque quedó vacío, la seguridad ya bloquea la bomba; detener.
      bool tankLow = sensors_->floatLow();

      if (reachedTarget || timeout || tankLow) {
        stopPump();
        if (reachedTarget) log(0, "Riego finalizado (objetivo)");
        else if (timeout) log(2, "Riego finalizado por tiempo máximo");
        else log(2, "Riego detenido: tanque vacío");
        state_ = State::IDLE;
        currentZone_ = (currentZone_ + 1) % cfg.actValves;
      }
      break;
    }
    default:
      state_ = State::IDLE;
      break;
  }
}

} // namespace gh
