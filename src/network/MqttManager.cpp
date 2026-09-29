#include "network/MqttManager.hpp"

namespace gh {

void MqttManager::begin(const SystemConfig& cfg) {
  cfg_ = cfg;
  baseTopic_ = "greenhouse/" + String(cfg.deviceId);
  enabled_ = (strlen(cfg.mqttHost) > 0);
  if (!enabled_) return;

  mqtt_.setServer(cfg.mqttHost, cfg.mqttPort);
  // Callback vía lambda (esta clase es la única instancia MQTT).
  mqtt_.setCallback([this](char* topic, uint8_t* payload, unsigned int len) {
    this->onMessage(topic, payload, len);
  });
  reconnect();
}

void MqttManager::reconnect() {
  if (!enabled_ || !WiFi.isConnected()) return;
  String id = String(cfg_.deviceId);
  bool ok = false;
  if (strlen(cfg_.mqttUser) > 0) {
    ok = mqtt_.connect(id.c_str(), cfg_.mqttUser, cfg_.mqttPass);
  } else {
    ok = mqtt_.connect(id.c_str());
  }
  if (ok) {
    String cmd = baseTopic_ + "/cmd";
    mqtt_.subscribe(cmd.c_str());
  }
}

void MqttManager::loop() {
  if (!enabled_) return;
  if (!mqtt_.connected()) {
    static uint32_t lastTry = 0;
    if (millis() - lastTry > 5000) { lastTry = millis(); reconnect(); }
  } else {
    mqtt_.loop();
  }
}

void MqttManager::publishSensors(const String& json) {
  if (connected()) mqtt_.publish((baseTopic_ + "/sensors").c_str(), json.c_str());
}

void MqttManager::publishStatus(const String& json) {
  if (connected()) mqtt_.publish((baseTopic_ + "/state").c_str(), json.c_str());
}

void MqttManager::publishActuators(const String& json) {
  if (connected()) mqtt_.publish((baseTopic_ + "/actuators").c_str(), json.c_str());
}

void MqttManager::publishWeather(const String& json) {
  if (connected()) mqtt_.publish((baseTopic_ + "/weather").c_str(), json.c_str());
}

void MqttManager::onMessage(const char* topic, const uint8_t* payload, unsigned int len) {
  // El procesamiento de comandos se delega al loop principal vía consumeCommand().
  pendingCmd_ = String((const char*)payload).substring(0, len);
  Serial.printf("[MQTT] cmd: %s\n", pendingCmd_.c_str());
}

bool MqttManager::consumeCommand(String& out) {
  if (pendingCmd_.length() == 0) return false;
  out = pendingCmd_;
  pendingCmd_ = "";
  return true;
}

} // namespace gh
