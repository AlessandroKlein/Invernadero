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

void MqttManager::onMessage(const char* topic, const uint8_t* payload, unsigned int len) {
  // El procesamiento de comandos se delega a la API/control (se reenvía el texto).
  char buf[128];
  unsigned int n = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
  memcpy(buf, payload, n);
  buf[n] = '\0';
  Serial.printf("[MQTT] cmd: %s\n", buf);
}

} // namespace gh
