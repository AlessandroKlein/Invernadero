#pragma once
// Expansor de GPIO I²C MCP23017 (16 E/S bidireccionales, sección 23).
// Permite hasta 8 dispositivos en el bus mediante A0/A1/A2 (0x20..0x27).

#include <Arduino.h>
#include <Wire.h>

namespace gh {

class Mcp23017 {
public:
  // Dirección I²C (0x20..0x27) y opcionalmente un bus Wire personalizado.
  void begin(uint8_t addr, TwoWire* wire = &Wire);

  void pinMode(uint8_t pin, uint8_t mode);  // INPUT/OUTPUT
  void digitalWrite(uint8_t pin, uint8_t val);
  uint8_t digitalRead(uint8_t pin);
  void setPullup(uint8_t pin, bool enabled);
  void writePort(uint8_t port, uint8_t val); // port 0=A, 1=B
  uint8_t readPort(uint8_t port);

private:
  uint8_t addr_ = 0x20;
  TwoWire* wire_ = nullptr;
  void writeReg(uint8_t reg, uint8_t val);
  uint8_t readReg(uint8_t reg);
};

} // namespace gh
