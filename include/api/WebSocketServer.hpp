#pragma once
// WebSocket en tiempo real (sección 44) en /ws (puerto 81).
// Envía periódicamente el estado de sensores y actuadores a los clientes.

#include <Arduino.h>
#include <WebSocketsServer.h>

#include "config/ConfigManager.hpp"
#include "sensors/SensorManager.hpp"
#include "actuators/ActuatorManager.hpp"

namespace gh {

class WebSocketServer {
public:
  void begin(ConfigManager* cfg, SensorManager* s, ActuatorManager* a);
  void loop();                        // Procesa y emite periódicamente

private:
  WebSocketsServer ws_{81};
  ConfigManager* cfg_ = nullptr;
  SensorManager* sensors_ = nullptr;
  ActuatorManager* actuators_ = nullptr;
  uint32_t lastBroadcast_ = 0;

  void broadcastState();
  static void onEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
};

} // namespace gh
