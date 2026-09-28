#include "system/OtaManager.hpp"
#include "system/Device.hpp"

#include <esp_ota_ops.h>

namespace gh {

void OtaManager::begin(const char* hostname) {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.onStart([]() {
    Device::setState(DeviceState::UPDATING); // sección 175
    String type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
    Serial.printf("[OTA] Inicio: %s\n", type.c_str());
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("[OTA] Fin");
    // Confirmar el nuevo firmware para cancelar el rollback automático del
    // bootloader (sección 146). Si la app no arranca correctamente, el
    // bootloader restaurará la partición anterior.
    esp_ota_mark_app_valid_cancel_rollback();
    Device::setState(DeviceState::RUN); // sección 175
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error %u\n", error);
  });
  ArduinoOTA.begin();
}

} // namespace gh
