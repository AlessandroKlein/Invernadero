#include "system/Diagnostics.hpp"

#include <ArduinoJson.h>
#include "system/Device.hpp"

namespace gh {

String Diagnostics::build(SensorManager* sensors, MqttManager* mqtt,
                          bool wifiOk, const char* ip, const char* fwVersion, const char* hwVersion) {
  DynamicJsonDocument doc(3072);

  // Red y comunicaciones (sección 55).
  doc["wifi"] = wifiOk ? "OK" : "ERROR";
  doc["rssi"] = WiFi.RSSI();
  doc["ip"] = ip;
  doc["mqtt"] = (mqtt && mqtt->enabled()) ? (mqtt->connected() ? "OK" : "ERROR") : "DISABLED";

  // Componentes de hardware.
  doc["sht31"] = (sensors && sensors->sht31Available()) ? "OK" : "NOT_FOUND";
  doc["ads1115"] = (sensors && sensors->adsAvailable()) ? "OK" : "NOT_FOUND";
  doc["ds18b20"] = (sensors && sensors->ds18b20Count() > 0) ? "OK" : "NOT_FOUND";
  doc["bh1750"] = (sensors && sensors->lightAvailable()) ? "OK" : "NOT_FOUND";
  doc["scd41"] = (sensors && sensors->co2Available()) ? "OK" : "NOT_FOUND";

  // Identidad y estado del dispositivo (secciones 133/150/175).
  doc["firmware"] = fwVersion;
  doc["hardware"] = hwVersion;
  doc["device_uid"] = Device::uid();
  doc["state"] = deviceStateString(Device::state());
  doc["reset_cause"] = resetCauseString(Device::resetCause());
  doc["uptime_s"] = (uint32_t)(millis() / 1000);
  doc["heap_free"] = ESP.getFreeHeap();
  doc["heap_min"] = ESP.getMinFreeHeap();

  // RS485/Modbus (sección 177).
  if (sensors) {
    ModbusStats s = sensors->modbusStats();
    JsonObject mb = doc.createNestedObject("rs485");
    mb["tx"] = s.txCount;
    mb["rx"] = s.rxCount;
    mb["crc_errors"] = s.crcErrors;
    mb["timeouts"] = s.timeouts;
    mb["devices_found"] = s.devicesFound;
  }

  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
