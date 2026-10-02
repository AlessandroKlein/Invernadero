#include "hardware/HardwareManager.hpp"

#include <ArduinoJson.h>

#include "core/PinMap.hpp"

namespace gh {

void HardwareManager::begin() {
  buses_.begin();

  // Nodos estáticos del PCB de referencia. El catálogo puede ampliarse por
  // configuración (expansores/ADC/SD/W5500 adicionales en V8.1).
  HardwareNode n;

  n.id[0] = '\0';
  strncpy(n.id, "hc595-0", sizeof(n.id) - 1);
  n.kind = HardwareKind::HC595;
  n.bus = BusType::SPI;
  n.busIndex = 0;
  n.address = 0;
  n.enabled = true;
  strncpy(n.owner, "actuators", sizeof(n.owner) - 1);
  registerNode(n);

  n.id[0] = '\0';
  strncpy(n.id, "mcp23017-0", sizeof(n.id) - 1);
  n.kind = HardwareKind::MCP23017;
  n.bus = BusType::I2C;
  n.busIndex = 0;
  n.address = pins::I2C_ADDR_MCP23017_1;
  n.enabled = true;
  strncpy(n.owner, "actuators", sizeof(n.owner) - 1);
  registerNode(n);
}

bool HardwareManager::registerNode(const HardwareNode& n) {
  if (n.id[0] == '\0') return false;
  int8_t idx = findNode(n.id);
  if (idx >= 0) {
    nodes_[idx] = n;               // actualiza
    return true;
  }
  if (nodeCount_ >= MAX_NODES) return false;
  nodes_[nodeCount_++] = n;
  return true;
}

bool HardwareManager::setNodeEnabled(const char* id, bool enabled) {
  int8_t idx = findNode(id);
  if (idx < 0) return false;
  nodes_[idx].enabled = enabled;
  return true;
}

bool HardwareManager::getNode(const char* id, HardwareNode& out) const {
  int8_t idx = findNode(id);
  if (idx < 0) return false;
  out = nodes_[idx];
  return true;
}

void HardwareManager::snapshot(HardwareNode* out, size_t max, size_t& n) const {
  n = 0;
  for (uint8_t i = 0; i < nodeCount_ && n < max; i++) out[n++] = nodes_[i];
}

String HardwareManager::toJson() const {
  // El estado de los buses se expone por separado vía BusManager::toJson();
  // aquí solo se serializa el catálogo de nodos de hardware.
  DynamicJsonDocument doc(2048);
  JsonArray nodes = doc.to<JsonArray>();
  for (uint8_t i = 0; i < nodeCount_; i++) {
    const HardwareNode& n = nodes_[i];
    JsonObject o = nodes.createNestedObject();
    o["id"] = n.id;
    o["kind"] = hardwareKindString(n.kind);
    o["bus"] = busTypeString(n.bus);
    o["bus_index"] = n.busIndex;
    o["address"] = n.address;
    o["enabled"] = n.enabled;
    if (n.owner[0]) o["owner"] = n.owner;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

int8_t HardwareManager::findNode(const char* id) const {
  if (id == nullptr) return -1;
  for (uint8_t i = 0; i < nodeCount_; i++) {
    if (strncmp(nodes_[i].id, id, sizeof(nodes_[i].id)) == 0) return i;
  }
  return -1;
}

} // namespace gh
