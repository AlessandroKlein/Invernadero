#pragma once
// API REST local (secciones 41-43) servida por el ESP32.
// Endpoints bajo /api/v1/ y la interfaz web en /.

#include <Arduino.h>
#include <WebServer.h>

#include "config/ConfigManager.hpp"
#include "sensors/SensorManager.hpp"
#include "actuators/ActuatorManager.hpp"
#include "storage/History.hpp"

namespace gh {

class RestApi {
public:
  void begin(ConfigManager* cfg, SensorManager* s, ActuatorManager* a, History* h);
  void loop() { server_.handleClient(); }

private:
  WebServer server_{80};
  ConfigManager* cfg_ = nullptr;
  SensorManager* sensors_ = nullptr;
  ActuatorManager* actuators_ = nullptr;
  History* history_ = nullptr;

  void setupRoutes();
  void handleRoot();
  void handleStatus();
  void handleSensors();
  void handleActuators();
  void handleConfigGet();
  void handleConfigPut();
  void handleEvents();
  void handleAlarms();
  void handleActuatorCommand();
  void handleFactoryReset();
  String buildStatusJson();
};

} // namespace gh
