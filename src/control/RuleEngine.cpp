#include "control/RuleEngine.hpp"

#include <ArduinoJson.h>

namespace gh {

// --- Conversiones de enums a cadena ---

const char* ruleVariableString(RuleVariable v) {
  switch (v) {
    case RuleVariable::TEMPERATURE:    return "temperature";
    case RuleVariable::HUMIDITY:       return "humidity";
    case RuleVariable::SOIL:           return "soil";
    case RuleVariable::LIGHT:          return "light";
    case RuleVariable::CO2:            return "co2";
    case RuleVariable::TANK:           return "tank";
    case RuleVariable::FLOW:           return "flow";
    case RuleVariable::PH:             return "ph";
    case RuleVariable::EC:             return "ec";
    case RuleVariable::VPD:            return "vpd";
    case RuleVariable::DEWPOINT:       return "dewpoint";
    case RuleVariable::EXTERIOR_TEMP:  return "exterior_temp";
    case RuleVariable::EXTERIOR_HUM:   return "exterior_hum";
    case RuleVariable::WIND_SPEED:     return "wind_speed";
    case RuleVariable::RAIN:           return "rain";
    default:                           return "unknown";
  }
}

const char* ruleOpString(RuleOp o) {
  switch (o) {
    case RuleOp::GT:  return "gt";
    case RuleOp::LT:  return "lt";
    case RuleOp::GTE: return "gte";
    case RuleOp::LTE: return "lte";
    case RuleOp::EQ:  return "eq";
    default:          return "gt";
  }
}

bool ruleVariableFromString(const char* s, RuleVariable& out) {
  if (!s) return false;
  String str(s);
  str.toLowerCase();
  if (str == "temperature" || str == "temp") { out = RuleVariable::TEMPERATURE; return true; }
  if (str == "humidity" || str == "hum") { out = RuleVariable::HUMIDITY; return true; }
  if (str == "soil") { out = RuleVariable::SOIL; return true; }
  if (str == "light" || str == "lux") { out = RuleVariable::LIGHT; return true; }
  if (str == "co2") { out = RuleVariable::CO2; return true; }
  if (str == "tank" || str == "level") { out = RuleVariable::TANK; return true; }
  if (str == "flow") { out = RuleVariable::FLOW; return true; }
  if (str == "ph") { out = RuleVariable::PH; return true; }
  if (str == "ec") { out = RuleVariable::EC; return true; }
  if (str == "vpd") { out = RuleVariable::VPD; return true; }
  if (str == "dewpoint" || str == "dew") { out = RuleVariable::DEWPOINT; return true; }
  if (str == "exterior_temp") { out = RuleVariable::EXTERIOR_TEMP; return true; }
  if (str == "exterior_hum") { out = RuleVariable::EXTERIOR_HUM; return true; }
  if (str == "wind" || str == "wind_speed") { out = RuleVariable::WIND_SPEED; return true; }
  if (str == "rain") { out = RuleVariable::RAIN; return true; }
  return false;
}

bool ruleOpFromString(const char* s, RuleOp& out) {
  if (!s) return false;
  String str(s);
  str.toLowerCase();
  if (str == "gt" || str == ">") { out = RuleOp::GT; return true; }
  if (str == "lt" || str == "<") { out = RuleOp::LT; return true; }
  if (str == "gte" || str == ">=") { out = RuleOp::GTE; return true; }
  if (str == "lte" || str == "<=") { out = RuleOp::LTE; return true; }
  if (str == "eq" || str == "==" || str == "=") { out = RuleOp::EQ; return true; }
  return false;
}

// --- Lectura de variables desde el SensorManager ---

float RuleEngine::readVariable(RuleVariable v, uint8_t zone) const {
  if (!sensors_) return NAN;
  switch (v) {
    case RuleVariable::TEMPERATURE:   return sensors_->temperature();
    case RuleVariable::HUMIDITY:      return sensors_->humidity();
    case RuleVariable::SOIL:          return sensors_->soilMoisture(zone);
    case RuleVariable::LIGHT:         return sensors_->lightLux();
    case RuleVariable::CO2:           return sensors_->co2();
    case RuleVariable::TANK:          return sensors_->tankLevel();
    case RuleVariable::FLOW:          return sensors_->flowRate();
    case RuleVariable::PH:            return sensors_->ph();
    case RuleVariable::EC:            return sensors_->ec();
    case RuleVariable::VPD:           return sensors_->vpd();
    case RuleVariable::DEWPOINT:      return sensors_->dewPoint();
    case RuleVariable::EXTERIOR_TEMP: return sensors_->exteriorTemperature();
    case RuleVariable::EXTERIOR_HUM:  return sensors_->exteriorHumidity();
    case RuleVariable::WIND_SPEED:    return sensors_->windSpeed();
    case RuleVariable::RAIN:          return sensors_->rainRate();
    default:                          return NAN;
  }
}

// --- Evaluación de la condición ---

bool RuleEngine::evaluate(RuleOp op, float value, float threshold) {
  if (isnan(value) || isnan(threshold)) return false;
  switch (op) {
    case RuleOp::GT:  return value >  threshold;
    case RuleOp::LT:  return value <  threshold;
    case RuleOp::GTE: return value >= threshold;
    case RuleOp::LTE: return value <= threshold;
    case RuleOp::EQ:  return fabsf(value - threshold) < 1e-4f;
  }
  return false;
}

// --- Bucle de evaluación ---

void RuleEngine::update() {
  if (!actuators_ || !mutex_) return;
  // Copia local de las reglas para minimizar el tiempo dentro del mutex.
  AutomationRule local[MAX_RULES];
  uint8_t n = 0;
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    n = count_;
    for (uint8_t i = 0; i < n; i++) local[i] = rules_[i];
    xSemaphoreGive(mutex_);
  }

  for (uint8_t i = 0; i < n; i++) {
    const AutomationRule& r = local[i];
    if (!r.enabled) continue;
    float v = readVariable(r.variable, r.zone);
    if (isnan(v)) continue; // No actuar sin lectura válida (sección 56/176)
    if (evaluate(r.op, v, r.threshold)) {
      actuators_->setRequest(r.action, r.actionIndex, r.actionValue);
    }
  }
}

// --- Gestión de reglas ---

bool RuleEngine::addRule(const AutomationRule& r) {
  if (!mutex_) return false;
  bool ok = false;
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    if (count_ < MAX_RULES) { rules_[count_++] = r; ok = true; }
    xSemaphoreGive(mutex_);
  }
  return ok;
}

bool RuleEngine::removeRule(uint8_t index) {
  if (!mutex_) return false;
  bool ok = false;
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    if (index < count_) {
      for (uint8_t i = index; i < count_ - 1; i++) rules_[i] = rules_[i + 1];
      count_--;
      ok = true;
    }
    xSemaphoreGive(mutex_);
  }
  return ok;
}

void RuleEngine::clear() {
  if (!mutex_) return;
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    count_ = 0;
    xSemaphoreGive(mutex_);
  }
}

uint8_t RuleEngine::count() const {
  uint8_t n = 0;
  if (mutex_ && xSemaphoreTake(mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    n = count_;
    xSemaphoreGive(mutex_);
  }
  return n;
}

void RuleEngine::snapshot(AutomationRule* out, size_t max, size_t& n) const {
  n = 0;
  if (!out || !mutex_) return;
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    n = count_ < max ? count_ : max;
    for (size_t i = 0; i < n; i++) out[i] = rules_[i];
    xSemaphoreGive(mutex_);
  }
}

String RuleEngine::toJson() const {
  AutomationRule local[MAX_RULES];
  size_t n = 0;
  snapshot(local, MAX_RULES, n);
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.createNestedArray("rules");
  for (size_t i = 0; i < n; i++) {
    const AutomationRule& r = local[i];
    JsonObject o = arr.createNestedObject();
    o["name"] = r.name;
    o["enabled"] = r.enabled;
    o["variable"] = ruleVariableString(r.variable);
    o["op"] = ruleOpString(r.op);
    o["threshold"] = r.threshold;
    o["action"] = (int)r.action;
    o["action_index"] = r.actionIndex;
    o["zone"] = r.zone;
    o["action_value"] = r.actionValue;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh


