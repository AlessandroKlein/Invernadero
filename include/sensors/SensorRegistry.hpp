#pragma once
// Catálogo configurable de sensores (README §274-276 / decisiones §20). Registra
// instancias lógicas definidas por configuración, independientes del driver
// compilado, para agregar sensores sin recompilar el firmware. El SensorManager
// sigue siendo el orquestador en tiempo de ejecución; este registro es el modelo
// de "qué sensores hay" que consultan la UI, el servidor y la automatización.

#include <Arduino.h>
#include <Preferences.h>

#include "core/PlatformTypes.hpp"

namespace gh {

class SensorRegistry {
public:
  static constexpr uint8_t MAX_ENTRIES = 24;

  // Registra o actualiza un sensor por id lógico. False si no hay espacio.
  bool registerSensor(const SensorEntry& e);
  // Puebla el catálogo a partir de la configuración actual (flags de Types.hpp).
  void buildFromConfig(const SystemConfig& cfg);
  bool remove(const char* id);
  bool get(const char* id, SensorEntry& out) const;
  bool has(const char* id) const { return find(id) >= 0; }
  uint8_t count() const { return count_; }
  uint8_t countEnabled() const;

  void snapshot(SensorEntry* out, size_t max, size_t& n) const;
  String toJson() const;

  // Edita el catálogo desde JSON (actualiza entradas existentes por id). Se
  // ignoran ids desconocidos para no permitir inyectar drivers inexistentes.
  bool fromJson(const String& json);
  void load();   // carga el catálogo editado desde NVS
  void save();   // guarda el catálogo en NVS

private:
  SensorEntry entries_[MAX_ENTRIES];
  uint8_t count_ = 0;
  mutable Preferences prefs_;
  int8_t find(const char* id) const;
};

} // namespace gh
