#pragma once
// Variables calculadas (sección 235): VPD (déficit de presión de vapor) y punto
// de rocío. Funciones puras e independientes del hardware, usadas tanto por el
// motor de reglas local como (en el servidor) para históricos/análisis.

#include <Arduino.h>
#include <math.h>

namespace gh {
namespace calc {

// Presión de vapor de saturación (kPa) — fórmula de Magnus.
inline float saturationVaporPressure(float tCelsius) {
  if (isnan(tCelsius)) return NAN;
  return 0.6108f * expf(17.27f * tCelsius / (tCelsius + 237.3f));
}

// VPD en kPa a partir de temperatura (°C) y humedad relativa (%).
// Devuelve NAN si alguna entrada es inválida (sección 176: nunca VPD = 0 por
// falta de datos; debe ser INVALID).
inline float vpd(float tCelsius, float rhPercent) {
  if (isnan(tCelsius) || isnan(rhPercent)) return NAN;
  if (rhPercent < 0.0f || rhPercent > 100.0f) return NAN;
  return saturationVaporPressure(tCelsius) * (1.0f - rhPercent / 100.0f);
}

// Punto de rocío en °C (aproximación de Magnus). Devuelve NAN si inválido.
inline float dewPoint(float tCelsius, float rhPercent) {
  if (isnan(tCelsius) || isnan(rhPercent)) return NAN;
  if (rhPercent <= 0.0f || rhPercent > 100.0f) return NAN;
  float a = 17.27f;
  float b = 237.3f;
  float gamma = logf(rhPercent / 100.0f) + a * tCelsius / (tCelsius + b);
  return b * gamma / (a - gamma);
}

} // namespace calc
} // namespace gh
