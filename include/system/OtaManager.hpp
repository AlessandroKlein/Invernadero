#pragma once
// Actualización OTA (sección 59) con ArduinoOTA (local) y descarga desde el
// servidor central (applyFromUrl) con rollback vía doble partición (app0/app1).
// La configuración (NVS) y los datos (SPIFFS) viven fuera de las particiones de
// app, por lo que se preservan entre actualizaciones.

#include <Arduino.h>
#include <ArduinoOTA.h>

namespace gh {

class OtaManager {
public:
  void begin(const char* hostname);
  void loop() { ArduinoOTA.handle(); }

  // Descarga y aplica un firmware desde una URL (HTTP/HTTPS). Verifica SHA-256
  // cuando expectedSha tiene 64 hex chars. Retorna true si la descarga e
  // instalación terminaron correctamente (se reinicia inmediatamente).
  bool applyFromUrl(const String& url, const String& expectedSha);

  bool updating() const { return updating_; }

private:
  bool updating_ = false;
};

} // namespace gh
