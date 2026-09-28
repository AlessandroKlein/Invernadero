#pragma once
// Actualización OTA (sección 59) con ArduinoOTA y rollback vía doble partición.

#include <Arduino.h>
#include <ArduinoOTA.h>

namespace gh {

class OtaManager {
public:
  void begin(const char* hostname);
  void loop() { ArduinoOTA.handle(); }
};

} // namespace gh
