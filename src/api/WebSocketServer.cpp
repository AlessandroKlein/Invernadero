#include "api/WebSocketServer.hpp"

#include <ArduinoJson.h>

namespace gh {

void WebSocketServer::begin(ConfigManager* cfg, SensorManager* s, ActuatorManager* a) {
  cfg_ = cfg; sensors_ = s; actuators_ = a;
  ws_.begin();
  ws_.onEvent([](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    // Se pueden procesar comandos entrantes aquí; por ahora solo log.
    if (type == WStype_TEXT) {
      Serial.printf("[WS] cliente %u: %.*s\n", num, (int)length, payload);
    }
  });
}

void WebSocketServer::broadcastState() {
  if (ws_.connectedClients() == 0) return;
  DynamicJsonDocument doc(2048);
  doc["temperature"] = sensors_->temperature();
  doc["humidity"] = sensors_->humidity();
  doc["soil"] = sensors_->soilMoisture(0);
  doc["tank"] = sensors_->tankLevel();
  doc["pump"] = actuators_->isOn(ActuatorRole::PUMP, 0);
  doc["fan"] = actuators_->isOn(ActuatorRole::FAN, 0);
  String out;
  serializeJson(doc, out);
  ws_.broadcastTXT(out.c_str());
}

void WebSocketServer::loop() {
  ws_.loop();
  if (millis() - lastBroadcast_ > 1000) {
    lastBroadcast_ = millis();
    broadcastState();
  }
}

} // namespace gh
