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
  void set(const SystemConfig& c);  // Actualiza, versiona y persiste (rollback previo)
  void save();                      // Persiste la configuración actual y la anterior
  void factoryReset();              // FACTORY RESET (sección 155)
  void resetNetwork();              // RESET NETWORK: solo parámetros de red (sección 155)
  void resetAutomation();           // RESET AUTOMATION: solo control/reglas (sección 155)
  bool rollback();                  // Restaura la configuración anterior (sección 104)

  // Serialización JSON (usada por la API REST y el arranque).
  static String toJson(const SystemConfig& c);
  static bool fromJson(const String& json, SystemConfig& out);

private:
  SystemConfig cfg_;                 // Configuración actual (CONFIG ACTUAL)
  SystemConfig prev_;                // Configuración anterior (CONFIG ANTERIOR, rollback)
  mutable SemaphoreHandle_t mutex_ = nullptr; // Protege cfg_/prev_
  Preferences prefs_;                // Almacenamiento NVS
  static constexpr const char* NVS_NS = "ghcfg";
  static constexpr const char* NVS_KEY = "config";
  static constexpr const char* NVS_KEY_PREV = "config_prev";
};

} // namespace gh
