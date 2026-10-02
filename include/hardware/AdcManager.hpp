#pragma once
// Administrador de ADC externos por SPI (V8.1, README §250-252): MCP3008 (10 bits)
// y MCP3208 (12 bits). El ADS1115 (I²C) mantiene su driver dedicado. Cada canal se
// lee como raw y como tensión referida a vref.

#include <Arduino.h>

#include "core/PlatformTypes.hpp"
#include "hardware/SpiManager.hpp"

namespace gh {

class AdcManager {
public:
  // bus: SpiManager ya inicializado; cs: pin CS del ADC.
  void begin(SpiManager* bus, uint8_t cs, AdcKind kind, uint8_t channels = 8);

  bool available() const { return bus_ != nullptr && kind_ != AdcKind::NONE; }
  AdcKind kind() const { return kind_; }
  uint8_t channels() const { return channels_; }

  uint16_t readRaw(uint8_t channel);
  float readVoltage(uint8_t channel, float vref = 3.3f);
  uint16_t maxRaw() const;  // 1023 (10 bits) o 4095 (12 bits)

private:
  SpiManager* bus_ = nullptr;
  uint8_t cs_ = 0;
  AdcKind kind_ = AdcKind::NONE;
  uint8_t channels_ = 8;
};

} // namespace gh
