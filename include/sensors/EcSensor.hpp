#pragma once
// Conductividad eléctrica (EC), sección 19. Opcional.
// Prioriza la lectura vía Modbus RTU (transmisor industrial); admite una
// entrada analógica (ADS1115) como estimación básica.

#include <Arduino.h>
#include "hardware/ModbusRtu.hpp"
#include "sensors/Ads1115Driver.hpp"

namespace gh {

class EcSensor {
public:
  enum class Interface : uint8_t { MODBUS_IF, ANALOG_IF }; // (evita macro ANALOG de Arduino)

  void setInterface(Interface iface) { iface_ = iface; }
  void setAdsChannel(uint8_t ch) { adsChannel_ = ch; }
  void setModbus(uint8_t slaveId, uint16_t reg, float scale = 0.001f); // µS -> mS
  void begin(Ads1115Driver* ads, ModbusRtu* modbus);

  bool read(float& ecMsCm); // Devuelve mS/cm; false si no es válido

private:
  Interface iface_ = Interface::MODBUS_IF;
  uint8_t adsChannel_ = 0;
  uint8_t slaveId_ = 1;
  uint16_t reg_ = 0;
  float modbusScale_ = 0.001f;
  Ads1115Driver* ads_ = nullptr;
  ModbusRtu* modbus_ = nullptr;
};

} // namespace gh
