#include "hardware/BusManager.hpp"

#include <ArduinoJson.h>

namespace gh {

void BusManager::begin(const PinConfig& pins) {
  // Única inicialización del bus I²C compartido (SHT31/AHT20/ADS1115/MCP23017).
  Wire.begin(pins.i2cSda, pins.i2cScl, pins.i2cFreq);
  registerBus(BusType::I2C, 0, true);
  // SPI (bit-banged por el 74HC595 hoy; periférico SPI para W5500/SD/MCP23S17 en V8.1).
  registerBus(BusType::SPI, 0, true);
  registerBus(BusType::ONEWIRE, 0, true);  // DS18B20
  registerBus(BusType::RS485, 0, true);    // Modbus RTU
  registerBus(BusType::GPIO, 0, true);
}

bool BusManager::registerBus(BusType type, uint8_t index, bool enabled) {
  if (type == BusType::NONE) return false;
  int8_t idx = find(type, index);
  if (idx >= 0) {
    slots_[idx].enabled = enabled;
    if (enabled && slots_[idx].state == BusState::UNREGISTERED)
      slots_[idx].state = BusState::READY;
    return true;
  }
  if (count_ >= MAX_BUSES) return false;
  Slot& s = slots_[count_++];
  s.type = type;
  s.index = index;
  s.enabled = enabled;
  s.state = enabled ? BusState::READY : BusState::UNREGISTERED;
  return true;
}

bool BusManager::claim(BusType type, uint8_t index, const char* owner) {
  int8_t idx = find(type, index);
  if (idx < 0 || slots_[idx].state == BusState::BUSY) return false;
  strncpy(slots_[idx].owner, owner ? owner : "", sizeof(slots_[idx].owner) - 1);
  slots_[idx].owner[sizeof(slots_[idx].owner) - 1] = '\0';
  slots_[idx].state = BusState::BUSY;
  return true;
}

bool BusManager::release(BusType type, uint8_t index, const char* owner) {
  int8_t idx = find(type, index);
  if (idx < 0) return false;
  // Solo el propietario registrado libera el bus (evita liberar bus ajeno).
  if (owner && strncmp(slots_[idx].owner, owner, sizeof(slots_[idx].owner)) != 0)
    return false;
  slots_[idx].owner[0] = '\0';
  slots_[idx].state = BusState::READY;
  return true;
}

bool BusManager::isRegistered(BusType type, uint8_t index) const {
  return find(type, index) >= 0;
}

bool BusManager::isBusy(BusType type, uint8_t index) const {
  int8_t idx = find(type, index);
  return idx >= 0 && slots_[idx].state == BusState::BUSY;
}

const char* BusManager::owner(BusType type, uint8_t index) const {
  int8_t idx = find(type, index);
  return idx >= 0 ? slots_[idx].owner : "";
}

BusState BusManager::state(BusType type, uint8_t index) const {
  int8_t idx = find(type, index);
  return idx >= 0 ? slots_[idx].state : BusState::UNREGISTERED;
}

uint8_t BusManager::scanI2c(uint8_t* found, uint8_t maxFound, uint8_t index) {
  (void)index;  // un único bus I²C en V8.0
  uint8_t n = 0;
  for (uint8_t addr = 1; addr < 127 && n < maxFound; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) found[n++] = addr;
  }
  return n;
}

String BusManager::toJson() const {
  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < count_; i++) {
    const Slot& s = slots_[i];
    JsonObject o = arr.createNestedObject();
    o["type"] = busTypeString(s.type);
    o["index"] = s.index;
    o["enabled"] = s.enabled;
    o["state"] = busStateString(s.state);
    if (s.owner[0]) o["owner"] = s.owner;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

int8_t BusManager::find(BusType type, uint8_t index) const {
  for (uint8_t i = 0; i < count_; i++) {
    if (slots_[i].type == type && slots_[i].index == index) return i;
  }
  return -1;
}

} // namespace gh
