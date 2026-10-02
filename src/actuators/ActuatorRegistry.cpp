#include "actuators/ActuatorRegistry.hpp"

#include <ArduinoJson.h>

namespace gh {

bool ActuatorRegistry::registerActuator(const ActuatorEntry& e) {
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

void ActuatorRegistry::buildFromConfig(const SystemConfig& cfg) {
  // Mapea los flags de configuración a entradas del catálogo. Los canales son
  // lógicos y el ActuatorManager resuelve el hardware real al aplicar la salida.
  auto add = [&](const char* id, const char* name, ActuatorRole role, OutputKind kind,
                 BusType bus, uint8_t channel, uint8_t zone, bool enabled, bool safeState) {
    ActuatorEntry e;
    strncpy(e.id, id, sizeof(e.id) - 1);
    strncpy(e.name, name, sizeof(e.name) - 1);
    e.role = role;
    e.kind = kind;
    e.bus = bus;
    e.channel = channel;
    e.zone = zone;
    e.enabled = enabled;
    e.safeState = safeState;
    registerActuator(e);
  };

  uint8_t ch = 0;
  if (cfg.actPump) add("pump", "Bomba", ActuatorRole::PUMP, OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, true);
  for (uint8_t i = 0; i < cfg.actValves && i < 8; i++) {
    char id[16], name[24];
    snprintf(id, sizeof(id), "valve_%u", i);
    snprintf(name, sizeof(name), "Válvula %u", i + 1);
    add(id, name, ActuatorRole::VALVE, OutputKind::DIGITAL, BusType::SPI, ch++, i, true, true);
  }
  for (uint8_t i = 0; i < cfg.actFans && i < 4; i++) {
    char id[16], name[24];
    snprintf(id, sizeof(id), "fan_%u", i);
    snprintf(name, sizeof(name), "Ventilador %u", i + 1);
    add(id, name, ActuatorRole::FAN, OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, false);
  }
  for (uint8_t i = 0; i < cfg.actExtractors && i < 4; i++) {
    char id[16], name[24];
    snprintf(id, sizeof(id), "extractor_%u", i);
    snprintf(name, sizeof(name), "Extractor %u", i + 1);
    add(id, name, ActuatorRole::EXTRACTOR, OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, false);
  }
  for (uint8_t i = 0; i < cfg.actLights && i < 4; i++) {
    char id[16], name[24];
    snprintf(id, sizeof(id), "light_%u", i);
    snprintf(name, sizeof(name), "Luz %u", i + 1);
    add(id, name, ActuatorRole::LIGHT, OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, false);
  }
  if (cfg.actHeater)     add("heater",     "Calefacción",  ActuatorRole::HEATER,     OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, false);
  if (cfg.actHumidifier) add("humidifier", "Humidificador", ActuatorRole::HUMIDIFIER, OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, false);
  if (cfg.actRoof) {
    add("roof_open",  "Techo abrir", ActuatorRole::ROOF_OPEN,  OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, true);
    add("roof_close", "Techo cerrar", ActuatorRole::ROOF_CLOSE, OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, true);
  }
  if (cfg.actWindow) {
    add("window_open",  "Ventana abrir", ActuatorRole::WINDOW_OPEN,  OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, true);
    add("window_close", "Ventana cerrar", ActuatorRole::WINDOW_CLOSE, OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, true);
  }
  if (cfg.actShade) {
    add("shade_open",  "Sombra abrir", ActuatorRole::SHADE_OPEN,  OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, true);
    add("shade_close", "Sombra cerrar", ActuatorRole::SHADE_CLOSE, OutputKind::DIGITAL, BusType::SPI, ch++, 0, true, true);
  }
}

bool ActuatorRegistry::remove(const char* id) {
  int8_t idx = find(id);
  if (idx < 0) return false;
  if (idx != count_ - 1) entries_[idx] = entries_[count_ - 1];
  count_--;
  return true;
}

bool ActuatorRegistry::get(const char* id, ActuatorEntry& out) const {
  int8_t idx = find(id);
  if (idx < 0) return false;
  out = entries_[idx];
  return true;
}

uint8_t ActuatorRegistry::countEnabled() const {
  uint8_t n = 0;
  for (uint8_t i = 0; i < count_; i++) if (entries_[i].enabled) n++;
  return n;
}

void ActuatorRegistry::snapshot(ActuatorEntry* out, size_t max, size_t& n) const {
  n = 0;
  for (uint8_t i = 0; i < count_ && n < max; i++) out[n++] = entries_[i];
}

String ActuatorRegistry::toJson() const {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < count_; i++) {
    const ActuatorEntry& e = entries_[i];
    JsonObject o = arr.createNestedObject();
    o["id"] = e.id;
    o["name"] = e.name;
    o["role"] = (int)e.role;
    o["kind"] = (e.kind == OutputKind::PWM) ? "PWM" : "DIGITAL";
    o["bus"] = busTypeString(e.bus);
    o["bus_index"] = e.busIndex;
    o["channel"] = e.channel;
    o["zone"] = e.zone;
    o["enabled"] = e.enabled;
    o["safe_state"] = e.safeState;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

int8_t ActuatorRegistry::find(const char* id) const {
  if (id == nullptr) return -1;
  for (uint8_t i = 0; i < count_; i++) {
    if (strncmp(entries_[i].id, id, sizeof(entries_[i].id)) == 0) return i;
  }
  return -1;
}

} // namespace gh
