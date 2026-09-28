#include "api/RestApi.hpp"

#include <ArduinoJson.h>
#include "web/WebAssets.hpp"

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

void RestApi::begin(ConfigManager* cfg, SensorManager* s, ActuatorManager* a, History* h) {
  cfg_ = cfg; sensors_ = s; actuators_ = a; history_ = h;
  setupRoutes();
  server_.begin();
}

void RestApi::setupRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/api/v1/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/v1/sensors", HTTP_GET, [this]() { handleSensors(); });
  server_.on("/api/v1/actuators", HTTP_GET, [this]() { handleActuators(); });
  server_.on("/api/v1/config", HTTP_GET, [this]() { handleConfigGet(); });
  server_.on("/api/v1/config", HTTP_PUT, [this]() { handleConfigPut(); });
  server_.on("/api/v1/events", HTTP_GET, [this]() { handleEvents(); });
  server_.on("/api/v1/alarms", HTTP_GET, [this]() { handleAlarms(); });
  server_.on("/api/v1/actuators", HTTP_POST, [this]() { handleActuatorCommand(); });
  server_.on("/api/v1/factory-reset", HTTP_POST, [this]() { handleFactoryReset(); });
}

void RestApi::handleRoot() {
  server_.sendHeader("Cache-Control", "no-cache");
  server_.send(200, "text/html", WebAssets::INDEX_HTML);
}

void RestApi::handleStatus() {
  server_.send(200, "application/json", buildStatusJson());
}

void RestApi::handleSensors() {
  server_.send(200, "application/json", sensors_->toJson());
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

String RestApi::buildStatusJson() {
  SystemConfig cfg = cfg_->get();
  DynamicJsonDocument doc(2048);
  doc["id"] = cfg.deviceId;
  doc["name"] = cfg.deviceName;
  doc["mode"] = "AUTO";
  doc["temperature"] = sensors_->temperature();
  doc["humidity"] = sensors_->humidity();
  doc["soil"] = sensors_->soilMoisture(0);
  doc["tank"] = sensors_->tankLevel();
  doc["light"] = sensors_->lightLux();
  doc["pump"] = actuators_->isOn(ActuatorRole::PUMP, 0);
  doc["fan"] = actuators_->isOn(ActuatorRole::FAN, 0);
  String out; serializeJson(doc, out);
  return out;
}

} // namespace gh
