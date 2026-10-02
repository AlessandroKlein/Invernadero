// PROYECTO INTEGRAL DE AUTOMATIZACIÓN MODULAR DE INVERNADERO BASADO EN ESP32
// Punto de entrada: inicializa todos los módulos y lanza la tarea de automatización.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "core/Types.hpp"
#include "core/PinConfig.hpp"
#include "core/Version.hpp"
#include "core/PlatformTypes.hpp"
#include "core/CapabilityRegistry.hpp"
#include "core/ModuleRegistry.hpp"
#include "core/EventBus.hpp"
#include "core/Scheduler.hpp"

#include "config/ConfigManager.hpp"
#include "storage/History.hpp"
#include "storage/StorageManager.hpp"

#include "hardware/ShiftRegister595.hpp"
#include "hardware/Mcp23017.hpp"
#include "hardware/BusManager.hpp"
#include "hardware/HardwareManager.hpp"

#include "sensors/SensorManager.hpp"
#include "sensors/SensorRegistry.hpp"
#include "sensors/ModbusProfileRegistry.hpp"
#include "sensors/ModbusGateway.hpp"
#include "actuators/ActuatorManager.hpp"
#include "actuators/ActuatorRegistry.hpp"

#include "control/ClimateController.hpp"
#include "control/IrrigationController.hpp"
#include "control/LightingController.hpp"
#include "control/RoofController.hpp"
#include "control/SafetyController.hpp"
#include "control/RuleEngine.hpp"

#include "network/NetworkManager.hpp"
#include "network/MqttManager.hpp"
#include "network/WeatherStation.hpp"

#include "api/RestApi.hpp"
#include "api/WebSocketServer.hpp"

#include "system/Watchdog.hpp"
#include "system/OtaManager.hpp"
#include "system/Diagnostics.hpp"
#include "system/Device.hpp"
#include "system/HealthMonitor.hpp"
#include "system/BootCounters.hpp"
#include "system/Logger.hpp"

using namespace gh;

// Contexto compartido entre setup/loop y la tarea de automatización.
struct App {
  ConfigManager config;
  History history;
  // Plataforma configurable (V8): buses/hardware y registros dinámicos.
  CapabilityRegistry capabilities;
  ModuleRegistry modules;
  HardwareManager hardware;
  SensorRegistry sensorRegistry;
  ActuatorRegistry actuatorRegistry;
  ModbusProfileRegistry modbusProfiles;
  ModbusGateway modbusGateway;
  PinConfigManager pinConfigMgr;
  PinConfig pinConfig;
  StorageManager storage;
  ShiftRegister595 shift;
  Mcp23017 mcpPool[4];  // pool de expansores I²C instanciados desde el catálogo
  SensorManager sensors;
  ActuatorManager actuators;
  ClimateController climate;
  IrrigationController irrigation;
  LightingController lighting;
  RoofController roof;
  SafetyController safety;
  RuleEngine rules;
  NetworkManager network;
  MqttManager mqtt;
  WeatherStation weather;
  RestApi api;
  WebSocketServer ws;
  Watchdog watchdog;
  HealthMonitor health;
  BootCounters boot;
  Logger logger;
  EventBus events;
  Scheduler scheduler;
  OtaManager ota;
};
static App app;

// Semáforo sensor→control: el control solo corre tras una lectura fresca.
static SemaphoreHandle_t sensorReady = nullptr;

// SensorTask (núcleo 0, prioridad alta): adquisición de sensores. Los accesores
// del SensorManager ahora están protegidos, por lo que ControlTask puede leerlos
// en paralelo sin carrera de datos (SEMA §202-203 / docs/MEJORAS.md §2).
static void sensorTask(void* arg) {
  (void)arg;
  app.watchdog.subscribe();
  app.health.registerTask("sensors", xTaskGetCurrentTaskHandle());
  for (;;) {
    app.sensors.update();          // Leer sensores habilitados (escribe slots protegidos)
    app.health.touch("sensors");
    app.watchdog.feed();
    xSemaphoreGive(sensorReady);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ControlTask (núcleo 0): seguridad + controladores + reglas + salidas.
static void controlTask(void* arg) {
  (void)arg;
  app.watchdog.subscribe();
  app.health.registerTask("control", xTaskGetCurrentTaskHandle());
  for (;;) {
    // Espera una lectura fresca (hasta 3 s). Si no llega, corre igual con lo
    // último disponible para no detener nunca la seguridad.
    xSemaphoreTake(sensorReady, pdMS_TO_TICKS(3000));

    SystemConfig cfg = app.config.get();
    app.safety.update(cfg);        // Seguridad (prioridad máxima)
    app.climate.update(cfg);       // Clima
    app.irrigation.update(cfg);    // Riego
    app.lighting.update(cfg);      // Iluminación
    app.roof.update(cfg);          // Techo/ventanas
    app.rules.update();            // Reglas configurables (se suman a los controladores)
    app.actuators.apply();         // Escribir salidas físicas

    app.health.touch("control");
    app.watchdog.feed();
  }
}

// ¿Hay red disponible? (WiFi o Ethernet, según la interfaz activa).
static bool isNetUp() { return app.network.connected(); }

// Publicación MQTT periódica (Scheduler, SEMA §206). Se ejecuta cada 10 s.
static void mqttPublishTask(void* ctx) {
  (void)ctx;
  if (app.mqtt.connected()) {
    app.mqtt.publishSensors(app.sensors.toJson());
    app.mqtt.publishActuators(app.actuators.toJson());
    if (app.weather.enabled()) app.mqtt.publishWeather(app.weather.toJson());
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Device::setState(DeviceState::BOOTING);
  Serial.printf("\n[BOOT] Invernadero %s (hw %s)\n", GH_FW_VERSION, GH_HW_VERSION);
  Serial.printf("[BOOT] UID %s | Reinicio: %s\n", Device::uid(), resetCauseString(Device::resetCause()));

  // 1) Configuración (NVS + JSON).
  app.config.begin();
  Device::setState(DeviceState::INITIALIZING);

  // 1.5) Salud por tareas, contadores, logger y bus de eventos (SEMA §132-134).
  app.boot.begin();  // registra arranque + causa del último reinicio
  app.health.begin();
  app.health.registerTask("loop", xTaskGetCurrentTaskHandle());
  app.logger.begin();
  app.events.begin();

  // 2) Historial y almacenamiento (LittleFS/SPIFFS).
  app.history.begin(128);
  app.storage.begin();
  app.logger.info("boot", "Sistema iniciando");
  app.events.publish(EventType::SYSTEM_BOOT, "boot");
  app.logger.info("storage", app.storage.mounted() ? "Filesystem montado" : "Filesystem no disponible");

  // 2.5) Plataforma configurable (V8): capacidades, módulos y catálogos.
  {
    SystemConfig pc = app.config.get();
    app.capabilities.syncFrom(Device::info(pc));
    app.modules.registerBuiltins();
    app.sensorRegistry.buildFromConfig(pc);
    app.sensorRegistry.load();  // aplica ediciones del catálogo guardadas en NVS
    app.actuatorRegistry.buildFromConfig(pc);

    // Perfiles Modbus de ejemplo (V9): pH y EC genéricos por RS485.
    ModbusProfile p;
    strncpy(p.id, "ph-generic", sizeof(p.id) - 1);
    strncpy(p.vendor, "generic", sizeof(p.vendor) - 1);
    p.slaveId = 1;
    p.registerAddress = 0;
    p.dataType = ModbusDataType::UINT16;
    p.scale = 0.01f;
    strncpy(p.unit, "pH", sizeof(p.unit) - 1);
    p.magnitude = SensorType::PH;
    app.modbusProfiles.registerProfile(p);
    strncpy(p.id, "ec-generic", sizeof(p.id) - 1);
    p.scale = 0.001f;
    strncpy(p.unit, "mS/cm", sizeof(p.unit) - 1);
    p.magnitude = SensorType::EC;
    app.modbusProfiles.registerProfile(p);

    Serial.printf("[BOOT] Capacidades: %u | Módulos: %u | Sensores: %u | Actuadores: %u | Perfiles Modbus: %u\n",
                  app.capabilities.count(), app.modules.count(),
                  app.sensorRegistry.count(), app.actuatorRegistry.count(),
                  app.modbusProfiles.profileCount());
  }

  // 3) Hardware de salida: 74HC595 (SPI) + MCP23017 (I²C), con pines de NVS.
  app.pinConfigMgr.begin();
  app.pinConfig = app.pinConfigMgr.get();
  app.shift.begin(app.pinConfig.hc595Mosi, app.pinConfig.hc595Sclk, app.pinConfig.hc595Latch, app.pinConfig.hc595Count);
  app.shift.allOff(); // estado seguro al arrancar (sección 21.14)
  // Soft-PWM deshabilitado por defecto (modo digital ON/OFF seguro). Para PWM
  // de alta frecuencia se recomienda LEDC en GPIO o controlador dedicado.
  // app.shift.setPwmEnabled(true, 200, 8);
  app.hardware.begin(app.pinConfig); // inicia los buses (I²C) y registra los nodos de hardware
  app.hardware.load(); // aplica ediciones del catálogo de expansores (NVS)

  // Pool de expansores MCP23017 (I²C) instanciados desde el catálogo (paso 3):
  // cada nodo MCP23017 habilitado ocupa un slot; el canal 0 queda en mcpPool[0]
  // (usado por ActuatorManager). Los nodos se agregan/editan por PUT /api/v1/hardware.
  {
    HardwareNode hwNodes[HardwareManager::MAX_NODES];
    size_t hwN = 0;
    app.hardware.snapshot(hwNodes, HardwareManager::MAX_NODES, hwN);
    uint8_t mcpCount = 0;
    for (size_t i = 0; i < hwN && mcpCount < 4; i++) {
      if (hwNodes[i].kind == HardwareKind::MCP23017 && hwNodes[i].enabled) {
        app.mcpPool[mcpCount++].begin(hwNodes[i].address, &Wire);
      }
    }
  }

  // 4) Sensores y actuadores.
  app.sensors.begin(app.config.get(), app.pinConfig, &app.sensorRegistry);
  app.actuators.begin(app.config.get(), &app.shift, &app.mcpPool[0]);
  app.actuators.allSafeState();
  Device::setState(DeviceState::SELF_TEST);

  // 5) Controladores.
  app.climate.begin(&app.sensors, &app.actuators, &app.history);
  app.irrigation.begin(&app.sensors, &app.actuators, &app.history);
  app.lighting.begin(&app.sensors, &app.actuators);
  app.roof.begin(&app.sensors, &app.actuators);
  app.safety.begin(&app.sensors, &app.actuators, &app.history);
  app.rules.begin(&app.sensors, &app.actuators, &app.history);

  // 5.5) Gateway RS485/Modbus (polling multi-esclavo por perfiles, V9).
  app.modbusGateway.begin(app.sensors.modbusRtu(), &app.modbusProfiles);
  app.modbusGateway.rebuild();

  // 6) Red, MQTT, API, WebSocket, OTA, Watchdog.
  app.network.begin(app.config.get());
  Device::setState(DeviceState::NETWORK);
  app.weather.begin(app.config.get());
  app.mqtt.begin(app.config.get(), app.network.client(), isNetUp);
  app.scheduler.add("mqtt_publish", 10000, mqttPublishTask, nullptr);
  app.api.begin(&app.config, &app.sensors, &app.actuators, &app.history,
                &app.network, &app.mqtt);
  app.api.setRuleEngine(&app.rules);
  app.api.setWeather(&app.weather);
  app.api.setPlatform(&app.hardware, &app.modules, &app.sensorRegistry, &app.actuatorRegistry);
  app.api.setHealth(&app.health, &app.boot);
  app.api.setStorage(&app.storage);
  app.api.setModbusProfiles(&app.modbusProfiles);
  app.api.setModbusGateway(&app.modbusGateway);
  app.api.setPinConfigManager(&app.pinConfigMgr);
  app.api.setLogger(&app.logger);
  app.ws.begin(&app.config, &app.sensors, &app.actuators);
  app.ota.begin(app.config.get().hostname);
  app.watchdog.begin(30);

  // 7) Lanzar tareas del núcleo 0: adquisición (prioridad alta) + control.
  sensorReady = xSemaphoreCreateBinary();
  xTaskCreatePinnedToCore(sensorTask, "sensors", 8192, nullptr, 3, nullptr, 0);
  xTaskCreatePinnedToCore(controlTask, "control", 8192, nullptr, 2, nullptr, 0);

  app.history.add(0, "Sistema iniciado");
  Device::setState(DeviceState::RUN);
  Serial.println("[BOOT] Listo");
}

void loop() {
  app.network.loop();
  app.weather.loop();
  app.mqtt.loop();
  app.api.loop();
  app.ws.loop();
  app.ota.loop();
  app.watchdog.feed();
  app.health.touch("loop");

  // Procesar comandos MQTT (p. ej. OTA remota desde el servidor central).
  String cmd;
  if (app.mqtt.consumeCommand(cmd)) {
    DynamicJsonDocument cdoc(1024);
    if (deserializeJson(cdoc, cmd) == DeserializationError::Ok) {
      String type = cdoc["type"] | "";
      if (type == "ota") {
        String url = cdoc["url"] | "";
        String sha = cdoc["sha256"] | "";
        if (url.length()) app.ota.applyFromUrl(url, sha, app.network.client());
      }
    }
  }

  // Publicación MQTT periódica vía Scheduler, gateway RS485 y drenaje de eventos.
  app.scheduler.tick();
  app.modbusGateway.tick();
  Event ev;
  if (app.events.poll(ev)) {
    app.logger.info("event", eventTypeString(ev.type));
  }
  delay(10);
}