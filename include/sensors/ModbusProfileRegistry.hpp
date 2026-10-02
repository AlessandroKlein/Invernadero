#pragma once
// Registro de perfiles e instancias Modbus (V9, README §277-279 / decisiones §17).
// Separa el "cómo se lee" (ModbusProfile: registros, tipo, escala, unidad) del
// "qué sensor es en esta instalación" (SensorInstance: nombre, slave, zona,
// estado de provisioning). Permite agregar sensores sin recompilar el firmware.

#include <Arduino.h>

#include "core/PlatformTypes.hpp"

namespace gh {

class ModbusProfileRegistry {
public:
  static constexpr uint8_t MAX_PROFILES = 16;
  static constexpr uint8_t MAX_INSTANCES = 24;

  // Registra o actualiza un perfil por id. False si no hay espacio.
  bool registerProfile(const ModbusProfile& p);
  bool registerInstance(const SensorInstance& i);
  bool setInstanceState(const char* id, ProvisioningState s);
  bool getProfile(const char* id, ModbusProfile& out) const;
  bool getInstance(const char* id, SensorInstance& out) const;
  bool hasProfile(const char* id) const { return findProfile(id) >= 0; }
  bool hasInstance(const char* id) const { return findInstance(id) >= 0; }
  uint8_t profileCount() const { return profileCount_; }
  uint8_t instanceCount() const { return instanceCount_; }
  String toJson() const;

private:
  ModbusProfile profiles_[MAX_PROFILES];
  uint8_t profileCount_ = 0;
  SensorInstance instances_[MAX_INSTANCES];
  uint8_t instanceCount_ = 0;
  int8_t findProfile(const char* id) const;
  int8_t findInstance(const char* id) const;
};

} // namespace gh
