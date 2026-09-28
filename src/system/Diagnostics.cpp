#include "system/Diagnostics.hpp"

#include <ArduinoJson.h>

namespace gh {

String Diagnostics::build(SensorManager* sensors, MqttManager* mqtt,
                          bool wifiOk, const char* ip, const char* fwVersion, const char* hwVersion) {
  DynamicJsonDocument doc(2048);
  doc["wifi"] = wifiOk ? "OK" : "ERROR";
  doc["ip"] = ip;
  doc["mqtt"] = (mqtt && mqtt->enabled()) ? (mqtt->connected() ? "OK" : "ERROR") : "DISABLED";
  doc["sht31"] = (sensors && sensors->sht31Available()) ? "OK" : "NOT_FOUND";
  doc["ads1115"] = (sensors && sensors->adsAvailable()) ? "OK" : "NOT_FOUND";
  doc["ds18b20"] = (sensors && sensors->ds18b20Count() > 0) ? "OK" : "NOT_FOUND";
  doc["bh1750"] = (sensors && sensors->lightAvailable()) ? "OK" : "NOT_FOUND";
  doc["scd41"] = (sensors && sensors->co2Available()) ? "OK" : "NOT_FOUND";
  doc["firmware"] = fwVersion;
  doc["hardware"] = hwVersion;

  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
