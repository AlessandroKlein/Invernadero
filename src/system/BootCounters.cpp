#include "system/BootCounters.hpp"

#include <ArduinoJson.h>

#include "system/Device.hpp"

namespace gh {

static const char* K_BOOT = "boot_count";
static const char* K_WDT = "watchdog_count";
static const char* K_BROWNOUT = "brownout_count";
static const char* K_PANIC = "panic_count";
static const char* K_SOFT = "soft_reset_count";
static const char* K_OTA = "ota_count";
static const char* K_FACTORY = "factory_count";

void BootCounters::begin() {
  prefs_.begin(NVS_NS, false);
  increment(K_BOOT);
  // Incrementa el contador específico de la causa del último reinicio (sección 150).
  switch (Device::resetCause()) {
    case ResetCause::WATCHDOG:       increment(K_WDT); break;
    case ResetCause::BROWNOUT:       increment(K_BROWNOUT); break;
    case ResetCause::PANIC:          increment(K_PANIC); break;
    case ResetCause::SOFTWARE_RESET: increment(K_SOFT); break;
    case ResetCause::OTA:            increment(K_OTA); break;
    case ResetCause::FACTORY_RESET:  increment(K_FACTORY); break;
    default: break;
  }
}

void BootCounters::increment(const char* key) {
  prefs_.putUInt(key, prefs_.getUInt(key, 0) + 1);
}

uint32_t BootCounters::get(const char* key) const {
  return prefs_.getUInt(key, 0);
}

String BootCounters::toJson() const {
  DynamicJsonDocument doc(512);
  doc["boot_count"] = get(K_BOOT);
  doc["watchdog_count"] = get(K_WDT);
  doc["brownout_count"] = get(K_BROWNOUT);
  doc["panic_count"] = get(K_PANIC);
  doc["soft_reset_count"] = get(K_SOFT);
  doc["ota_count"] = get(K_OTA);
  doc["factory_count"] = get(K_FACTORY);
  doc["last_reset"] = resetCauseString(Device::resetCause());
  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
