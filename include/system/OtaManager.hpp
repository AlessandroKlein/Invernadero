#pragma once
// Actualización OTA (sección 59) con ArduinoOTA (local) y descarga desde el
// servidor central (applyFromUrl) con rollback vía doble partición (app0/app1).
// La configuración (NVS) y los datos (SPIFFS) viven fuera de las particiones de
// app, por lo que se preservan entre actualizaciones.
//
// applyFromUrl usa el flujo (Stream) de red activo:
//   - HTTP  → GET manual sobre Client* (WiFi o Ethernet/W5500), alimenta Update.
//   - HTTPS → HTTPClient + WiFiClientSecure (solo WiFi; TLS sobre W5500 clásico
//             no está disponible sin una pila con TLS, p. ej. W5500lwIP).

#include <Arduino.h>
#include <Client.h>
#include <ArduinoOTA.h>

namespace gh {

class OtaManager {
public:
  void begin(const char* hostname);
  void loop() { ArduinoOTA.handle(); }

  // Descarga y aplica un firmware desde una URL (HTTP/HTTPS). Verifica SHA-256
  // cuando expectedSha tiene 64 hex chars. Retorna true si la descarga e
  // instalación terminaron correctamente (se reinicia inmediatamente).
  // netClient: cliente de red activo (WiFi o Ethernet) para el caso HTTP.
  bool applyFromUrl(const String& url, const String& expectedSha, Client* netClient = nullptr);

  bool updating() const { return updating_; }

private:
  bool updating_ = false;
};

} // namespace gh
