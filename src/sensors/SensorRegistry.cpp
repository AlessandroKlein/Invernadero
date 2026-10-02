#include "sensors/SensorRegistry.hpp"

#include <ArduinoJson.h>

namespace gh {

bool SensorRegistry::registerSensor(const SensorEntry& e) {
  if (e.id[0] == '\0') return false;
  int8_t idx = find(e.id);
  if (idx >= 0) {
    entries_[idx] = e;
    return true;
  }
  if (count_ >= MAX_ENTRIES) return false;
  entries_[count_++] = e;
  return true;
}

void SensorRegistry::buildFromConfig(const SystemConfig& cfg) {
  // Mapea los flags de configuración a entradas del catálogo. Los drivers y
  // direcciones reflejan el hardware de referencia; el catálogo puede ampliarse
  // por configuración sin tocar este código.
  auto add = [&](const char* id, const char* name, const char* driver,
                 SensorType type, BusType bus, uint8_t addr, uint8_t zone, bool enabled) {
    SensorEntry e;
    strncpy(e.id, id, sizeof(e.id) - 1);
    strncpy(e.name, name, sizeof(e.name) - 1);
    strncpy(e.driver, driver, sizeof(e.driver) - 1);
    e.type = type;
    e.bus = bus;
    e.address = addr;
    e.zone = zone;
    e.enabled = enabled;
    registerSensor(e);
  };

  if (cfg.sensorSht31) {
    add("temp_interior", "Temperatura interior", "sht31", SensorType::TEMP_SHT31, BusType::I2C, 0x44, 0, true);
    add("hum_interior",  "Humedad interior",     "sht31", SensorType::HUM_SHT31,  BusType::I2C, 0x44, 0, true);
  }
  if (cfg.sensorDs18b20)
    add("temp_18b20", "Temperatura 1-Wire", "ds18b20", SensorType::TEMP_DS18B20, BusType::ONEWIRE, 0, 0, true);
  if (cfg.sensorSoil) {
    for (uint8_t z = 0; z < cfg.zoneCount && z < 4; z++) {
      char id[16], name[24];
      snprintf(id, sizeof(id), "soil_%u", z);
      snprintf(name, sizeof(name), "Humedad suelo Z%u", z + 1);
      add(id, name, "capacitive", SensorType::SOIL_MOISTURE, BusType::GPIO, 0, z, true);
    }
  }
  if (cfg.sensorLight) add("light", "Iluminación", "bh1750", SensorType::LIGHT_LUX, BusType::I2C, 0x23, 0, true);
  if (cfg.sensorCo2)   add("co2",   "CO2",         "scd41",  SensorType::CO2,       BusType::I2C, 0x62, 0, true);
  if (cfg.sensorTank)  add("tank",  "Nivel tanque", "ultrasonic", SensorType::TANK_LEVEL, BusType::GPIO, 0, 0, true);
  if (cfg.sensorFlow)  add("flow",  "Caudal",      "flow_meter", SensorType::FLOW_RATE,   BusType::GPIO, 0, 0, true);
  if (cfg.sensorRain)  add("rain",  "Lluvia",      "rain_gauge", SensorType::RAIN_ACCUM,  BusType::GPIO, 0, 0, true);
  if (cfg.sensorWind)  add("wind",  "Viento",      "anemometer", SensorType::WIND_SPEED,  BusType::GPIO, 0, 0, true);
  if (cfg.sensorPh)    add("ph",    "pH",          "ph",         SensorType::PH,          BusType::RS485, 0, 0, true);
  if (cfg.sensorEc)    add("ec",    "EC",          "ec",         SensorType::EC,          BusType::RS485, 0, 0, true);
  if (cfg.sensorExterior) {
    add("temp_exterior", "Temperatura exterior", "aht20", SensorType::TEMP_EXTERIOR, BusType::I2C, 0x38, 0, true);
    add("hum_exterior",  "Humedad exterior",     "aht20", SensorType::HUM_EXTERIOR,  BusType::I2C, 0x38, 0, true);
  }
}

bool SensorRegistry::remove(const char* id) {
  int8_t idx = find(id);
  if (idx < 0) return false;
  if (idx != count_ - 1) entries_[idx] = entries_[count_ - 1];
  count_--;
  return true;
}

bool SensorRegistry::get(const char* id, SensorEntry& out) const {
  int8_t idx = find(id);
  if (idx < 0) return false;
  out = entries_[idx];
  return true;
}

uint8_t SensorRegistry::countEnabled() const {
  uint8_t n = 0;
  for (uint8_t i = 0; i < count_; i++) if (entries_[i].enabled) n++;
  return n;
}

void SensorRegistry::snapshot(SensorEntry* out, size_t max, size_t& n) const {
  n = 0;
  for (uint8_t i = 0; i < count_ && n < max; i++) out[n++] = entries_[i];
}

String SensorRegistry::toJson() const {
  DynamicJsonDocument doc(3072);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < count_; i++) {
    const SensorEntry& e = entries_[i];
    JsonObject o = arr.createNestedObject();
    o["id"] = e.id;
    o["name"] = e.name;
    o["driver"] = e.driver;
    o["type"] = (int)e.type;
    o["bus"] = busTypeString(e.bus);
    o["bus_index"] = e.busIndex;
    o["address"] = e.address;
    o["zone"] = e.zone;
    o["enabled"] = e.enabled;
    o["read_interval_ms"] = e.readIntervalMs;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

bool SensorRegistry::fromJson(const String& json) {
  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, json)) return false;
  if (!doc.is<JsonArray>()) return false;
  for (JsonObject o : doc.as<JsonArray>()) {
    const char* id = o["id"] | "";
    SensorEntry e;
    if (!get(id, e)) continue;  // solo actualizar entradas existentes (driver conocido)
    e.enabled = o["enabled"] | e.enabled;
    e.address = o["address"] | e.address;
    e.zone = o["zone"] | e.zone;
    e.busIndex = o["bus_index"] | e.busIndex;
    e.readIntervalMs = o["read_interval_ms"] | e.readIntervalMs;
    registerSensor(e);
  }
  return true;
}

void SensorRegistry::load() {
  prefs_.begin("ghsensors", false);
  String j = prefs_.getString("catalog", "");
  prefs_.end();
  if (j.length() > 0) fromJson(j);
}

void SensorRegistry::save() {
  prefs_.begin("ghsensors", false);
  prefs_.putString("catalog", toJson());
  prefs_.end();
}

int8_t SensorRegistry::find(const char* id) const {
  if (id == nullptr) return -1;
  for (uint8_t i = 0; i < count_; i++) {
    if (strncmp(entries_[i].id, id, sizeof(entries_[i].id)) == 0) return i;
  }
  return -1;
}

} // namespace gh
