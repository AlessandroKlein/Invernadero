#pragma once
// Valores predeterminados de fábrica (sección 84).
// Todos los actuadores de riesgo arrancan deshabilitados hasta configuración.

#include "core/Types.hpp"

namespace gh {

// Devuelve una configuración de fábrica segura.
inline SystemConfig defaults() {
  SystemConfig c; // ya inicializa con valores seguros en Types.hpp
  // Riego automático deshabilitado hasta configuración (evita actuación accidental).
  c.featureIrrigation = true;
  c.sensorSht31 = true;
  c.sensorDs18b20 = true;
  c.sensorSoil = true;
  c.sensorLight = true;
  c.sensorTank = true;
  c.sensorFlow = true;
  c.actPump = true;
  c.actValves = 4;
  c.actFans = 2;
  return c;
}

} // namespace gh
