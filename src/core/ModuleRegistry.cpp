#include "core/ModuleRegistry.hpp"

#include <ArduinoJson.h>

#include "core/Version.hpp"

namespace gh {

bool ModuleRegistry::registerModule(const ModuleDescriptor& m) {
  if (m.id[0] == '\0') return false;
  int8_t idx = find(m.id);
  if (idx >= 0) {
    modules_[idx] = m;               // actualiza el descriptor
    return true;
  }
  if (count_ >= MAX_MODULES) return false;
  modules_[count_++] = m;
  return true;
}

void ModuleRegistry::registerBuiltins() {
  // Módulos embebidos de la plataforma. El id es estable; la versión se hereda
  // del firmware. Las capacidades describen qué expone cada módulo a la UI.
  const struct Builtin {
    const char* id;
    const char* caps[4];
    uint8_t capCount;
  } defs[] = {
    {"core",      {}, 0},
    {"config",    {}, 0},
    {"sensors",   {"temperature", "humidity", "co2", "soil"}, 4},
    {"actuators", {"relay", "pwm", "valve", "pump"}, 4},
    {"network",   {"wifi"}, 1},
    {"mqtt",      {"mqtt"}, 1},
    {"rules",     {"automation"}, 1},
    {"safety",    {"safety"}, 1},
    {"ota",       {"ota"}, 1},
    {"storage",   {"history"}, 1},
  };

  for (const Builtin& d : defs) {
    ModuleDescriptor m;
    strncpy(m.id, d.id, sizeof(m.id) - 1);
    strncpy(m.version, GH_FW_VERSION, sizeof(m.version) - 1);
    for (uint8_t i = 0; i < d.capCount && i < 6; i++) {
      strncpy(m.capabilities[i], d.caps[i], sizeof(m.capabilities[i]) - 1);
    }
    m.capabilityCount = d.capCount;
    m.state = ModuleState::MODULE_ENABLED;
    registerModule(m);
  }
}

bool ModuleRegistry::setState(const char* id, ModuleState s) {
  int8_t idx = find(id);
  if (idx < 0) return false;
  modules_[idx].state = s;
  return true;
}

bool ModuleRegistry::get(const char* id, ModuleDescriptor& out) const {
  int8_t idx = find(id);
  if (idx < 0) return false;
  out = modules_[idx];
  return true;
}

bool ModuleRegistry::isEnabled(const char* id) const {
  int8_t idx = find(id);
  return idx >= 0 && modules_[idx].state == ModuleState::MODULE_ENABLED;
}

bool ModuleRegistry::dependenciesSatisfied(const char* id) const {
  int8_t idx = find(id);
  if (idx < 0) return false;
  for (uint8_t d = 0; d < modules_[idx].dependencyCount; d++) {
    if (find(modules_[idx].dependencies[d]) < 0) return false;
  }
  return true;
}

void ModuleRegistry::snapshot(ModuleDescriptor* out, size_t max, size_t& n) const {
  n = 0;
  for (uint8_t i = 0; i < count_ && n < max; i++) out[n++] = modules_[i];
}

String ModuleRegistry::toJson() const {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < count_; i++) {
    const ModuleDescriptor& m = modules_[i];
    JsonObject o = arr.createNestedObject();
    o["id"] = m.id;
    o["version"] = m.version;
    o["state"] = moduleStateString(m.state);
    if (m.dependencyCount) {
      JsonArray deps = o.createNestedArray("dependencies");
      for (uint8_t d = 0; d < m.dependencyCount; d++) deps.add(m.dependencies[d]);
    }
    if (m.capabilityCount) {
      JsonArray caps = o.createNestedArray("capabilities");
      for (uint8_t c = 0; c < m.capabilityCount; c++) caps.add(m.capabilities[c]);
    }
  }
  String out;
  serializeJson(doc, out);
  return out;
}

int8_t ModuleRegistry::find(const char* id) const {
  if (id == nullptr) return -1;
  for (uint8_t i = 0; i < count_; i++) {
    if (strncmp(modules_[i].id, id, sizeof(modules_[i].id)) == 0) return i;
  }
  return -1;
}

} // namespace gh
