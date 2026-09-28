#pragma once
// Medición de pH (sección 18). Admite dos interfaces:
//  - ANALOG: electrodo DFRobot Gravity (SEN0161) leído vía ADS1115.
//  - MODBUS: transmisor RS485/Modbus RTU industrial.
// Incluye estado válido/no válido y calibración por soluciones patrón.

#include <Arduino.h>
#include "hardware/ModbusRtu.hpp"
#include "sensors/Ads1115Driver.hpp"

namespace gh {

class PhSensor {
public:
  enum class Interface : uint8_t { ANALOG_IF, MODBUS_IF }; // (evita macro ANALOG de Arduino)

  void setInterface(Interface iface) { iface_ = iface; }
  void setAdsChannel(uint8_t ch) { adsChannel_ = ch; }
  void setModbus(uint8_t slaveId, uint16_t reg, float scale = 0.01f);
  void setCalibration(float v4, float v7, float v10);
  void begin(Ads1115Driver* ads, ModbusRtu* modbus);

  bool read(float& ph);   // Devuelve false si el valor no es válido
  float voltageToPh(float v) const; // Conversión calibrada

private:
  Interface iface_ = Interface::ANALOG_IF;
  uint8_t adsChannel_ = 0;
  uint8_t slaveId_ = 1;
  uint16_t reg_ = 0;
  float modbusScale_ = 0.01f; // pH suele venir como entero pH*100
  float v4_ = 3.02f, v7_ = 2.51f, v10_ = 2.01f;
  Ads1115Driver* ads_ = nullptr;
  ModbusRtu* modbus_ = nullptr;
};

} // namespace gh
