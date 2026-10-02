#pragma once
// Expansión de entradas mediante registros 74HC165 (V8.1, README §227-228).
// Registro de desplazamiento paralelo→serie en cascada con 3 pines
// (DATA=Q7, CLOCK=SHCP, LATCH=PL). Lectura bit-banged por software.

#include <Arduino.h>

namespace gh {

class ShiftRegister165 {
public:
  // Inicializa pines. numChips registros en cascada (1..4 → 8..32 entradas).
  void begin(int dataPin, int clockPin, int latchPin, uint8_t numChips = 1);

  // Lee numChips*8 bits (máx. 32). El orden de bits depende del cableado.
  uint32_t read();
  uint16_t read16() { return (uint16_t)read(); }
  uint8_t readByte(uint8_t chip = 0);
  bool readBit(uint8_t bit);

  uint8_t numChips() const { return numChips_; }
  uint16_t numInputs() const { return numChips_ * 8; }

private:
  int dataPin_ = -1, clockPin_ = -1, latchPin_ = -1;
  uint8_t numChips_ = 1;
};

} // namespace gh
