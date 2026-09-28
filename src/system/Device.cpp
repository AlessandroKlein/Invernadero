#include "system/Device.hpp"

#include <WiFi.h>
#include <esp_system.h>
#include <ArduinoJson.h>

#include "core/Version.hpp"

namespace gh {

// --- Conversiones a cadena (secciones 150/175/176). Definidas aquí para no
// --- crear un archivo fuente adicional. ---

const char* sensorStatusString(SensorStatus s) {
  switch (s) {
    case SensorStatus::OK:          return "GOOD";
    case SensorStatus::WARNING:     return "WARNING";
    case SensorStatus::ERROR:       return "ERROR";
    case SensorStatus::DISCONNECTED:return "DISCONNECTED";
    case SensorStatus::OUT_OF_RANGE:return "OUT_OF_RANGE";
    case SensorStatus::INVALID:     return "INVALID";
    case SensorStatus::TIMEOUT:     return "TIMEOUT";
    case SensorStatus::CALIBRATION: return "CALIBRATION";
    default:                        return "UNKNOWN";
  }
}

const char* deviceStateString(DeviceState s) {
  switch (s) {
    case DeviceState::BOOTING:      return "BOOTING";
    case DeviceState::INITIALIZING: return "INITIALIZING";
    case DeviceState::SELF_TEST:    return "SELF_TEST";
    case DeviceState::NETWORK:      return "NETWORK";
    case DeviceState::SYNC:         return "SYNC";
    case DeviceState::RUN:          return "RUN";
    case DeviceState::DEGRADED:     return "DEGRADED";
    case DeviceState::ERROR:        return "ERROR";
    case DeviceState::MAINTENANCE:  return "MAINTENANCE";
    case DeviceState::UPDATING:     return "UPDATING";
    case DeviceState::RECOVERY:     return "RECOVERY";
    default:                        return "UNKNOWN";
  }
}

const char* resetCauseString(ResetCause r) {
  switch (r) {
    case ResetCause::POWER_ON:      return "POWER_ON";
    case ResetCause::SOFTWARE_RESET:return "SOFTWARE_RESET";
    case ResetCause::WATCHDOG:      return "WATCHDOG";
    case ResetCause::BROWNOUT:      return "BROWNOUT";
    case ResetCause::PANIC:         return "PANIC";
    case ResetCause::OTA:           return "OTA";
    case ResetCause::FACTORY_RESET: return "FACTORY_RESET";
    default:                        return "UNKNOWN";
  }
}

const char* updateChannelString(UpdateChannel c) {
  switch (c) {
    case UpdateChannel::BETA:        return "beta";
    case UpdateChannel::DEVELOPMENT: return "development";
    default:                         return "stable";
  }
}

// --- Identidad del dispositivo ---

DeviceState Device::state_ = DeviceState::BOOTING;

const char* Device::uid() {
  // UID permanente a partir de los 24 bits bajos de la MAC del chip (sección 118).
  static char buf[24];
  uint64_t mac = ESP.getEfuseMac();
  snprintf(buf, sizeof(buf), "ESP32-%06X", (uint32_t)(mac & 0xFFFFFF));
  return buf;
}

static void addCap(DeviceInfo& d, const char* c) {
  if (d.capabilityCount < 10) {
    strncpy(d.capabilities[d.capabilityCount++], c, 15);
  }
}

DeviceInfo Device::info(const SystemConfig& cfg) {
  (void)cfg;
  DeviceInfo d;
  strncpy(d.deviceUid, uid(), sizeof(d.deviceUid) - 1);
  strncpy(d.hardwareProfile, GH_HW_PROFILE, sizeof(d.hardwareProfile) - 1);
  strncpy(d.firmwareVersion, GH_FW_VERSION, sizeof(d.firmwareVersion) - 1);
  strncpy(d.hardwareVersion, GH_HW_VERSION, sizeof(d.hardwareVersion) - 1);
  d.configSchemaVersion = GH_CONFIG_SCHEMA_VERSION;
  d.protocolVersion = GH_PROTOCOL_VERSION;
  d.channel = cfg.updateChannel;
  // Capacidades soportadas por este firmware/hardware (sección 166).
  addCap(d, "WIFI");
  addCap(d, "I2C");
  addCap(d, "SPI");
  addCap(d, "ONEWIRE");
  addCap(d, "ADC");
  addCap(d, "GPIO");
  addCap(d, "RS485");
  addCap(d, "MODBUS");
  addCap(d, "PWM");
  addCap(d, "OTA");
  return d;
}

ResetCause Device::resetCause() {
  // Traducción del motivo de reinicio de esp_system.h (sección 150).
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  return ResetCause::POWER_ON;
    case ESP_RST_SW:       return ResetCause::SOFTWARE_RESET;
    case ESP_RST_PANIC:    return ResetCause::PANIC;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:      return ResetCause::WATCHDOG;
    case ESP_RST_BROWNOUT: return ResetCause::BROWNOUT;
    default:               return ResetCause::UNKNOWN;
  }
}

void Device::setState(DeviceState s) { state_ = s; }
DeviceState Device::state() { return state_; }

String Device::deviceJson(const SystemConfig& cfg) {
  DeviceInfo d = info(cfg);
  DynamicJsonDocument doc(1024);
  doc["device_uid"] = d.deviceUid;
  doc["device_id"] = cfg.deviceId;
  doc["greenhouse_id"] = cfg.greenhouseId;
  doc["hardware_profile"] = d.hardwareProfile;
  doc["firmware"] = d.firmwareVersion;
  doc["hardware"] = d.hardwareVersion;
  doc["config_schema"] = d.configSchemaVersion;
  doc["protocol"] = d.protocolVersion;
  doc["channel"] = updateChannelString(d.channel);
  doc["state"] = deviceStateString(state());
  doc["reset_cause"] = resetCauseString(resetCause());
  String out;
  serializeJson(doc, out);
  return out;
}

String Device::capabilitiesJson(const SystemConfig& cfg) {
  DeviceInfo d = info(cfg);
  DynamicJsonDocument doc(512);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < d.capabilityCount; i++) arr.add(d.capabilities[i]);
  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
