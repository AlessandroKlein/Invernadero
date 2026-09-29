#pragma once
// Comunicación MQTT con el servidor central (secciones 45/74).
// Publica estado/sensores/actuadores/eventos/alarmas y recibe comandos.

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "core/Types.hpp"

namespace gh {

class MqttManager {
public:
  void begin(const SystemConfig& cfg);
  void loop();                                   // Reconexión + suscripción
  bool enabled() const { return enabled_; }
  bool connected() { return enabled_ && mqtt_.connected(); }
  void publishSensors(const String& json);
  void publishStatus(const String& json);
  void publishActuators(const String& json);
  void publishWeather(const String& json);

  // Callback de comandos (topic .../cmd).
  void onMessage(const char* topic, const uint8_t* payload, unsigned int len);

  // Consume el último comando recibido (se limpia tras leerlo). Usado por el
  // loop principal para disparar acciones como OTA desde el servidor central.
  bool consumeCommand(String& out);

private:
  WiFiClient client_;
  PubSubClient mqtt_{client_};
  bool enabled_ = false;
  String baseTopic_ = "greenhouse/GH001";
  SystemConfig cfg_;
  String pendingCmd_;

  void reconnect();
};

} // namespace gh
