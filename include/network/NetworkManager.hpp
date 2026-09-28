#pragma once
// Gestión de red (secciones 46/86/87): WiFi STA con AP de respaldo, mDNS y NTP.
// Si no hay SSID configurado, arranca un Access Point temporal para configurar.

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>

#include "core/Types.hpp"

namespace gh {

class NetworkManager {
public:
  void begin(const SystemConfig& cfg);
  void loop();                 // Mantiene conexión y reconecta si es necesario
  bool connected() const { return WiFi.status() == WL_CONNECTED; }
  String ip() const { return WiFi.localIP().toString(); }
  bool isApMode() const { return apMode_; }

private:
  bool apMode_ = false;
  String hostname_ = "invernadero";

  void startAp(const SystemConfig& cfg);
  void startSta(const SystemConfig& cfg);
};

} // namespace gh
