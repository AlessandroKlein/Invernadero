#include "system/OtaManager.hpp"

namespace gh {

void OtaManager::begin(const char* hostname) {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.onStart([]() {
    String type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
    Serial.printf("[OTA] Inicio: %s\n", type.c_str());
  });
  ArduinoOTA.onEnd([]() { Serial.println("[OTA] Fin"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error %u\n", error);
  });
  ArduinoOTA.begin();
}

} // namespace gh
