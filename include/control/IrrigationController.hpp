#pragma once
// Control de riego por zonas (secciones 34-36/66).
// Máquina de estados: abre válvula, enciende bomba, verifica caudal, riega
// hasta alcanzar el objetivo, y protege la bomba (sin caudal / tanque vacío).

#include <Arduino.h>
#include "core/Types.hpp"
#include "sensors/SensorManager.hpp"
#include "actuators/ActuatorManager.hpp"
#include "storage/History.hpp"

namespace gh {

class IrrigationController {
public:
  void begin(SensorManager* s, ActuatorManager* a, History* h) { sensors_ = s; actuators_ = a; history_ = h; }

  void update(const SystemConfig& cfg);

private:
  enum class State : uint8_t { IDLE, VALVE_OPEN, PUMP_START, IRRIGATING, STOPPING };

  SensorManager* sensors_ = nullptr;
  ActuatorManager* actuators_ = nullptr;
  History* history_ = nullptr;

  State state_ = State::IDLE;
  uint8_t currentZone_ = 0;
  uint32_t stateStartMs_ = 0;
  uint32_t irrigationStartMs_ = 0;
  uint32_t lastEventMs_ = 0;

  bool startNextZone(const SystemConfig& cfg);
  void stopPump();
  void log(uint8_t sev, const char* msg);
};

} // namespace gh
