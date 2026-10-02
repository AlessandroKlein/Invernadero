#pragma once
// Gestión de red (secciones 46/86/87/247-248): interfaz intercambiable WiFi o
// Ethernet (W5500 por SPI). El usuario elige en configuración (netInterface);
// solo una queda activa por arranque. Expone un Client* genérico para MQTT/HTTP.

#include <Arduino.h>
#include <Client.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Ethernet.h>  // librería SPI Ethernet (W5500/W5200/W5100)

#include "core/Types.hpp"

namespace gh {

class NetworkManager {
public:
  void begin(const SystemConfig& cfg);
  void loop();                 // Mantiene conexión y reconecta si es necesario
  bool connected() const;
  String ip() const;
  bool isApMode() const { return apMode_; }
  bool isEthernet() const { return useEthernet_; }

  // Cliente de red activo (WiFi o Ethernet) para MQTT y HTTP.
  Client* client() { return client_; }

private:
  bool apMode_ = false;
  bool useEthernet_ = false;
  String hostname_ = "invernadero";
  Client* client_ = nullptr;
  WiFiClient wifiClient_;
  EthernetClient ethClient_;

  void startAp(const SystemConfig& cfg);
  void startSta(const SystemConfig& cfg);
  void startEthernet(const SystemConfig& cfg);
};

} // namespace gh
