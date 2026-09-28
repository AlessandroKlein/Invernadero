#pragma once
// Gestión de la configuración no volátil (NVS) mediante JSON.
// La configuración se serializa como JSON y se guarda en Preferences/NVS,
// de modo que sea editable desde la web sin recompilar el firmware.

#include <Arduino.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "core/Types.hpp"

namespace gh {

class ConfigManager {
public:
  void begin();                     // Abre NVS y carga (o crea) la configuración
  SystemConfig get() const;         // Devuelve una copia (protegida por mutex)
  void set(const SystemConfig& c);  // Actualiza y persiste
  void save();                      // Persiste la configuración actual
  void factoryReset();              // Restaura valores de fábrica (sección 85)

  // Serialización JSON (usada por la API REST y el arranque).
  static String toJson(const SystemConfig& c);
  static bool fromJson(const String& json, SystemConfig& out);

private:
  SystemConfig cfg_;                 // Configuración en memoria
  mutable SemaphoreHandle_t mutex_ = nullptr; // Protege cfg_
  Preferences prefs_;                // Almacenamiento NVS
  static constexpr const char* NVS_NS = "ghcfg";
  static constexpr const char* NVS_KEY = "config";
};

} // namespace gh
