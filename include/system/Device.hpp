#pragma once
// Identidad del dispositivo, máquina de estados, capacidades y causa de reinicio.
// Secciones 118/129/150/166/175/179. Centraliza la identidad permanente
// (UID derivado de la MAC), la lista de capacidades y el estado de funcionamiento.

#include <Arduino.h>
#include "core/Types.hpp"

namespace gh {

class Device {
public:
  // Identidad permanente derivada de la MAC (sección 118). Puntero estático.
  static const char* uid();

  // Construye la identidad completa (UID, perfil, versiones, capacidades).
  static DeviceInfo info(const SystemConfig& cfg);

  // Causa del último reinicio (sección 150).
  static ResetCause resetCause();

  // Máquina de estados del dispositivo (sección 175).
  static void setState(DeviceState s);
  static DeviceState state();

  // JSON para /api/v1/device y /api/v1/capabilities (sección 181).
  static String deviceJson(const SystemConfig& cfg);
  static String capabilitiesJson(const SystemConfig& cfg);

private:
  static DeviceState state_; // Estado actual (protegido por uso single-thread en setup)
};

} // namespace gh
