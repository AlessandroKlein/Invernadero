#pragma once
// Contadores de arranque/reinicio persistidos en NVS (SEMA §134). Permiten
// detectar estaciones inestables (boot loops, watchdogs, brownouts, pánicos...).

#include <Arduino.h>
#include <Preferences.h>

namespace gh {

class BootCounters {
public:
  void begin();  // carga, registra el arranque actual (según causa) y persiste
  uint32_t get(const char* key) const;
  String toJson() const;

private:
  mutable Preferences prefs_;
  static constexpr const char* NVS_NS = "ghboot";
  void increment(const char* key);
};

} // namespace gh
