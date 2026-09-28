#include "sensors/LightSensor.hpp"

namespace gh {

bool LightSensor::begin(uint8_t addr, TwoWire* wire) {
  addr_ = addr;
  wire_ = wire ? wire : &Wire;
  // Encender el sensor y configurar modo continuo de alta resolución (1 lux).
  wire_->beginTransmission(addr_);
  wire_->write(0x01); // power on
  wire_->endTransmission();
  wire_->beginTransmission(addr_);
  wire_->write(0x10); // continuous high resolution mode
  available_ = (wire_->endTransmission() == 0);
  return available_;
}

float LightSensor::readLux() {
  if (!available_) return NAN;
  // Leer 2 bytes de medición (modo continuo).
  wire_->requestFrom(addr_, (uint8_t)2);
  if (wire_->available() < 2) return NAN;
  uint16_t raw = ((uint16_t)wire_->read() << 8) | wire_->read();
  // lux = valor crudo / 1.2 (factor de resolución del BH1750).
  return raw / 1.2f;
}

} // namespace gh
