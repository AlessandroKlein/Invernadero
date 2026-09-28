#include "network/NetworkManager.hpp"

namespace gh {

void NetworkManager::begin(const SystemConfig& cfg) {
  hostname_ = String(cfg.hostname).length() ? String(cfg.hostname) : "invernadero";
  WiFi.mode(WIFI_AP_STA); // permite AP y STA simultáneos

  // Sincronizar hora (NTP) con la zona horaria configurada.
  configTime(cfg.timezoneOffset * 3600, 0, cfg.ntpServer);

  if (strlen(cfg.wifiSsid) > 0) {
    startSta(cfg);
  } else {
    startAp(cfg); // Sin SSID: modo configuración
  }

  // mDNS para acceso por nombre (ej. http://invernadero.local).
  if (MDNS.begin(hostname_.c_str())) {
    MDNS.addService("http", "tcp", 80);
  }
}

void NetworkManager::startAp(const SystemConfig& cfg) {
  apMode_ = true;
  WiFi.softAP(cfg.apSsid, cfg.apPass);
  delay(200);
  Serial.printf("[NET] AP: %s\n", cfg.apSsid);
}

void NetworkManager::startSta(const SystemConfig& cfg) {
  apMode_ = false;
  WiFi.begin(cfg.wifiSsid, cfg.wifiPass);
  Serial.printf("[NET] Conectando a %s...\n", cfg.wifiSsid);
  // Intentar conectar hasta 15 s; si falla, activar AP de respaldo.
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
    delay(200);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[NET] Conectado: %s\n", WiFi.localIP().toString().c_str());
  } else {
    // Fallback: AP de configuración.
    startAp(cfg);
  }
}

void NetworkManager::loop() {
  // Reconectar STA si se cayó y hay credenciales.
  if (!apMode_ && WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(100);
  }
}

} // namespace gh
