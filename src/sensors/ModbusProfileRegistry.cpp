#include "sensors/ModbusProfileRegistry.hpp"

#include <ArduinoJson.h>

namespace gh {

bool ModbusProfileRegistry::registerProfile(const ModbusProfile& p) {
  if (p.id[0] == '\0') return false;
  int8_t idx = findProfile(p.id);
  if (idx >= 0) {
    profiles_[idx] = p;
    return true;
  }
  if (profileCount_ >= MAX_PROFILES) return false;
  profiles_[profileCount_++] = p;
  return true;
}

bool ModbusProfileRegistry::registerInstance(const SensorInstance& i) {
  if (i.id[0] == '\0') return false;
  int8_t idx = findInstance(i.id);
  if (idx >= 0) {
    instances_[idx] = i;
    return true;
  }
  if (instanceCount_ >= MAX_INSTANCES) return false;
  instances_[instanceCount_++] = i;
  return true;
}

bool ModbusProfileRegistry::setInstanceState(const char* id, ProvisioningState s) {
  int8_t idx = findInstance(id);
  if (idx < 0) return false;
  instances_[idx].state = s;
  return true;
}

bool ModbusProfileRegistry::getProfile(const char* id, ModbusProfile& out) const {
  int8_t idx = findProfile(id);
  if (idx < 0) return false;
  out = profiles_[idx];
  return true;
}

bool ModbusProfileRegistry::getInstance(const char* id, SensorInstance& out) const {
  int8_t idx = findInstance(id);
  if (idx < 0) return false;
  out = instances_[idx];
  return true;
}

String ModbusProfileRegistry::toJson() const {
  DynamicJsonDocument doc(4096);
  JsonObject root = doc.to<JsonObject>();
  JsonArray pr = root.createNestedArray("profiles");
  for (uint8_t i = 0; i < profileCount_; i++) {
    const ModbusProfile& p = profiles_[i];
    JsonObject o = pr.createNestedObject();
    o["id"] = p.id;
    o["vendor"] = p.vendor;
    o["product"] = p.product;
    o["vendor_id"] = p.vendorId;
    o["product_id"] = p.productId;
    o["slave_id"] = p.slaveId;
    o["register"] = p.registerAddress;
    o["data_type"] = modbusDataTypeString(p.dataType);
    o["scale"] = p.scale;
    o["offset"] = p.offset;
    o["unit"] = p.unit;
    o["magnitude"] = (int)p.magnitude;
  }
  JsonArray in = root.createNestedArray("instances");
  for (uint8_t i = 0; i < instanceCount_; i++) {
    const SensorInstance& s = instances_[i];
    JsonObject o = in.createNestedObject();
    o["id"] = s.id;
    o["name"] = s.name;
    o["profile"] = s.profileId;
    o["slave_id"] = s.slaveId;
    o["zone"] = s.zone;
    o["enabled"] = s.enabled;
    o["state"] = provisioningStateString(s.state);
  }
  String out;
  serializeJson(doc, out);
  return out;
}

int8_t ModbusProfileRegistry::findProfile(const char* id) const {
  if (id == nullptr) return -1;
  for (uint8_t i = 0; i < profileCount_; i++) {
    if (strncmp(profiles_[i].id, id, sizeof(profiles_[i].id)) == 0) return i;
  }
  return -1;
}

int8_t ModbusProfileRegistry::findInstance(const char* id) const {
  if (id == nullptr) return -1;
  for (uint8_t i = 0; i < instanceCount_; i++) {
    if (strncmp(instances_[i].id, id, sizeof(instances_[i].id)) == 0) return i;
  }
  return -1;
}

} // namespace gh
