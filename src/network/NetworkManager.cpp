#include "network/NetworkManager.hpp"

namespace gh {

// §253: SSID de AP identificable a partir de los últimos 3 bytes de la MAC.
static String apSsidFromMac() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[24];
  snprintf(buf, sizeof(buf), "INVERNADERO-%06X", (uint32_t)(mac & 0xFFFFFF));
  return String(buf);
}

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
  // §253: si el SSID es el default legado (o vacío), derivar de la MAC.
  String ssid = (strcmp(cfg.apSsid, "Invernadero-AP") == 0 || strlen(cfg.apSsid) == 0)
                    ? apSsidFromMac()
                    : String(cfg.apSsid);
  WiFi.softAP(ssid.c_str(), cfg.apPass);
  delay(200);
  Serial.printf("[NET] AP: %s\n", ssid.c_str());
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
