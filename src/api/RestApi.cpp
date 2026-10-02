#include "api/RestApi.hpp"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_system.h>

#include "web/WebAssets.hpp"
#include "system/Device.hpp"
#include "system/Diagnostics.hpp"
#include "core/Version.hpp"

namespace gh {

// Convierte un nombre de rol (string) a su enum.
static ActuatorRole roleFromString(const String& s) {
  if (s == "pump") return ActuatorRole::PUMP;
  if (s == "valve") return ActuatorRole::VALVE;
  if (s == "fan") return ActuatorRole::FAN;
  if (s == "extractor") return ActuatorRole::EXTRACTOR;
  if (s == "heater") return ActuatorRole::HEATER;
  if (s == "humidifier") return ActuatorRole::HUMIDIFIER;
  if (s == "light") return ActuatorRole::LIGHT;
  if (s == "roof") return ActuatorRole::ROOF_OPEN;
  if (s == "window") return ActuatorRole::WINDOW_OPEN;
  if (s == "shade") return ActuatorRole::SHADE_OPEN;
  if (s == "alarm") return ActuatorRole::ALARM;
  return ActuatorRole::GENERIC;
}

void RestApi::begin(ConfigManager* cfg, SensorManager* s, ActuatorManager* a, History* h,
                    NetworkManager* net, MqttManager* mqtt) {
  cfg_ = cfg; sensors_ = s; actuators_ = a; history_ = h;
  network_ = net; mqtt_ = mqtt;
  setupRoutes();
  server_.begin();
}

void RestApi::setupRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/pins", HTTP_GET, [this]() { handlePins(); });
  server_.on("/api/v1/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/v1/sensors", HTTP_GET, [this]() { handleSensors(); });
  server_.on("/api/v1/weather", HTTP_GET, [this]() { handleWeather(); });
  server_.on("/api/v1/actuators", HTTP_GET, [this]() { handleActuators(); });
  server_.on("/api/v1/config", HTTP_GET, [this]() { handleConfigGet(); });
  server_.on("/api/v1/config", HTTP_PUT, [this]() { if (requireAuth()) handleConfigPut(); });
  server_.on("/api/v1/config/export", HTTP_GET, [this]() { handleConfigExport(); });
  server_.on("/api/v1/config/import", HTTP_POST, [this]() { if (requireAuth()) handleConfigImport(); });
  server_.on("/api/v1/automation", HTTP_GET, [this]() { handleAutomationGet(); });
  server_.on("/api/v1/automation", HTTP_POST, [this]() { if (requireAuth()) handleAutomationPost(); });
  server_.on("/api/v1/automation", HTTP_DELETE, [this]() { if (requireAuth()) handleAutomationDelete(); });
  server_.on("/api/v1/events", HTTP_GET, [this]() { handleEvents(); });
  server_.on("/api/v1/alarms", HTTP_GET, [this]() { handleAlarms(); });
  server_.on("/api/v1/actuators", HTTP_POST, [this]() { if (requireAuth()) handleActuatorCommand(); });
  server_.on("/api/v1/factory-reset", HTTP_POST, [this]() { if (requireAuth()) handleFactoryReset(); });

  // Ampliación de la API (sección 181): identidad, capacidades, red, RS485, etc.
  server_.on("/api/v1/device", HTTP_GET, [this]() { handleDevice(); });
  server_.on("/api/v1/capabilities", HTTP_GET, [this]() { handleCapabilities(); });
  server_.on("/api/v1/config/schema", HTTP_GET, [this]() { handleConfigSchema(); });
  server_.on("/api/v1/network", HTTP_GET, [this]() { handleNetwork(); });
  server_.on("/api/v1/network/scan", HTTP_POST, [this]() { if (requireAuth()) handleNetworkScan(); });
  server_.on("/api/v1/rs485", HTTP_GET, [this]() { handleRs485(); });
  server_.on("/api/v1/rs485/scan", HTTP_POST, [this]() { if (requireAuth()) handleRs485Scan(); });
  server_.on("/api/v1/modbus", HTTP_GET, [this]() { handleModbus(); });
  server_.on("/api/v1/firmware", HTTP_GET, [this]() { handleFirmware(); });
  server_.on("/api/v1/ota", HTTP_GET, [this]() { handleOta(); });
  server_.on("/api/v1/zones", HTTP_GET, [this]() { handleZones(); });
  server_.on("/api/v1/diagnostics", HTTP_GET, [this]() { handleDiagnostics(); });
  server_.on("/api/v1/reset", HTTP_POST, [this]() { if (requireAuth()) handleReset(); });
  server_.on("/api/v1/config/rollback", HTTP_POST, [this]() { if (requireAuth()) handleRollback(); });
  server_.on("/api/v1/auth/login", HTTP_POST, [this]() { handleLogin(); });

  // Plataforma configurable (V8): registros y token de API.
  server_.on("/api/v1/modules", HTTP_GET, [this]() { handleModules(); });
  server_.on("/api/v1/buses", HTTP_GET, [this]() { handleBuses(); });
  server_.on("/api/v1/hardware", HTTP_GET, [this]() { handleHardware(); });
  server_.on("/api/v1/sensors/catalog", HTTP_GET, [this]() { handleSensorCatalog(); });
  server_.on("/api/v1/actuators/catalog", HTTP_GET, [this]() { handleActuatorCatalog(); });
  server_.on("/api/v1/token/status", HTTP_GET, [this]() { if (requireAuth()) handleTokenStatus(); });
  server_.on("/api/v1/token/rotate", HTTP_POST, [this]() { if (requireAuth()) handleTokenRotate(); });
  server_.on("/api/v1/token/revoke", HTTP_POST, [this]() { if (requireAuth()) handleTokenRevoke(); });
  server_.on("/api/v1/health", HTTP_GET, [this]() { handleHealth(); });
  server_.on("/api/v1/boot", HTTP_GET, [this]() { handleBoot(); });
  server_.on("/api/v1/storage", HTTP_GET, [this]() { handleStorage(); });
  server_.on("/api/v1/modbus/profiles", HTTP_GET, [this]() { handleModbusProfiles(); });
  server_.on("/api/v1/logs", HTTP_GET, [this]() { handleLogs(); });
}

void RestApi::handleRoot() {
  server_.sendHeader("Cache-Control", "no-cache");
  server_.send(200, "text/html", WebAssets::INDEX_HTML);
}

void RestApi::handlePins() {
  // Página de pines: solo HTML (accesible desde la IP del dispositivo), no JSON.
  server_.sendHeader("Cache-Control", "no-cache");
  server_.send(200, "text/html", WebAssets::PINS_HTML);
}

void RestApi::handleModules() {
  server_.send(200, "application/json", modules_ ? modules_->toJson() : "[]");
}

void RestApi::handleBuses() {
  server_.send(200, "application/json", hardware_ ? hardware_->buses().toJson() : "[]");
}

void RestApi::handleHardware() {
  server_.send(200, "application/json", hardware_ ? hardware_->toJson() : "[]");
}

void RestApi::handleSensorCatalog() {
  server_.send(200, "application/json", sensorReg_ ? sensorReg_->toJson() : "[]");
}

void RestApi::handleActuatorCatalog() {
  server_.send(200, "application/json", actuatorReg_ ? actuatorReg_->toJson() : "[]");
}

void RestApi::handleTokenStatus() {
  String t = cfg_->apiToken();
  DynamicJsonDocument r(256);
  r["configured"] = t.length() > 0;
  r["length"] = (int)t.length();
  String out; serializeJson(r, out);
  server_.send(200, "application/json", out);
}

void RestApi::handleTokenRotate() {
  // Genera un token nuevo y lo devuelve una única vez (no se vuelve a mostrar).
  String t = cfg_->rotateApiToken();
  DynamicJsonDocument r(256);
  r["ok"] = true;
  r["token"] = t;
  String out; serializeJson(r, out);
  server_.send(200, "application/json", out);
}

void RestApi::handleTokenRevoke() {
  cfg_->revokeApiToken();
  server_.send(200, "application/json", "{\"ok\":true}");
}

void RestApi::handleHealth() {
  server_.send(200, "application/json", health_ ? health_->toJson() : "{}");
}

void RestApi::handleBoot() {
  server_.send(200, "application/json", boot_ ? boot_->toJson() : "{}");
}

void RestApi::handleStorage() {
  server_.send(200, "application/json", storage_ ? storage_->toJson() : "{}");
}

void RestApi::handleModbusProfiles() {
  server_.send(200, "application/json", modbusProfiles_ ? modbusProfiles_->toJson() : "{}");
}

void RestApi::handleLogs() {
  server_.send(200, "application/json", logger_ ? logger_->toJson() : "[]");
}

void RestApi::handleStatus() {
  server_.send(200, "application/json", buildStatusJson());
}

void RestApi::handleSensors() {
  server_.send(200, "application/json", sensors_->toJson());
}

void RestApi::handleWeather() {
  if (!weather_) { server_.send(404, "application/json", "{\"error\":\"weather no disponible\"}"); return; }
  server_.send(200, "application/json", weather_->toJson());
}

void RestApi::handleActuators() {
  server_.send(200, "application/json", actuators_->toJson());
}

void RestApi::handleConfigGet() {
  server_.send(200, "application/json", ConfigManager::toJson(cfg_->get()));
}

void RestApi::handleConfigPut() {
  if (!server_.hasArg("plain")) { server_.send(400, "text/plain", "body requerido"); return; }
  SystemConfig cfg = cfg_->get();
  if (!ConfigManager::fromJson(server_.arg("plain"), cfg)) {
    server_.send(400, "text/plain", "JSON inválido");
    return;
  }
  cfg_->set(cfg);
  server_.send(200, "application/json", "{\"ok\":true}");
}

void RestApi::handleEvents() {
  server_.send(200, "application/json", history_->toJson(64));
}

void RestApi::handleAlarms() {
  // Alarmas = eventos con severidad >= 2.
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();
  size_t n = history_->count();
  for (size_t i = 0; i < n && i < 32; i++) {
    LogEvent e = history_->get(n - 1 - i);
    if (e.severity < 2) continue;
    JsonObject o = arr.createNestedObject();
    o["t"] = e.timestamp;
    o["sev"] = e.severity;
    o["msg"] = e.message;
  }
  String out; serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

void RestApi::handleActuatorCommand() {
  // Espera JSON: {"role":"pump","index":0,"output":100} o {"role":"pump","state":true}
  if (!server_.hasArg("plain")) { server_.send(400, "text/plain", "body requerido"); return; }
  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, server_.arg("plain"))) { server_.send(400, "text/plain", "JSON inválido"); return; }
  String role = doc["role"] | "";
  uint8_t index = doc["index"] | 0;
  float output;
  if (doc["output"].is<float>()) output = doc["output"];
  else output = (doc["state"] | false) ? 100.0f : 0.0f;

  ActuatorRole r = roleFromString(role);
  if (r == ActuatorRole::GENERIC) { server_.send(404, "text/plain", "rol desconocido"); return; }
  actuators_->setRequest(r, index, output);
  server_.send(200, "application/json", "{\"ok\":true}");
}

void RestApi::handleFactoryReset() {
  cfg_->factoryReset();
  server_.send(200, "application/json", "{\"ok\":true}");
}

void RestApi::handleDevice() {
  server_.send(200, "application/json", Device::deviceJson(cfg_->get()));
}

void RestApi::handleCapabilities() {
  server_.send(200, "application/json", Device::capabilitiesJson(cfg_->get()));
}

void RestApi::handleConfigSchema() {
  // Esquema descriptivo mínimo de la configuración (sección 181).
  server_.send(200, "application/json",
    "{\"type\":\"object\",\"properties\":{"
    "\"device\":{\"type\":\"object\"},\"features\":{\"type\":\"object\"},"
    "\"climate\":{\"type\":\"object\"},\"irrigation\":{\"type\":\"object\"},"
    "\"ventilation\":{\"type\":\"object\"},\"roof\":{\"type\":\"object\"},"
    "\"lighting\":{\"type\":\"object\"},\"calibration\":{\"type\":\"object\"},"
    "\"network\":{\"type\":\"object\"},\"sensors\":{\"type\":\"object\"},"
    "\"actuators\":{\"type\":\"object\"},\"zones\":{\"type\":\"array\"},"
    "\"config_version\":{\"type\":\"integer\"},"
    "\"configuration_source\":{\"type\":\"string\"},"
    "\"simulation\":{\"type\":\"boolean\"},"
    "\"update_channel\":{\"type\":\"string\"}}}");
}

void RestApi::handleNetwork() {
  SystemConfig c = cfg_->get();
  DynamicJsonDocument doc(1024);
  doc["ssid"] = c.wifiSsid;
  doc["hostname"] = c.hostname;
  doc["ip"] = network_ ? network_->ip() : WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["rssi"] = WiFi.RSSI();
  doc["ap_mode"] = network_ ? network_->isApMode() : false;
  doc["mqtt_host"] = c.mqttHost;
  doc["mqtt_port"] = c.mqttPort;
  doc["ntp"] = c.ntpServer;
  doc["timezone"] = c.timezone;
  doc["dns1"] = c.dnsPrimary;
  doc["dns2"] = c.dnsSecondary;
  doc["managed_by_central"] = c.managedByCentral;
  doc["central_url"] = c.centralUrl;
  doc["rs485_baud"] = c.rs485Baud;
  doc["rs485_parity"] = c.rs485Parity;
  doc["rs485_stop"] = c.rs485StopBits;
  String out; serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

void RestApi::handleRs485() {
  SystemConfig c = cfg_->get();
  ModbusStats s = sensors_->modbusStats();
  DynamicJsonDocument doc(512);
  doc["baud"] = c.rs485Baud;
  doc["parity"] = c.rs485Parity;
  doc["stop_bits"] = c.rs485StopBits;
  doc["tx"] = s.txCount;
  doc["rx"] = s.rxCount;
  doc["crc_errors"] = s.crcErrors;
  doc["timeouts"] = s.timeouts;
  doc["devices_found"] = s.devicesFound;
  String out; serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

void RestApi::handleRs485Scan() {
  // Descubrimiento de dispositivos en el bus (sección 115).
  uint8_t found[64];
  uint8_t n = sensors_->scanModbus(found, 64);
  DynamicJsonDocument doc(512);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < n; i++) arr.add(found[i]);
  String out; serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

void RestApi::handleModbus() {
  // Herramienta Modbus de mantenimiento (sección 178).
  if (!server_.hasArg("slave") || !server_.hasArg("reg")) {
    server_.send(400, "text/plain", "faltan slave/reg");
    return;
  }
  uint8_t slave = server_.arg("slave").toInt();
  uint16_t reg = server_.arg("reg").toInt();
  uint16_t qty = server_.hasArg("qty") ? server_.arg("qty").toInt() : 1;
  uint8_t func = server_.hasArg("func") ? server_.arg("func").toInt() : 3;
  if (qty == 0 || qty > 10) qty = 1;
  uint16_t vals[10];
  bool ok = (func == 4) ? sensors_->modbusReadInput(slave, reg, qty, vals)
                        : sensors_->modbusReadHolding(slave, reg, qty, vals);
  DynamicJsonDocument doc(512);
  doc["ok"] = ok;
  JsonArray arr = doc.createNestedArray("registers");
  for (uint16_t i = 0; i < (ok ? qty : 0); i++) {
    JsonObject r = arr.createNestedObject();
    r["addr"] = reg + i;
    r["value"] = vals[i];
  }
  String out; serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

void RestApi::handleFirmware() {
  SystemConfig c = cfg_->get();
  DynamicJsonDocument doc(512);
  doc["project"] = "Invernadero";
  doc["version"] = GH_FW_VERSION;
  doc["channel"] = updateChannelString(c.updateChannel);
  doc["hardware_profile"] = GH_HW_PROFILE;
  doc["config_schema"] = GH_CONFIG_SCHEMA_VERSION;
  doc["protocol"] = GH_PROTOCOL_VERSION;
  String out; serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

void RestApi::handleOta() {
  SystemConfig c = cfg_->get();
  DynamicJsonDocument doc(256);
  doc["ota_enabled"] = true;
  doc["channel"] = updateChannelString(c.updateChannel);
  doc["version"] = GH_FW_VERSION;
  String out; serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

void RestApi::handleZones() {
  SystemConfig c = cfg_->get();
  DynamicJsonDocument doc(512);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < c.zoneCount && i < 8; i++) arr.add(c.zoneNames[i]);
  String out; serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

void RestApi::handleDiagnostics() {
  bool wifiOk = network_ ? network_->connected() : (WiFi.status() == WL_CONNECTED);
  String ip = network_ ? network_->ip() : WiFi.localIP().toString();
  server_.send(200, "application/json",
    Diagnostics::build(sensors_, mqtt_, wifiOk, ip.c_str(), GH_FW_VERSION, GH_HW_VERSION));
}

void RestApi::handleReset() {
  // Niveles de reset (sección 155): network | automation | factory.
  if (!server_.hasArg("plain")) { server_.send(400, "text/plain", "body requerido"); return; }
  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, server_.arg("plain"))) { server_.send(400, "text/plain", "JSON inválido"); return; }
  String level = doc["level"] | "factory";
  if (level == "network") cfg_->resetNetwork();
  else if (level == "automation") cfg_->resetAutomation();
  else cfg_->factoryReset();
  server_.send(200, "application/json", "{\"ok\":true}");
}

void RestApi::handleRollback() {
  // Rollback de configuración (sección 104).
  cfg_->rollback();
  server_.send(200, "application/json", "{\"ok\":true}");
}

void RestApi::handleNetworkScan() {
  // Escaneo WiFi (§107): devuelve SSID, RSSI, canal y tipo de seguridad.
  int n = WiFi.scanNetworks();
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.createNestedArray("networks");
  for (int i = 0; i < n && i < 30; i++) {
    JsonObject o = arr.createNestedObject();
    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
    o["channel"] = WiFi.channel(i);
    o["encryption"] = (uint8_t)WiFi.encryptionType(i);
    o["open"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
  }
  doc["count"] = n;
  String out; serializeJson(doc, out);
  WiFi.scanDelete();
  server_.send(200, "application/json", out);
}

void RestApi::handleConfigExport() {
  // Exportar configuración completa (§258) con schema_version (§221).
  SystemConfig c = cfg_->get();
  DynamicJsonDocument cfgDoc(8192);
  deserializeJson(cfgDoc, ConfigManager::toJson(c));
  DynamicJsonDocument outDoc(12288);
  outDoc["schema_version"] = GH_CONFIG_SCHEMA_VERSION;
  outDoc["config_version"] = c.configVersion;
  outDoc["config"] = cfgDoc.as<JsonObject>();
  String out; serializeJson(outDoc, out);
  server_.sendHeader("Content-Disposition", "attachment; filename=\"invernadero-config.json\"");
  server_.send(200, "application/json", out);
}

void RestApi::handleConfigImport() {
  // Importar configuración (§257): valida, aplica y persiste (mantiene previo para rollback).
  if (!server_.hasArg("plain")) { server_.send(400, "text/plain", "body requerido"); return; }
  DynamicJsonDocument doc(12288);
  if (deserializeJson(doc, server_.arg("plain"))) {
    server_.send(400, "application/json", "{\"error\":\"JSON inválido\"}");
    return;
  }
  // Acepta un export completo ({config:{...}}) o la configuración cruda.
  String cfgJson;
  if (doc["config"].is<JsonObject>()) {
    serializeJson(doc["config"], cfgJson);
  } else {
    cfgJson = server_.arg("plain");
  }
  SystemConfig c;
  if (!ConfigManager::fromJson(cfgJson, c)) {
    server_.send(400, "application/json", "{\"error\":\"Configuración inválida\"}");
    return;
  }
  cfg_->set(c); // versiona y persiste (rollback previo)
  SystemConfig applied = cfg_->get();
  DynamicJsonDocument r(128);
  r["ok"] = true;
  r["config_version"] = applied.configVersion;
  String out; serializeJson(r, out);
  server_.send(200, "application/json", out);
}

void RestApi::handleAutomationGet() {
  if (!rules_) { server_.send(404, "application/json", "{\"rules\":[]}"); return; }
  server_.send(200, "application/json", rules_->toJson());
}

void RestApi::handleAutomationPost() {
  // Crear/agregar una regla (sección 237). Las reglas viven en RAM (no persisten
  // en NVS aún); se suman a los controladores fijos.
  if (!rules_) { server_.send(503, "application/json", "{\"error\":\"rule engine no disponible\"}"); return; }
  if (!server_.hasArg("plain")) { server_.send(400, "text/plain", "body requerido"); return; }
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, server_.arg("plain"))) { server_.send(400, "text/plain", "JSON inválido"); return; }

  AutomationRule r;
  if (doc["name"].is<const char*>()) strncpy(r.name, doc["name"], sizeof(r.name) - 1);
  r.enabled = doc["enabled"] | true;
  RuleVariable var; RuleOp op;
  if (!ruleVariableFromString(doc["variable"] | "temperature", var) ||
      !ruleOpFromString(doc["op"] | "gt", op)) {
    server_.send(400, "application/json", "{\"error\":\"variable/op inválidos\"}");
    return;
  }
  r.variable = var;
  r.op = op;
  r.threshold = doc["threshold"] | 0.0f;
  r.action = roleFromString(doc["action"] | "fan");
  r.actionIndex = doc["action_index"] | 0;
  r.zone = doc["zone"] | 0;
  r.actionValue = doc["action_value"] | 100.0f;

  if (!rules_->addRule(r)) {
    server_.send(409, "application/json", "{\"error\":\"límite de reglas alcanzado\"}");
    return;
  }
  server_.send(200, "application/json", "{\"ok\":true,\"count\":" + String(rules_->count()) + "}");
}

void RestApi::handleAutomationDelete() {
  if (!rules_) { server_.send(404, "application/json", "{\"error\":\"rule engine no disponible\"}"); return; }
  rules_->clear();
  server_.send(200, "application/json", "{\"ok\":true,\"count\":0}");
}

void RestApi::handleLogin() {
  if (!cfg_) { server_.send(503, "application/json", "{\"error\":\"config no disponible\"}"); return; }
  if (!server_.hasArg("plain")) { server_.send(400, "application/json", "{\"error\":\"body requerido\"}"); return; }
  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, server_.arg("plain"))) { server_.send(400, "text/plain", "JSON inválido"); return; }
  String user = doc["user"] | "";
  String pass = doc["pass"] | "";
  if (!cfg_->auth(user, pass)) {
    server_.send(401, "application/json", "{\"error\":\"credenciales inválidas\"}");
    return;
  }
  issueToken();
  DynamicJsonDocument r(256);
  r["ok"] = true;
  r["token"] = sessionToken_;
  r["user"] = cfg_->adminUser();
  String out;
  serializeJson(r, out);
  server_.send(200, "application/json", out);
}

void RestApi::issueToken() {
  // Token de sesión en RAM con expiración (sección 154). No es persistente.
  snprintf(sessionToken_, sizeof(sessionToken_), "%08lX%08X",
           (unsigned long)millis(), (unsigned)esp_random());
  sessionExpires_ = millis() + 3600UL * 1000UL; // 1 hora
}

bool RestApi::requireAuth() {
  // Verifica el token de sesión (admin local) o el token de API persistente.
  // Acepta "Authorization: Bearer <token>" o el header "X-Auth-Token: <token>".
  String token = server_.header("X-Auth-Token");
  if (token.length() == 0) {
    String auth = server_.header("Authorization");
    if (auth.startsWith("Bearer ")) token = auth.substring(7);
  }
  if (token.length() > 0 && sessionToken_[0] != '\0' &&
      token == String(sessionToken_) && millis() < sessionExpires_) {
    return true;
  }
  // Token de API persistente: autoriza control/modificación desde el servidor
  // central sin depender de la sesión interactiva del administrador.
  if (cfg_ && cfg_->validateApiToken(token)) return true;
  server_.send(401, "application/json", "{\"error\":\"unauthorized\"}");
  return false;
}

String RestApi::buildStatusJson() {
  SystemConfig cfg = cfg_->get();
  DynamicJsonDocument doc(2048);
  doc["id"] = cfg.deviceId;
  doc["name"] = cfg.deviceName;
  doc["mode"] = "AUTO";
  doc["state"] = deviceStateString(Device::state());
  doc["config_version"] = cfg.configVersion;
  doc["simulation"] = cfg.simulation;
  doc["temperature"] = sensors_->temperature();
  doc["humidity"] = sensors_->humidity();
  doc["soil"] = sensors_->soilMoisture(0);
  doc["tank"] = sensors_->tankLevel();
  doc["light"] = sensors_->lightLux();
  doc["vpd"] = sensors_->vpd();
  doc["dewpoint"] = sensors_->dewPoint();
  if (weather_ && weather_->available()) {
    doc["weather"] = true;
    doc["ext_temp"] = weather_->temperature();
    doc["ext_hum"] = weather_->humidity();
    doc["wind"] = weather_->windSpeed();
    doc["rain"] = weather_->rain();
    doc["pressure"] = weather_->pressure();
  } else {
    doc["weather"] = false;
  }
  doc["pump"] = actuators_->isOn(ActuatorRole::PUMP, 0);
  doc["fan"] = actuators_->isOn(ActuatorRole::FAN, 0);
  String out; serializeJson(doc, out);
  return out;
}

} // namespace gh
