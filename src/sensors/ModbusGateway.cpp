#include "sensors/ModbusGateway.hpp"

#include <ArduinoJson.h>

namespace gh {

void ModbusGateway::begin(ModbusRtu* rtu, ModbusProfileRegistry* registry) {
  rtu_ = rtu;
  registry_ = registry;
  count_ = 0;
}

uint8_t ModbusGateway::registersFor(ModbusDataType t) {
  return (t == ModbusDataType::UINT32 || t == ModbusDataType::INT32 || t == ModbusDataType::FLOAT32) ? 2 : 1;
}

float ModbusGateway::convert(const uint16_t* regs, ModbusDataType type, float scale, float offset) {
  float raw = 0.0f;
  switch (type) {
    case ModbusDataType::UINT16: raw = (float)regs[0]; break;
    case ModbusDataType::INT16:  raw = (float)(int16_t)regs[0]; break;
    case ModbusDataType::UINT32: raw = (float)(((uint32_t)regs[0] << 16) | regs[1]); break;
    case ModbusDataType::INT32:  raw = (float)(int32_t)(((uint32_t)regs[0] << 16) | regs[1]); break;
    case ModbusDataType::FLOAT32: {
      // Orden de palabras big-endian (registro alto primero), típico en Modbus.
      uint32_t b = ((uint32_t)regs[0] << 16) | regs[1];
      float f;
      memcpy(&f, &b, 4);
      raw = f;
      break;
    }
  }
  return raw * scale + offset;
}

void ModbusGateway::rebuild() {
  count_ = 0;
  if (!registry_) return;

  SensorInstance inst[ModbusProfileRegistry::MAX_INSTANCES];
  size_t n = 0;
  registry_->snapshotInstances(inst, ModbusProfileRegistry::MAX_INSTANCES, n);

  for (size_t i = 0; i < n && count_ < MAX_SLAVES; i++) {
    const SensorInstance& si = inst[i];
    if (!si.enabled) continue;
    ModbusProfile p;
    if (!registry_->getProfile(si.profileId, p)) continue;

    Slave& s = slaves_[count_++];
    strncpy(s.instanceId, si.id, sizeof(s.instanceId) - 1);
    s.instanceId[sizeof(s.instanceId) - 1] = '\0';
    s.slaveId = (si.slaveId != 0) ? si.slaveId : p.slaveId;
    s.registerAddress = p.registerAddress;
    s.dataType = p.dataType;
    s.scale = p.scale;
    s.offset = p.offset;
    s.magnitude = p.magnitude;
    s.pollIntervalMs = 5000;
    s.lastPollMs = 0;
    s.value = 0.0f;
    s.valid = false;
    s.state = UNKNOWN;
    s.failCount = 0;
  }
}

bool ModbusGateway::readSlave(Slave& s) {
  uint16_t regs[2] = {0, 0};
  uint8_t n = registersFor(s.dataType);
  bool ok = rtu_->readHoldingRegisters(s.slaveId, s.registerAddress, n, regs, 250);
  if (!ok) {
    s.valid = false;
    s.failCount++;
    // Clasificar el fallo usando las estadísticas del bus.
    ModbusStats st = rtu_->stats();
    s.state = (st.lastError == 2) ? CRC_ERROR : TIMEOUT;
    return false;
  }
  s.value = convert(regs, s.dataType, s.scale, s.offset);
  s.valid = true;
  s.state = OK;
  s.failCount = 0;
  return true;
}

void ModbusGateway::tick() {
  if (!rtu_ || count_ == 0) return;
  uint32_t now = millis();
  for (uint8_t i = 0; i < count_; i++) {
    Slave& s = slaves_[i];
    if (now - s.lastPollMs >= s.pollIntervalMs) {
      readSlave(s);
      s.lastPollMs = now;
    }
  }
}

void ModbusGateway::snapshot(Slave* out, size_t max, size_t& n) const {
  n = 0;
  for (uint8_t i = 0; i < count_ && n < max; i++) out[n++] = slaves_[i];
}

String ModbusGateway::toJson() const {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < count_; i++) {
    const Slave& s = slaves_[i];
    JsonObject o = arr.createNestedObject();
    o["instance"] = s.instanceId;
    o["slave_id"] = s.slaveId;
    o["register"] = s.registerAddress;
    o["magnitude"] = (int)s.magnitude;
    o["value"] = s.value;
    o["valid"] = s.valid;
    o["state"] = (int)s.state;
    o["fail_count"] = s.failCount;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
