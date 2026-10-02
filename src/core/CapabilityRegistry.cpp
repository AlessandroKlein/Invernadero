#include "core/CapabilityRegistry.hpp"

#include <ArduinoJson.h>

namespace gh {

bool CapabilityRegistry::add(const char* cap) {
  if (cap == nullptr || cap[0] == '\0') return false;
  if (find(cap) >= 0) return true;          // ya existe
  if (count_ >= MAX_CAPABILITIES) return false;
  strncpy(caps_[count_], cap, CAP_MAX_LEN - 1);
  caps_[count_][CAP_MAX_LEN - 1] = '\0';
  count_++;
  return true;
}

bool CapabilityRegistry::remove(const char* cap) {
  int8_t idx = find(cap);
  if (idx < 0) return false;
  // Compactar el array sin preservar orden (el orden no es semántico).
  if (idx != count_ - 1) {
    memcpy(caps_[idx], caps_[count_ - 1], CAP_MAX_LEN);
  }
  caps_[count_ - 1][0] = '\0';
  count_--;
  return true;
}

bool CapabilityRegistry::has(const char* cap) const {
  return find(cap) >= 0;
}

void CapabilityRegistry::clear() {
  for (uint8_t i = 0; i < MAX_CAPABILITIES; i++) caps_[i][0] = '\0';
  count_ = 0;
}

void CapabilityRegistry::syncFrom(const DeviceInfo& info) {
  for (uint8_t i = 0; i < info.capabilityCount; i++) add(info.capabilities[i]);
}

void CapabilityRegistry::snapshot(char out[MAX_CAPABILITIES][CAP_MAX_LEN], uint8_t& n) const {
  n = 0;
  for (uint8_t i = 0; i < count_; i++) {
    strncpy(out[n], caps_[i], CAP_MAX_LEN - 1);
    out[n][CAP_MAX_LEN - 1] = '\0';
    n++;
  }
}

String CapabilityRegistry::toJson() const {
  DynamicJsonDocument doc(512);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < count_; i++) arr.add(caps_[i]);
  String out;
  serializeJson(doc, out);
  return out;
}

int8_t CapabilityRegistry::find(const char* cap) const {
  if (cap == nullptr) return -1;
  for (uint8_t i = 0; i < count_; i++) {
    if (strncmp(caps_[i], cap, CAP_MAX_LEN) == 0) return i;
  }
  return -1;
}

} // namespace gh
