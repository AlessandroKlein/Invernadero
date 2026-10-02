#pragma once
// Registro de módulos del firmware (decisiones §21). El mismo concepto de módulo
// (instalado/habilitado/deshabilitado/versión/dependencias/capacidades) aplica a
// frontend, servidor y firmware. Aquí modela los módulos embebidos (drivers y
// subsistemas) para que el resto de la plataforma consulte qué está activo.

#include <Arduino.h>

#include "core/PlatformTypes.hpp"

namespace gh {

class ModuleRegistry {
public:
  static constexpr uint8_t MAX_MODULES = 16;

  // Registra o actualiza un módulo por id. Devuelve false si no hay espacio.
  bool registerModule(const ModuleDescriptor& m);
  // Registra los módulos embebidos del firmware (core, sensores, red, ...).
  void registerBuiltins();
  bool setState(const char* id, ModuleState s);
  bool get(const char* id, ModuleDescriptor& out) const;
  bool isEnabled(const char* id) const;
  bool has(const char* id) const { return find(id) >= 0; }
  uint8_t count() const { return count_; }

  // Verifica que todas las dependencias de un módulo estén registradas.
  bool dependenciesSatisfied(const char* id) const;

  void snapshot(ModuleDescriptor* out, size_t max, size_t& n) const;
  String toJson() const;

private:
  ModuleDescriptor modules_[MAX_MODULES];
  uint8_t count_ = 0;
  int8_t find(const char* id) const;
};

} // namespace gh
