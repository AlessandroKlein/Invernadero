#include "core/Scheduler.hpp"

#include <ArduinoJson.h>

namespace gh {

bool Scheduler::add(const char* name, uint32_t intervalMs, TaskFn fn, void* ctx) {
  if (name == nullptr || name[0] == '\0' || fn == nullptr) return false;
  int8_t idx = find(name);
  if (idx < 0) {
    if (count_ >= MAX_TASKS) return false;
    idx = count_++;
  }
  strncpy(tasks_[idx].name, name, sizeof(tasks_[idx].name) - 1);
  tasks_[idx].name[sizeof(tasks_[idx].name) - 1] = '\0';
  tasks_[idx].intervalMs = intervalMs;
  tasks_[idx].lastRunMs = millis();  // no ejecutar inmediatamente
  tasks_[idx].enabled = true;
  tasks_[idx].fn = fn;
  tasks_[idx].ctx = ctx;
  return true;
}

bool Scheduler::remove(const char* name) {
  int8_t idx = find(name);
  if (idx < 0) return false;
  if (idx != count_ - 1) tasks_[idx] = tasks_[count_ - 1];
  count_--;
  return true;
}

bool Scheduler::setEnabled(const char* name, bool enabled) {
  int8_t idx = find(name);
  if (idx < 0) return false;
  tasks_[idx].enabled = enabled;
  return true;
}

void Scheduler::tick() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < count_; i++) {
    Task& t = tasks_[i];
    if (!t.enabled || t.fn == nullptr) continue;
    if (now - t.lastRunMs >= t.intervalMs) {
      t.fn(t.ctx);
      t.lastRunMs = now;
    }
  }
}

String Scheduler::toJson() const {
  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < count_; i++) {
    const Task& t = tasks_[i];
    JsonObject o = arr.createNestedObject();
    o["name"] = t.name;
    o["interval_ms"] = t.intervalMs;
    o["enabled"] = t.enabled;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

int8_t Scheduler::find(const char* name) const {
  if (name == nullptr) return -1;
  for (uint8_t i = 0; i < count_; i++) {
    if (strncmp(tasks_[i].name, name, sizeof(tasks_[i].name)) == 0) return i;
  }
  return -1;
}

} // namespace gh
