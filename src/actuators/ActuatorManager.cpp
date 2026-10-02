#include "actuators/ActuatorManager.hpp"

#include <ArduinoJson.h>

namespace gh {

// Mapa por defecto de roles a canales lógicos del 74HC595.
// El usuario no ve canales físicos, solo nombres (sección 39).
struct RoleDef {
  ActuatorRole role;
  uint8_t index;
  const char* name;
  uint8_t channel;
  OutputKind kind;
};

static const RoleDef ROLE_TABLE[] = {
  { ActuatorRole::PUMP, 0, "Bomba", 0, OutputKind::DIGITAL },
  { ActuatorRole::VALVE, 0, "Válvula 1", 1, OutputKind::DIGITAL },
  { ActuatorRole::VALVE, 1, "Válvula 2", 2, OutputKind::DIGITAL },
  { ActuatorRole::VALVE, 2, "Válvula 3", 3, OutputKind::DIGITAL },
  { ActuatorRole::VALVE, 3, "Válvula 4", 4, OutputKind::DIGITAL },
  { ActuatorRole::VALVE, 4, "Válvula 5", 5, OutputKind::DIGITAL },
  { ActuatorRole::VALVE, 5, "Válvula 6", 6, OutputKind::DIGITAL },
  { ActuatorRole::VALVE, 6, "Válvula 7", 7, OutputKind::DIGITAL },
  { ActuatorRole::VALVE, 7, "Válvula 8", 8, OutputKind::DIGITAL },
  { ActuatorRole::FAN, 0, "Ventilador 1", 9, OutputKind::PWM },
  { ActuatorRole::FAN, 1, "Ventilador 2", 10, OutputKind::PWM },
  { ActuatorRole::FAN, 2, "Ventilador 3", 11, OutputKind::PWM },
  { ActuatorRole::EXTRACTOR, 0, "Extractor 1", 12, OutputKind::PWM },
  { ActuatorRole::EXTRACTOR, 1, "Extractor 2", 13, OutputKind::PWM },
  { ActuatorRole::HEATER, 0, "Calefacción", 14, OutputKind::DIGITAL },
  { ActuatorRole::HUMIDIFIER, 0, "Humidificador", 15, OutputKind::DIGITAL },
  { ActuatorRole::LIGHT, 0, "Iluminación 1", 16, OutputKind::PWM },
  { ActuatorRole::LIGHT, 1, "Iluminación 2", 17, OutputKind::PWM },
  { ActuatorRole::WINDOW_OPEN, 0, "Ventana abrir", 18, OutputKind::DIGITAL },
  { ActuatorRole::WINDOW_CLOSE, 0, "Ventana cerrar", 19, OutputKind::DIGITAL },
  { ActuatorRole::ROOF_OPEN, 0, "Techo abrir", 20, OutputKind::DIGITAL },
  { ActuatorRole::ROOF_CLOSE, 0, "Techo cerrar", 21, OutputKind::DIGITAL },
  { ActuatorRole::SHADE_OPEN, 0, "Sombra abrir", 22, OutputKind::DIGITAL },
  { ActuatorRole::SHADE_CLOSE, 0, "Sombra cerrar", 23, OutputKind::DIGITAL },
  { ActuatorRole::ALARM, 0, "Alarma", 24, OutputKind::DIGITAL },
};
static const uint8_t ROLE_TABLE_SIZE = sizeof(ROLE_TABLE) / sizeof(ROLE_TABLE[0]);

void ActuatorManager::begin(const SystemConfig& cfg, ShiftRegister595* shift, Mcp23017* mcpPool, uint8_t mcpCount) {
  cfg_ = cfg;
  shift_ = shift;
  mcpCount_ = mcpCount > 4 ? 4 : mcpCount;
  for (uint8_t i = 0; i < mcpCount_; i++) mcpPool_[i] = &mcpPool[i];
  if (mutex_ == nullptr) mutex_ = xSemaphoreCreateMutex();

  // Construir los slots de actuadores a partir de la tabla de roles.
  count_ = 0;
  for (uint8_t i = 0; i < ROLE_TABLE_SIZE && count_ < MAX_ACTUATORS; i++) {
    ActuatorState s;
    s.role = ROLE_TABLE[i].role;
    s.index = ROLE_TABLE[i].index;
    s.name = ROLE_TABLE[i].name;
    s.kind = ROLE_TABLE[i].kind;
    s.output = 0;
    s.requested = false;
    slots_[count_++] = s;
  }
  reconfigure(cfg);
}

void ActuatorManager::reconfigure(const SystemConfig& cfg) {
  cfg_ = cfg;
  // Marcar habilitados según la configuración de actuadores.
  for (uint8_t i = 0; i < count_; i++) {
    bool en = false;
    switch (slots_[i].role) {
      case ActuatorRole::PUMP: en = cfg_.actPump; break;
      case ActuatorRole::VALVE: en = slots_[i].index < cfg_.actValves; break;
      case ActuatorRole::FAN: en = slots_[i].index < cfg_.actFans; break;
      case ActuatorRole::EXTRACTOR: en = slots_[i].index < cfg_.actExtractors; break;
      case ActuatorRole::HEATER: en = cfg_.actHeater; break;
      case ActuatorRole::HUMIDIFIER: en = cfg_.actHumidifier; break;
      case ActuatorRole::LIGHT: en = slots_[i].index < cfg_.actLights; break;
      case ActuatorRole::WINDOW_OPEN:
      case ActuatorRole::WINDOW_CLOSE: en = cfg_.actWindow; break;
      case ActuatorRole::ROOF_OPEN:
      case ActuatorRole::ROOF_CLOSE: en = cfg_.actRoof; break;
      case ActuatorRole::SHADE_OPEN:
      case ActuatorRole::SHADE_CLOSE: en = cfg_.actShade; break;
      case ActuatorRole::ALARM: en = true; break;
      default: break;
    }
    slots_[i].enabled = en;
  }
}

int8_t ActuatorManager::findSlot(ActuatorRole role, uint8_t index) const {
  for (uint8_t i = 0; i < count_; i++) {
    if (slots_[i].role == role && slots_[i].index == index) return i;
  }
  return -1;
}

void ActuatorManager::setRequest(ActuatorRole role, uint8_t index, float pct) {
  int8_t i = findSlot(role, index);
  if (i < 0) return;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  slots_[i].requested = (pct > 0);
  slots_[i].output = pct;
  slots_[i].lastChangeMs = millis();
}

void ActuatorManager::setSafetyOverride(ActuatorRole role, uint8_t index, float pct) {
  int8_t i = findSlot(role, index);
  if (i < 0) return;
  slots_[i].fault = (pct >= 0 && pct < 100); // marcar si fuerza apagado
  setRequest(role, index, pct < 0 ? slots_[i].output : pct);
}

void ActuatorManager::clearSafetyOverrides() {
  for (uint8_t i = 0; i < count_; i++) slots_[i].fault = false;
}

void ActuatorManager::writeChannel(const ActuatorState& s, float pct) {
  // Buscar el canal físico en la tabla de roles.
  uint8_t channel = 255;
  for (uint8_t k = 0; k < ROLE_TABLE_SIZE; k++) {
    if (ROLE_TABLE[k].role == s.role && ROLE_TABLE[k].index == s.index) { channel = ROLE_TABLE[k].channel; break; }
  }
  if (channel == 255) return;

  if (channel >= 32) {
    // Expansión I²C (MCP23017): canales 32..95 → device 0..3, pin 0..15.
    uint16_t idx = channel - 32;
    uint8_t dev = idx / 16;
    uint8_t pin = idx % 16;
    if (dev < mcpCount_ && mcpPool_[dev]) mcpPool_[dev]->digitalWrite(pin, pct > 0 ? HIGH : LOW);
  } else if (shift_) {
    // Expansión SPI (74HC595) con soft-PWM.
    shift_->setChannelPercent(channel, pct);
  }
}

void ActuatorManager::apply() {
  for (uint8_t i = 0; i < count_; i++) {
    ActuatorState& s = slots_[i];
    // Salidas deshabilitadas o en estado seguro -> 0.
    float target = s.enabled ? s.output : 0.0f;
    if (s.fault) target = 0.0f; // sobrescritura de seguridad prevalece
    writeChannel(s, target);
  }
  if (shift_) shift_->commit();
}

void ActuatorManager::allSafeState() {
  for (uint8_t i = 0; i < count_; i++) {
    slots_[i].output = 0;
    slots_[i].requested = false;
    writeChannel(slots_[i], 0);
  }
  if (shift_) shift_->commit();
}

float ActuatorManager::output(ActuatorRole role, uint8_t index) const {
  int8_t i = findSlot(role, index);
  return (i < 0) ? 0.0f : slots_[i].output;
}

bool ActuatorManager::isOn(ActuatorRole role, uint8_t index) const {
  return output(role, index) > 0;
}

void ActuatorManager::snapshot(ActuatorState* out, size_t max, size_t& n) const {
  if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
  n = 0;
  for (uint8_t i = 0; i < count_ && n < max; i++) {
    if (slots_[i].enabled) out[n++] = slots_[i];
  }
  if (mutex_) xSemaphoreGive(mutex_);
}

String ActuatorManager::toJson() const {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < count_; i++) {
    if (!slots_[i].enabled) continue;
    JsonObject o = arr.createNestedObject();
    o["name"] = slots_[i].name;
    o["role"] = (int)slots_[i].role;
    o["kind"] = (int)slots_[i].kind;
    o["output"] = slots_[i].output;
    o["fault"] = slots_[i].fault;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
