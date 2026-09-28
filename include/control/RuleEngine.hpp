#pragma once
// Motor de reglas configurable (secciones 121/122/237).
// Permite definir reglas "SI <variable> <op> <umbral> ENTONCES <actuador> = <%>"
// sin recompilar el firmware. Se evalúan en RAM (pueden añadirse vía API) y se
// suman a los controladores fijos (clima, riego, iluminación, techo, seguridad).
//
// Los interlocks de seguridad (sección 122) siguen viviendo en SafetyController y
// tienen prioridad sobre cualquier acción de estas reglas (sección 57/174).

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "core/Types.hpp"
#include "sensors/SensorManager.hpp"
#include "actuators/ActuatorManager.hpp"
#include "storage/History.hpp"

namespace gh {

// Conversiones a/desde cadena (usadas por la API REST).
const char* ruleVariableString(RuleVariable v);
const char* ruleOpString(RuleOp o);
bool ruleVariableFromString(const char* s, RuleVariable& out);
bool ruleOpFromString(const char* s, RuleOp& out);

class RuleEngine {
public:
  static constexpr uint8_t MAX_RULES = 16;

  void begin(SensorManager* s, ActuatorManager* a, History* h) {
    sensors_ = s; actuators_ = a; history_ = h;
    if (!mutex_) mutex_ = xSemaphoreCreateMutex();
  }

  // Evalúa todas las reglas habilitadas y aplica sus acciones.
  void update();

  // Gestión de reglas (protegida por mutex). Añade/sobrescribe por índice.
  bool addRule(const AutomationRule& r);
  bool removeRule(uint8_t index);
  void clear();
  uint8_t count() const;

  // Snapshot para la API.
  void snapshot(AutomationRule* out, size_t max, size_t& n) const;
  String toJson() const;

  // Evalúa una variable contra el sensor manager (para la API/diagnóstico).
  float readVariable(RuleVariable v, uint8_t zone) const;

private:
  SensorManager* sensors_ = nullptr;
  ActuatorManager* actuators_ = nullptr;
  History* history_ = nullptr;
  AutomationRule rules_[MAX_RULES];
  uint8_t count_ = 0;
  mutable SemaphoreHandle_t mutex_ = nullptr;

  static bool evaluate(RuleOp op, float value, float threshold);
};

} // namespace gh
