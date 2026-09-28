#pragma once
// Diagnóstico de componentes (sección 55): estado de WiFi/MQTT/sensores.

#include <Arduino.h>
#include <WiFi.h>

#include "core/Types.hpp"
#include "sensors/SensorManager.hpp"
#include "network/MqttManager.hpp"

namespace gh {

class Diagnostics {
public:
  // Genera un JSON con el estado de cada componente.
  static String build(SensorManager* sensors, MqttManager* mqtt,
                      bool wifiOk, const char* ip, const char* fwVersion, const char* hwVersion);
};

} // namespace gh
