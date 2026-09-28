#pragma once
// Gestor de actuadores. Abstrae la salida física (74HC595 soft-PWM o MCP23017)
// y aplica la jerarquía: SEGURIDAD > MANUAL > AUTOMÁTICO > PROGRAMADO (sección 57).

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "core/Types.hpp"
#include "hardware/ShiftRegister595.hpp"
#include "hardware/Mcp23017.hpp"

namespace gh {

class ActuatorManager {
public:
  static constexpr uint8_t MAX_ACTUATORS = 32;

  void begin(const SystemConfig& cfg, ShiftRegister595* shift, Mcp23017* mcp = nullptr);
  void reconfigure(const SystemConfig& cfg);

  // Establece la orden deseada (0..100) desde los controladores o el usuario.
  void setRequest(ActuatorRole role, uint8_t index, float pct);
  // Sobrescritura de seguridad: fuerza una salida (0..100). -1 = sin sobrescritura.
  void setSafetyOverride(ActuatorRole role, uint8_t index, float pct);
  void clearSafetyOverrides();

  // Resuelve seguridad + orden + tiempos mínimos y escribe al hardware.
  void apply();
  // Estado seguro: apaga todo y escribe (sección 21.14).
  void allSafeState();

  float output(ActuatorRole role, uint8_t index) const;
  bool isOn(ActuatorRole role, uint8_t index) const;

  // Snapshot para API/WebSocket.
  void snapshot(ActuatorState* out, size_t max, size_t& n) const;
  String toJson() const;
  uint8_t count() const { return count_; }

  // Setters específicos por tipo (usados por los controladores).
  void setPump(float pct) { setRequest(ActuatorRole::PUMP, 0, pct); }
  void setValve(uint8_t v, float pct) { setRequest(ActuatorRole::VALVE, v, pct); }
  void setFan(uint8_t i, float pct) { setRequest(ActuatorRole::FAN, i, pct); }
  void setExtractor(uint8_t i, float pct) { setRequest(ActuatorRole::EXTRACTOR, i, pct); }
  void setHeater(float pct) { setRequest(ActuatorRole::HEATER, 0, pct); }
  void setHumidifier(float pct) { setRequest(ActuatorRole::HUMIDIFIER, 0, pct); }
  void setLight(uint8_t i, float pct) { setRequest(ActuatorRole::LIGHT, i, pct); }
  void setWindowOpen(bool on) { setRequest(ActuatorRole::WINDOW_OPEN, 0, on ? 100 : 0); setRequest(ActuatorRole::WINDOW_CLOSE, 0, 0); }
  void setWindowClose(bool on) { setRequest(ActuatorRole::WINDOW_CLOSE, 0, on ? 100 : 0); setRequest(ActuatorRole::WINDOW_OPEN, 0, 0); }
  void setRoofOpen(bool on) { setRequest(ActuatorRole::ROOF_OPEN, 0, on ? 100 : 0); setRequest(ActuatorRole::ROOF_CLOSE, 0, 0); }
  void setRoofClose(bool on) { setRequest(ActuatorRole::ROOF_CLOSE, 0, on ? 100 : 0); setRequest(ActuatorRole::ROOF_OPEN, 0, 0); }
  void setShadeOpen(bool on) { setRequest(ActuatorRole::SHADE_OPEN, 0, on ? 100 : 0); setRequest(ActuatorRole::SHADE_CLOSE, 0, 0); }
  void setShadeClose(bool on) { setRequest(ActuatorRole::SHADE_CLOSE, 0, on ? 100 : 0); setRequest(ActuatorRole::SHADE_OPEN, 0, 0); }
  void setAlarm(bool on) { setRequest(ActuatorRole::ALARM, 0, on ? 100 : 0); }

private:
  ActuatorState slots_[MAX_ACTUATORS];
  uint8_t count_ = 0;
  ShiftRegister595* shift_ = nullptr;
  Mcp23017* mcp_ = nullptr;
  SystemConfig cfg_;
  mutable SemaphoreHandle_t mutex_ = nullptr;

  int8_t findSlot(ActuatorRole role, uint8_t index) const;
  void writeChannel(const ActuatorState& s, float pct);
};

} // namespace gh
