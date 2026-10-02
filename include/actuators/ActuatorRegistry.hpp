#pragma once
// Catálogo configurable de actuadores (decisiones §20). Modelo de "qué
// actuadores hay" definido por configuración, independiente del cableado físico
// compilado. Complementa al ActuatorManager, que aplica las salidas en tiempo de
// ejecución con la jerarquía seguridad > manual > automático.

#include <Arduino.h>

#include "core/PlatformTypes.hpp"

namespace gh {

class ActuatorRegistry {
public:
  static constexpr uint8_t MAX_ENTRIES = 32;

  // Registra o actualiza un actuador por id lógico. False si no hay espacio.
  bool registerActuator(const ActuatorEntry& e);
  // Puebla el catálogo a partir de la configuración actual (flags de Types.hpp).
  void buildFromConfig(const SystemConfig& cfg);
  bool remove(const char* id);
  bool get(const char* id, ActuatorEntry& out) const;
  bool has(const char* id) const { return find(id) >= 0; }
  uint8_t count() const { return count_; }
  uint8_t countEnabled() const;

  void snapshot(ActuatorEntry* out, size_t max, size_t& n) const;
  String toJson() const;

private:
  ActuatorEntry entries_[MAX_ENTRIES];
  uint8_t count_ = 0;
  int8_t find(const char* id) const;
};

} // namespace gh
