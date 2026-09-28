#pragma once
// API REST local (secciones 41-43 / 181) servida por el ESP32.
// Endpoints bajo /api/v1/ y la interfaz web en /.

#include <Arduino.h>
#include <WebServer.h>

#include "config/ConfigManager.hpp"
#include "sensors/SensorManager.hpp"
#include "actuators/ActuatorManager.hpp"
#include "storage/History.hpp"
#include "network/NetworkManager.hpp"
#include "network/MqttManager.hpp"

namespace gh {

class RestApi {
public:
  void begin(ConfigManager* cfg, SensorManager* s, ActuatorManager* a, History* h,
             NetworkManager* net = nullptr, MqttManager* mqtt = nullptr);
  void loop() { server_.handleClient(); }

private:
  WebServer server_{80};
  ConfigManager* cfg_ = nullptr;
  SensorManager* sensors_ = nullptr;
  ActuatorManager* actuators_ = nullptr;
  History* history_ = nullptr;
  NetworkManager* network_ = nullptr;
  MqttManager* mqtt_ = nullptr;

  void setupRoutes();
  void handleRoot();
  void handleStatus();
  void handleSensors();
  void handleActuators();
  void handleConfigGet();
  void handleConfigPut();
  void handleConfigSchema();
  void handleEvents();
  void handleAlarms();
  void handleActuatorCommand();
  void handleFactoryReset();
  void handleDevice();
  void handleCapabilities();
  void handleNetwork();
  void handleNetworkScan();
  void handleRs485();
  void handleRs485Scan();
  void handleModbus();
  void handleFirmware();
  void handleOta();
  void handleZones();
  void handleDiagnostics();
  void handleReset();
  void handleRollback();
  String buildStatusJson();
};

} // namespace gh
