#pragma once
// Medición continua del nivel del depósito con sensor ultrasónico impermeable
// (HC-SR04 o similar). Sección 14. Complementado con flotadores de seguridad.

#include <Arduino.h>

namespace gh {

class UltrasonicLevel {
public:
  void begin(uint8_t trigPin, uint8_t echoPin, float tankDepthCm = 100.0f);
  // Distancia en cm (NaN si hay error/eco fuera de rango).
  float readDistanceCm();
  // Nivel en % (0 = vacío, 100 = lleno) usando la profundidad calibrada.
  float readLevelPercent();

private:
  uint8_t trig_ = 0xFF, echo_ = 0xFF;
  float tankDepthCm_ = 100.0f;
};

} // namespace gh
