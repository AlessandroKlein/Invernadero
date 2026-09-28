#include "sensors/Co2Sensor.hpp"

namespace gh {

bool Co2Sensor::begin(uint8_t addr, TwoWire* wire) {
  addr_ = addr;
  wire_ = wire ? wire : &Wire;
  // El SCD4x no ofrece una lectura de ID simple universal; se marca disponible
  // y se valida al leer mediciones.
  available_ = true;
  return available_;
}

void Co2Sensor::writeCommand(uint16_t cmd) {
  wire_->beginTransmission(addr_);
  wire_->write(cmd >> 8);
  wire_->write(cmd & 0xFF);
  wire_->endTransmission();
}

uint8_t Co2Sensor::crc8(const uint8_t* data, size_t len) {
  uint8_t crc = 0xFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
    }
  }
  return crc;
}

void Co2Sensor::startPeriodicMeasurement() {
  writeCommand(0x21B1);
}

bool Co2Sensor::readMeasurement(uint16_t& co2, float& temp, float& hum) {
  if (!available_) return false;
  writeCommand(0xEC05);
  uint8_t buf[9];
  wire_->requestFrom(addr_, (uint8_t)9);
  if (wire_->available() < 9) return false;
  for (int i = 0; i < 9; i++) buf[i] = wire_->read();

  // Verificar CRC8 de cada pareja de bytes.
  if (crc8(buf, 2) != buf[2]) return false;
  if (crc8(buf + 3, 2) != buf[5]) return false;
  if (crc8(buf + 6, 2) != buf[8]) return false;

  uint16_t rawCo2 = (buf[0] << 8) | buf[1];
  uint16_t rawT = (buf[3] << 8) | buf[4];
  uint16_t rawH = (buf[6] << 8) | buf[7];

  co2 = rawCo2;
  temp = -45.0f + 175.0f * rawT / 65535.0f;
  hum = 100.0f * rawH / 65535.0f;
  return true;
}

} // namespace gh
