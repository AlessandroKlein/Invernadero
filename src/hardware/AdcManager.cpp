#include "hardware/AdcManager.hpp"

namespace gh {

void AdcManager::begin(SpiManager* bus, uint8_t cs, AdcKind kind, uint8_t channels) {
  bus_ = bus;
  cs_ = cs;
  kind_ = kind;
  channels_ = channels;
  pinMode(cs_, OUTPUT);
  digitalWrite(cs_, HIGH);
}

uint16_t AdcManager::maxRaw() const {
  return (kind_ == AdcKind::MCP3008) ? 1023 : 4095;
}

uint16_t AdcManager::readRaw(uint8_t channel) {
  if (!available()) return 0;
  channel %= channels_;
  uint8_t out[3];
  uint8_t in[3];
  if (kind_ == AdcKind::MCP3008) {
    // MCP3008 (10 bits): start + SGL/DIFF + D2..D0, luego null.
    out[0] = 0x01;
    out[1] = 0x80 | ((channel & 0x07) << 4);
    out[2] = 0x00;
    bus_->select(cs_);
    bus_->beginTransaction();
    for (int i = 0; i < 3; i++) in[i] = bus_->bus().transfer(out[i]);
    bus_->endTransaction();
    bus_->deselect(cs_);
    return ((in[1] & 0x03) << 8) | in[2];
  } else {
    // MCP3208 (12 bits): start + SGL/DIFF + D2, luego D1 D0, luego null.
    out[0] = 0x06 | ((channel >> 2) & 0x01);
    out[1] = (channel & 0x03) << 6;
    out[2] = 0x00;
    bus_->select(cs_);
    bus_->beginTransaction();
    for (int i = 0; i < 3; i++) in[i] = bus_->bus().transfer(out[i]);
    bus_->endTransaction();
    bus_->deselect(cs_);
    return ((in[0] & 0x0F) << 8) | in[1];
  }
}

float AdcManager::readVoltage(uint8_t channel, float vref) {
  return (float)readRaw(channel) * vref / (float)maxRaw();
}

} // namespace gh
