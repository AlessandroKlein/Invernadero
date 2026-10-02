#include "system/HealthMonitor.hpp"

#include <ArduinoJson.h>

namespace gh {

void HealthMonitor::begin() {
  for (uint8_t i = 0; i < MAX_TASKS; i++) tasks_[i].name[0] = '\0';
  count_ = 0;
}

void HealthMonitor::registerTask(const char* name, TaskHandle_t handle) {
  if (name == nullptr || name[0] == '\0') return;
  int8_t idx = find(name);
  if (idx < 0) {
    if (count_ >= MAX_TASKS) return;
    idx = count_++;
  }
  strncpy(tasks_[idx].name, name, sizeof(tasks_[idx].name) - 1);
  tasks_[idx].name[sizeof(tasks_[idx].name) - 1] = '\0';
  tasks_[idx].handle = handle;
  tasks_[idx].lastActivityMs = millis();
}

void HealthMonitor::touch(const char* name) {
  int8_t idx = find(name);
  if (idx >= 0) tasks_[idx].lastActivityMs = millis();
}

String HealthMonitor::toJson() const {
  DynamicJsonDocument doc(1536);
  doc["uptime_ms"] = (uint32_t)millis();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["min_free_heap"] = ESP.getMinFreeHeap();
  JsonArray arr = doc.createNestedArray("tasks");
  for (uint8_t i = 0; i < count_; i++) {
    const Entry& e = tasks_[i];
    JsonObject o = arr.createNestedObject();
    o["name"] = e.name;
    o["last_activity_ms"] = (uint32_t)e.lastActivityMs;
    // Sin heartbeat en 15 s se considera bloqueada (comparación con wrap-around).
    o["alive"] = (millis() - e.lastActivityMs) < 15000UL;
    if (e.handle) o["stack_hwm"] = (uint32_t)uxTaskGetStackHighWaterMark(e.handle);
  }
  String out;
  serializeJson(doc, out);
  return out;
}

int8_t HealthMonitor::find(const char* name) const {
  for (uint8_t i = 0; i < count_; i++) {
    if (strncmp(tasks_[i].name, name, sizeof(tasks_[i].name)) == 0) return i;
  }
  return -1;
}

} // namespace gh
