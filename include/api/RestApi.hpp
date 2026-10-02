#pragma once
// API REST local (secciones 41-43 / 181) servida por el ESP32.
// Endpoints bajo /api/v1/ y la interfaz web en /.

#include <Arduino.h>
#include <WebServer.h>

#include "config/ConfigManager.hpp"
#include "sensors/SensorManager.hpp"
#include "sensors/SensorRegistry.hpp"
#include "sensors/ModbusProfileRegistry.hpp"
#include "actuators/ActuatorManager.hpp"
#include "actuators/ActuatorRegistry.hpp"
#include "storage/History.hpp"
#include "storage/StorageManager.hpp"
#include "network/NetworkManager.hpp"
#include "network/MqttManager.hpp"
#include "network/WeatherStation.hpp"
#include "control/RuleEngine.hpp"
#include "core/ModuleRegistry.hpp"
#include "hardware/HardwareManager.hpp"
#include "system/HealthMonitor.hpp"
#include "system/BootCounters.hpp"
#include "system/Logger.hpp"

namespace gh {

class RestApi {
public:
  void begin(ConfigManager* cfg, SensorManager* s, ActuatorManager* a, History* h,
             NetworkManager* net = nullptr, MqttManager* mqtt = nullptr);
  void loop() { server_.handleClient(); }
  void setRuleEngine(RuleEngine* r) { rules_ = r; }
  void setWeather(WeatherStation* w) { weather_ = w; }
  // Registros de la plataforma configurable (V8) expuestos por REST.
  void setPlatform(HardwareManager* hw, ModuleRegistry* mod,
                   SensorRegistry* sr, ActuatorRegistry* ar) {
    hardware_ = hw; modules_ = mod; sensorReg_ = sr; actuatorReg_ = ar;
  }
  void setHealth(HealthMonitor* health, BootCounters* boot) {
    health_ = health; boot_ = boot;
  }
  void setStorage(StorageManager* storage) { storage_ = storage; }
  void setModbusProfiles(ModbusProfileRegistry* mp) { modbusProfiles_ = mp; }
  void setLogger(Logger* logger) { logger_ = logger; }
private:
  WebServer server_{80};
  ConfigManager* cfg_ = nullptr;
  SensorManager* sensors_ = nullptr;
  ActuatorManager* actuators_ = nullptr;
  History* history_ = nullptr;
  NetworkManager* network_ = nullptr;
  MqttManager* mqtt_ = nullptr;
  RuleEngine* rules_ = nullptr;
  WeatherStation* weather_ = nullptr;
  HardwareManager* hardware_ = nullptr;
  ModuleRegistry* modules_ = nullptr;
  SensorRegistry* sensorReg_ = nullptr;
  ActuatorRegistry* actuatorReg_ = nullptr;
  HealthMonitor* health_ = nullptr;
  BootCounters* boot_ = nullptr;
  StorageManager* storage_ = nullptr;
  ModbusProfileRegistry* modbusProfiles_ = nullptr;
  Logger* logger_ = nullptr;

  // Sesión de administrador local (token en RAM, expira) — secciones 19/154.
  char sessionToken_[40] = "";
  uint32_t sessionExpires_ = 0;

  void setupRoutes();
  void handleRoot();
  void handleStatus();
  void handleSensors();
  void handleWeather();
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
  void handleConfigExport();
  void handleConfigImport();
  void handleAutomationGet();
  void handleAutomationPost();
  void handleAutomationDelete();
  void handleRs485();
  void handleRs485Scan();
  void handleModbus();
  void handleFirmware();
  void handleOta();
  void handleZones();
  void handleDiagnostics();
  void handleReset();
  void handleRollback();
  void handleLogin();
  void handleModules();
  void handleBuses();
  void handleHardware();
  void handleSensorCatalog();
  void handleActuatorCatalog();
  void handleTokenStatus();
  void handleTokenRotate();
  void handleTokenRevoke();
  void handleHealth();
  void handleBoot();
  void handleStorage();
  void handleModbusProfiles();
  void handleLogs();
  void handlePins();
  void handleDetect();
  bool requireAuth();
  void issueToken();
  String buildStatusJson();
};

} // namespace gh
