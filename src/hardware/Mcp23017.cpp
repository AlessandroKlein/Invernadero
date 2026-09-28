#include "hardware/Mcp23017.hpp"

namespace gh {

// Registros del MCP23017 (modo bank=0).
enum : uint8_t {
  REG_IODIRA = 0x00, REG_IODIRB = 0x01,
  REG_GPPUA  = 0x0C, REG_GPPUB  = 0x0D,
  REG_GPIOA  = 0x12, REG_GPIOB  = 0x13,
  REG_OLATA  = 0x14, REG_OLATB  = 0x15
};

void Mcp23017::begin(uint8_t addr, TwoWire* wire) {
  addr_ = addr;
  wire_ = wire ? wire : &Wire;
  // Por defecto todos los pines como entradas (estado seguro).
  writeReg(REG_IODIRA, 0xFF);
  writeReg(REG_IODIRB, 0xFF);
}

void Mcp23017::writeReg(uint8_t reg, uint8_t val) {
  wire_->beginTransmission(addr_);
  wire_->write(reg);
  wire_->write(val);
  wire_->endTransmission();
}

uint8_t Mcp23017::readReg(uint8_t reg) {
  wire_->beginTransmission(addr_);
  wire_->write(reg);
  wire_->endTransmission();
  wire_->requestFrom(addr_, (uint8_t)1);
  return wire_->available() ? wire_->read() : 0;
}

void Mcp23017::pinMode(uint8_t pin, uint8_t mode) {
  bool isB = pin >= 8;              // pines 8..15 -> puerto B
  uint8_t bit = pin % 8;
  uint8_t reg = isB ? REG_IODIRB : REG_IODIRA;
  uint8_t dir = readReg(reg);
  if (mode == OUTPUT) dir &= ~(1 << bit);   // 0 = salida
  else dir |= (1 << bit);                   // 1 = entrada
  writeReg(reg, dir);
}

void Mcp23017::digitalWrite(uint8_t pin, uint8_t val) {
  bool isB = pin >= 8;
  uint8_t bit = pin % 8;
  uint8_t reg = isB ? REG_OLATB : REG_OLATA;
  uint8_t olat = readReg(reg);
  if (val) olat |= (1 << bit);
  else olat &= ~(1 << bit);
  writeReg(reg, olat);
}

uint8_t Mcp23017::digitalRead(uint8_t pin) {
  bool isB = pin >= 8;
  uint8_t bit = pin % 8;
  uint8_t reg = isB ? REG_GPIOB : REG_GPIOA;
  return (readReg(reg) >> bit) & 1;
}

void Mcp23017::setPullup(uint8_t pin, bool enabled) {
  bool isB = pin >= 8;
  uint8_t bit = pin % 8;
  uint8_t reg = isB ? REG_GPPUB : REG_GPPUA;
  uint8_t gppu = readReg(reg);
  if (enabled) gppu |= (1 << bit);
  else gppu &= ~(1 << bit);
  writeReg(reg, gppu);
}

void Mcp23017::writePort(uint8_t port, uint8_t val) {
  writeReg(port == 0 ? REG_OLATA : REG_OLATB, val);
}

uint8_t Mcp23017::readPort(uint8_t port) {
  return readReg(port == 0 ? REG_GPIOA : REG_GPIOB);
}

} // namespace gh
