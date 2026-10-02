#pragma once
// Expansor de GPIO SPI MCP23S17 (V8.1, README §229-230). 16 E/S bidireccionales,
// equivalente SPI del MCP23017. Hasta 8 dispositivos por CS/address (A0..A2).

#include <Arduino.h>
#include <SPI.h>

#include "hardware/SpiManager.hpp"

namespace gh {

class Mcp23s17 {
public:
  // bus: SpiManager ya inicializado; cs: pin CS del dispositivo; addr: A0..A2 (0..7).
  void begin(SpiManager* bus, uint8_t cs, uint8_t addr = 0);

  void pinMode(uint8_t pin, uint8_t mode);  // INPUT/OUTPUT
  void digitalWrite(uint8_t pin, uint8_t val);
  uint8_t digitalRead(uint8_t pin);
  void setPullup(uint8_t pin, bool enabled);
  void writePort(uint8_t port, uint8_t val); // port 0=A, 1=B
  uint8_t readPort(uint8_t port);

private:
  SpiManager* bus_ = nullptr;
  uint8_t cs_ = 0;
  uint8_t addr_ = 0;
  void writeReg(uint8_t reg, uint8_t val);
  uint8_t readReg(uint8_t reg);
};

} // namespace gh
