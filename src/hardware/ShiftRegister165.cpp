#include "hardware/ShiftRegister165.hpp"

namespace gh {

void ShiftRegister165::begin(int dataPin, int clockPin, int latchPin, uint8_t numChips) {
  dataPin_ = dataPin;
  clockPin_ = clockPin;
  latchPin_ = latchPin;
  numChips_ = numChips;
  pinMode(dataPin_, INPUT);
  pinMode(clockPin_, OUTPUT);
  pinMode(latchPin_, OUTPUT);
  digitalWrite(clockPin_, LOW);
  digitalWrite(latchPin_, HIGH);
}

uint8_t ShiftRegister165::readByte(uint8_t chip) {
  if (chip >= numChips_ || dataPin_ < 0) return 0;
  // Cargar las entradas paralelas y desplazarlas en serie (MSB first).
  digitalWrite(latchPin_, LOW);
  delayMicroseconds(2);
  digitalWrite(latchPin_, HIGH);
  return shiftIn(dataPin_, clockPin_, MSBFIRST);
}

uint32_t ShiftRegister165::read() {
  uint32_t val = 0;
  uint8_t n = numChips_ > 4 ? 4 : numChips_;  // máx. 32 bits
  for (uint8_t c = 0; c < n; c++) {
    val |= (uint32_t)readByte(c) << (c * 8);
  }
  return val;
}

bool ShiftRegister165::readBit(uint8_t bit) {
  if (bit >= numInputs()) return false;
  return (read() >> bit) & 1;
}

} // namespace gh
