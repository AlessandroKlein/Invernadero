#include "network/NetworkManager.hpp"

namespace gh {

// §253: SSID de AP identificable a partir de los últimos 3 bytes de la MAC.
static String apSsidFromMac() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[24];
  snprintf(buf, sizeof(buf), "INVERNADERO-%06X", (uint32_t)(mac & 0xFFFFFF));
  return String(buf);
}

// MAC para el W5500 derivada de la efuse del ESP32 (bit local-admin puesto).
static void ethMacFromDevice(uint8_t mac[6]) {
  uint64_t m = ESP.getEfuseMac();
  for (int i = 0; i < 6; i++) mac[i] = (uint8_t)(m >> (8 * (5 - i)));
  mac[0] |= 0x02;  // unicast + locally administered
}

void NetworkManager::begin(const SystemConfig& cfg) {
  hostname_ = String(cfg.hostname).length() ? String(cfg.hostname) : "invernadero";

  if (cfg.netInterface == NetInterface::ETHERNET) {
    startEthernet(cfg);
  } else if (strlen(cfg.wifiSsid) > 0) {
    WiFi.mode(WIFI_AP_STA);  // permite STA + AP de respaldo
    startSta(cfg);
  } else {
    WiFi.mode(WIFI_AP_STA);
    startAp(cfg);            // Sin SSID: modo configuración
  }

  // Sincronizar hora (NTP) con la zona horaria configurada (funciona en ambas
  // interfaces, ya que lwIP enruta por la interfaz activa).
  configTime(cfg.timezoneOffset * 3600, 0, cfg.ntpServer);

  // mDNS para acceso por nombre (ej. http://invernadero.local).
  if (MDNS.begin(hostname_.c_str())) {
    MDNS.addService("http", "tcp", 80);
  }
}

void NetworkManager::startAp(const SystemConfig& cfg) {
  apMode_ = true;
  useEthernet_ = false;
  client_ = &wifiClient_;
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
  useEthernet_ = false;
  client_ = &wifiClient_;
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
    startAp(cfg);  // Fallback: AP de configuración
  }
}

void NetworkManager::startEthernet(const SystemConfig& cfg) {
  useEthernet_ = true;
  apMode_ = false;
  WiFi.mode(WIFI_OFF);  // desactiva WiFi para no interferir con el W5500

  Ethernet.init(cfg.ethCsPin);
  uint8_t mac[6];
  ethMacFromDevice(mac);

  if (cfg.ethDhcp || strlen(cfg.ethIp) == 0) {
    Ethernet.begin(mac);  // DHCP
  } else {
    IPAddress ip, gw, mask, dns;
    ip.fromString(cfg.ethIp);
    gw.fromString(cfg.ethGateway);
    mask.fromString(cfg.ethMask);
    dns.fromString(cfg.ethDns);
    Ethernet.begin(mac, ip, dns, gw, mask);  // IP estática
  }

  client_ = &ethClient_;
  Serial.printf("[NET] Ethernet (W5500) CS=%d · link=%s · IP=%s\n", cfg.ethCsPin,
                (Ethernet.linkStatus() == LinkON ? "ON" : "OFF"),
                Ethernet.localIP().toString().c_str());
}

bool NetworkManager::connected() const {
  if (useEthernet_) return Ethernet.linkStatus() == LinkON;
  return WiFi.status() == WL_CONNECTED;
}

String NetworkManager::ip() const {
  return useEthernet_ ? Ethernet.localIP().toString() : WiFi.localIP().toString();
}

void NetworkManager::loop() {
  if (useEthernet_) return;  // el W5500 mantiene el enlace por sí solo
  if (!apMode_ && WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(100);
  }
}

} // namespace gh
