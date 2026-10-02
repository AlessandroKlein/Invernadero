#include "core/PinConfig.hpp"

#include <ArduinoJson.h>

namespace gh {

PinConfig defaultPinConfig() {
  return PinConfig();  // los defaults del struct son el PinMap de referencia
}

String pinConfigToJson(const PinConfig& p) {
  DynamicJsonDocument doc(1536);
  JsonObject o = doc.to<JsonObject>();
  o["i2c_sda"] = p.i2cSda;
  o["i2c_scl"] = p.i2cScl;
  o["i2c_freq"] = p.i2cFreq;
  o["hc595_mosi"] = p.hc595Mosi;
  o["hc595_sclk"] = p.hc595Sclk;
  o["hc595_latch"] = p.hc595Latch;
  o["hc595_count"] = p.hc595Count;
  o["onewire"] = p.oneWire;
  o["flow_pin"] = p.flowPin;
  o["rain_pin"] = p.rainPin;
  o["wind_pin"] = p.windPin;
  o["tank_trig"] = p.tankTrig;
  o["tank_echo"] = p.tankEcho;
  o["float_low"] = p.floatLow;
  o["float_high"] = p.floatHigh;
  o["emergency_stop"] = p.emergencyStop;
  o["rs485_rx"] = p.rs485Rx;
  o["rs485_tx"] = p.rs485Tx;
  o["rs485_de"] = p.rs485De;
  o["spi_sck"] = p.spiSck;
  o["spi_miso"] = p.spiMiso;
  o["spi_mosi"] = p.spiMosi;
  o["i2c_addr_sht31"] = p.i2cAddrSht31;
  o["i2c_addr_aht20"] = p.i2cAddrAht20;
  o["i2c_addr_ads1115"] = p.i2cAddrAds1115;
  o["i2c_addr_bh1750"] = p.i2cAddrBh1750;
  o["i2c_addr_scd41"] = p.i2cAddrScd41;
  o["i2c_addr_mcp23017"] = p.i2cAddrMcp23017;
  String out;
  serializeJson(doc, out);
  return out;
}

bool pinConfigFromJson(const String& json, PinConfig& out) {
  DynamicJsonDocument doc(1536);
  if (deserializeJson(doc, json)) return false;
  if (!doc.is<JsonObject>()) return false;
  JsonObject o = doc.as<JsonObject>();
  out.i2cSda = o["i2c_sda"] | out.i2cSda;
  out.i2cScl = o["i2c_scl"] | out.i2cScl;
  out.i2cFreq = o["i2c_freq"] | out.i2cFreq;
  out.hc595Mosi = o["hc595_mosi"] | out.hc595Mosi;
  out.hc595Sclk = o["hc595_sclk"] | out.hc595Sclk;
  out.hc595Latch = o["hc595_latch"] | out.hc595Latch;
  out.hc595Count = o["hc595_count"] | out.hc595Count;
  out.oneWire = o["onewire"] | out.oneWire;
  out.flowPin = o["flow_pin"] | out.flowPin;
  out.rainPin = o["rain_pin"] | out.rainPin;
  out.windPin = o["wind_pin"] | out.windPin;
  out.tankTrig = o["tank_trig"] | out.tankTrig;
  out.tankEcho = o["tank_echo"] | out.tankEcho;
  out.floatLow = o["float_low"] | out.floatLow;
  out.floatHigh = o["float_high"] | out.floatHigh;
  out.emergencyStop = o["emergency_stop"] | out.emergencyStop;
  out.rs485Rx = o["rs485_rx"] | out.rs485Rx;
  out.rs485Tx = o["rs485_tx"] | out.rs485Tx;
  out.rs485De = o["rs485_de"] | out.rs485De;
  out.spiSck = o["spi_sck"] | out.spiSck;
  out.spiMiso = o["spi_miso"] | out.spiMiso;
  out.spiMosi = o["spi_mosi"] | out.spiMosi;
  out.i2cAddrSht31 = o["i2c_addr_sht31"] | out.i2cAddrSht31;
  out.i2cAddrAht20 = o["i2c_addr_aht20"] | out.i2cAddrAht20;
  out.i2cAddrAds1115 = o["i2c_addr_ads1115"] | out.i2cAddrAds1115;
  out.i2cAddrBh1750 = o["i2c_addr_bh1750"] | out.i2cAddrBh1750;
  out.i2cAddrScd41 = o["i2c_addr_scd41"] | out.i2cAddrScd41;
  out.i2cAddrMcp23017 = o["i2c_addr_mcp23017"] | out.i2cAddrMcp23017;
  return true;
}

void PinConfigManager::begin() {
  prefs_.begin(NVS_NS, false);
  String json = prefs_.getString(NVS_KEY, "");
  if (json.length() > 0 && pinConfigFromJson(json, cfg_)) return;
  cfg_ = defaultPinConfig();
  prefs_.putString(NVS_KEY, pinConfigToJson(cfg_));
}

PinConfig PinConfigManager::get() const {
  return cfg_;
}

void PinConfigManager::set(const PinConfig& p) {
  cfg_ = p;
  prefs_.putString(NVS_KEY, pinConfigToJson(cfg_));
}

void PinConfigManager::reset() {
  cfg_ = defaultPinConfig();
  prefs_.putString(NVS_KEY, pinConfigToJson(cfg_));
}

} // namespace gh
