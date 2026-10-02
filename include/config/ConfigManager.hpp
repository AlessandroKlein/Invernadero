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

  // Autenticación local de la web (secciones 19/154). El password se guarda en
  // una key NVS separada para que no se exporte en el JSON de configuración.
  bool auth(const String& user, const String& pass) const;
  String adminUser() const;

  // Token de API para autorizar control/modificación desde el servidor central.
  // Se almacena en una key NVS separada (nunca se exporta en el JSON) y se
  // gestiona desde la página de configuración.
  String apiToken() const;
  String rotateApiToken();                  // genera uno nuevo aleatorio y lo persiste
  void revokeApiToken();                    // lo elimina
  bool validateApiToken(const String& t) const;

  // Serialización JSON (usada por la API REST y el arranque).
  static String toJson(const SystemConfig& c);
  static bool fromJson(const String& json, SystemConfig& out);

  // Migraciones de esquema: actualiza la configuración desde su schemaVersion
  // al actual (GH_CONFIG_SCHEMA_VERSION). Idempotente.
  static void migrate(SystemConfig& c);

  // Merge profundo de dos JSON de configuración (base + capa parcial). Útil para
  // la configuración por capas: FACTORY → ... → USER.
  static String mergeLayerJson(const String& base, const String& layer);

private:
  SystemConfig cfg_;                 // Configuración actual (CONFIG ACTUAL)
  SystemConfig prev_;                // Configuración anterior (CONFIG ANTERIOR, rollback)
  mutable SemaphoreHandle_t mutex_ = nullptr; // Protege cfg_/prev_
  mutable Preferences prefs_;        // Almacenamiento NVS (getString no es const)
  static constexpr const char* NVS_NS = "ghcfg";
  static constexpr const char* NVS_KEY = "config";
  static constexpr const char* NVS_KEY_PREV = "config_prev";
  static constexpr const char* NVS_KEY_ADMIN_PASS = "admin_pass";
  static constexpr const char* NVS_KEY_API_TOKEN = "api_token";

  void ensureAdminPassword();        // Deriva del UID si está vacía y persiste
};

} // namespace gh
