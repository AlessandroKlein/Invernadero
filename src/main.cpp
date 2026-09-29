// PROYECTO INTEGRAL DE AUTOMATIZACIÓN MODULAR DE INVERNADERO BASADO EN ESP32
// Punto de entrada: inicializa todos los módulos y lanza la tarea de automatización.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "core/Types.hpp"
#include "core/PinMap.hpp"
#include "core/Version.hpp"

#include "config/ConfigManager.hpp"
#include "storage/History.hpp"

#include "hardware/ShiftRegister595.hpp"
#include "hardware/Mcp23017.hpp"

#include "sensors/SensorManager.hpp"
#include "actuators/ActuatorManager.hpp"

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

using namespace gh;

// Contexto compartido entre setup/loop y la tarea de automatización.
struct App {
  ConfigManager config;
  History history;
  ShiftRegister595 shift;
  Mcp23017 mcp;
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
  OtaManager ota;
};
static App app;

// Tarea de automatización: lectura de sensores + control + aplicación de salidas.
static void automationTask(void* arg) {
  (void)arg;
  for (;;) {
    SystemConfig cfg = app.config.get();

    app.sensors.update();          // Leer sensores habilitados
    app.safety.update(cfg);        // Seguridad (prioridad máxima)
    app.climate.update(cfg);       // Clima
    app.irrigation.update(cfg);    // Riego
    app.lighting.update(cfg);      // Iluminación
    app.roof.update(cfg);          // Techo/ventanas
    app.rules.update();            // Reglas configurables (se suman a los controladores)
    app.actuators.apply();         // Escribir salidas físicas

    vTaskDelay(pdMS_TO_TICKS(2000));
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

  // 2) Historial.
  app.history.begin(128);

  // 3) Hardware de salida: 74HC595 (SPI) + MCP23017 (I²C).
  app.shift.begin(pins::HC595_MOSI, pins::HC595_SCLK, pins::HC595_LATCH, pins::HC595_COUNT);
  app.shift.allOff(); // estado seguro al arrancar (sección 21.14)
  // Soft-PWM deshabilitado por defecto (modo digital ON/OFF seguro). Para PWM
  // de alta frecuencia se recomienda LEDC en GPIO o controlador dedicado.
  // app.shift.setPwmEnabled(true, 200, 8);
  Wire.begin(pins::I2C_SDA, pins::I2C_SCL, pins::I2C_FREQ);
  app.mcp.begin(pins::I2C_ADDR_MCP23017_1, &Wire);

  // 4) Sensores y actuadores.
  app.sensors.begin(app.config.get());
  app.actuators.begin(app.config.get(), &app.shift, &app.mcp);
  app.actuators.allSafeState();
  Device::setState(DeviceState::SELF_TEST);

  // 5) Controladores.
  app.climate.begin(&app.sensors, &app.actuators, &app.history);
  app.irrigation.begin(&app.sensors, &app.actuators, &app.history);
  app.lighting.begin(&app.sensors, &app.actuators);
  app.roof.begin(&app.sensors, &app.actuators);
  app.safety.begin(&app.sensors, &app.actuators, &app.history);
  app.rules.begin(&app.sensors, &app.actuators, &app.history);

  // 6) Red, MQTT, API, WebSocket, OTA, Watchdog.
  app.network.begin(app.config.get());
  Device::setState(DeviceState::NETWORK);
  app.weather.begin(app.config.get());
  app.mqtt.begin(app.config.get());
  app.api.begin(&app.config, &app.sensors, &app.actuators, &app.history,
                &app.network, &app.mqtt);
  app.api.setRuleEngine(&app.rules);
  app.api.setWeather(&app.weather);
  app.ws.begin(&app.config, &app.sensors, &app.actuators);
  app.ota.begin(app.config.get().hostname);
  app.watchdog.begin(30);

  // 7) Lanzar tarea de automatización en el núcleo 0.
  xTaskCreatePinnedToCore(automationTask, "automation", 8192, nullptr, 1, nullptr, 0);

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

  // Procesar comandos MQTT (p. ej. OTA remota desde el servidor central).
  String cmd;
  if (app.mqtt.consumeCommand(cmd)) {
    DynamicJsonDocument cdoc(1024);
    if (deserializeJson(cdoc, cmd) == DeserializationError::Ok) {
      String type = cdoc["type"] | "";
      if (type == "ota") {
        String url = cdoc["url"] | "";
        String sha = cdoc["sha256"] | "";
        if (url.length()) app.ota.applyFromUrl(url, sha);
      }
    }
  }

  // Publicar estado por MQTT cada 10 s (si hay servidor central).
  static uint32_t lastMqtt = 0;
  if (millis() - lastMqtt > 10000) {
    lastMqtt = millis();
    if (app.mqtt.connected()) {
      app.mqtt.publishSensors(app.sensors.toJson());
      app.mqtt.publishActuators(app.actuators.toJson());
      if (app.weather.enabled()) app.mqtt.publishWeather(app.weather.toJson());
    }
  }
  delay(10);
}