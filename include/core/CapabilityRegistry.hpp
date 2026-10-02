#pragma once
// Registro de capacidades del dispositivo (decisiones §23 / README §166).
// Determina qué funcionalidades están disponibles para la interfaz y el resto
// del sistema, de modo que la UI no muestre opciones que el hardware no soporta.

#include <Arduino.h>

#include "core/PlatformTypes.hpp"

namespace gh {

class CapabilityRegistry {
public:
  static constexpr uint8_t MAX_CAPABILITIES = 32;
  static constexpr uint8_t CAP_MAX_LEN = 16;

  // Añade una capacidad (idempotente). Devuelve false si no hay espacio o el
  // nombre es inválido.
  bool add(const char* cap);
  bool remove(const char* cap);
  bool has(const char* cap) const;
  void clear();
  // Sincroniza con las capacidades estáticas reportadas por Device::info().
  void syncFrom(const DeviceInfo& info);
  uint8_t count() const { return count_; }

  // Copia las capacidades a un buffer externo (para API/diagnóstico).
  void snapshot(char out[MAX_CAPABILITIES][CAP_MAX_LEN], uint8_t& n) const;
  String toJson() const;

private:
  char caps_[MAX_CAPABILITIES][CAP_MAX_LEN] = {};
  uint8_t count_ = 0;
  int8_t find(const char* cap) const;
};

} // namespace gh
