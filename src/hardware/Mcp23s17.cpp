#include "hardware/Mcp23s17.hpp"

namespace gh {

// Registros del MCP23S17 (bank=0), idénticos a los del MCP23017.
enum : uint8_t {
  SREG_IODIRA = 0x00, SREG_IODIRB = 0x01,
  SREG_GPPUA  = 0x0C, SREG_GPPUB  = 0x0D,
  SREG_GPIOA  = 0x12, SREG_GPIOB  = 0x13,
  SREG_OLATA  = 0x14, SREG_OLATB  = 0x15
};

// Byte de control: [0100 A2 A1 A0 R/W]. R/W=0 escritura, 1 lectura.
static uint8_t ctrlWrite(uint8_t addr) { return 0x40 | (addr << 1); }
static uint8_t ctrlRead(uint8_t addr)  { return 0x41 | (addr << 1); }

void Mcp23s17::begin(SpiManager* bus, uint8_t cs, uint8_t addr) {
  bus_ = bus;
  cs_ = cs;
  addr_ = addr & 0x07;
  pinMode(cs_, OUTPUT);
  digitalWrite(cs_, HIGH);
  // Por defecto todos los pines como entradas (estado seguro).
  writeReg(SREG_IODIRA, 0xFF);
  writeReg(SREG_IODIRB, 0xFF);
}

void Mcp23s17::writeReg(uint8_t reg, uint8_t val) {
  if (!bus_ || !bus_->isReady()) return;
  bus_->select(cs_);
  bus_->beginTransaction();
  bus_->bus().transfer(ctrlWrite(addr_));
  bus_->bus().transfer(reg);
  bus_->bus().transfer(val);
  bus_->endTransaction();
  bus_->deselect(cs_);
}

uint8_t Mcp23s17::readReg(uint8_t reg) {
  if (!bus_ || !bus_->isReady()) return 0;
  uint8_t v = 0;
  bus_->select(cs_);
  bus_->beginTransaction();
  bus_->bus().transfer(ctrlRead(addr_));
  bus_->bus().transfer(reg);
  v = bus_->bus().transfer(0x00);
  bus_->endTransaction();
  bus_->deselect(cs_);
  return v;
}

void Mcp23s17::pinMode(uint8_t pin, uint8_t mode) {
  bool isB = pin >= 8;
  uint8_t bit = pin % 8;
  uint8_t reg = isB ? SREG_IODIRB : SREG_IODIRA;
  uint8_t dir = readReg(reg);
  if (mode == OUTPUT) dir &= ~(1 << bit);
  else dir |= (1 << bit);
  writeReg(reg, dir);
}

void Mcp23s17::digitalWrite(uint8_t pin, uint8_t val) {
  bool isB = pin >= 8;
  uint8_t bit = pin % 8;
  uint8_t reg = isB ? SREG_OLATB : SREG_OLATA;
  uint8_t olat = readReg(reg);
  if (val) olat |= (1 << bit);
  else olat &= ~(1 << bit);
  writeReg(reg, olat);
}

uint8_t Mcp23s17::digitalRead(uint8_t pin) {
  bool isB = pin >= 8;
  uint8_t bit = pin % 8;
  uint8_t reg = isB ? SREG_GPIOB : SREG_GPIOA;
  return (readReg(reg) >> bit) & 1;
}

void Mcp23s17::setPullup(uint8_t pin, bool enabled) {
  bool isB = pin >= 8;
  uint8_t bit = pin % 8;
  uint8_t reg = isB ? SREG_GPPUB : SREG_GPPUA;
  uint8_t gppu = readReg(reg);
  if (enabled) gppu |= (1 << bit);
  else gppu &= ~(1 << bit);
  writeReg(reg, gppu);
}

void Mcp23s17::writePort(uint8_t port, uint8_t val) {
  writeReg(port == 0 ? SREG_OLATA : SREG_OLATB, val);
}

uint8_t Mcp23s17::readPort(uint8_t port) {
  return readReg(port == 0 ? SREG_GPIOA : SREG_GPIOB);
}

} // namespace gh
