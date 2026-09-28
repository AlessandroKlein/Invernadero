#include "sensors/Ads1115Driver.hpp"

namespace gh {

bool Ads1115Driver::begin(uint8_t addr, TwoWire* wire) {
  addr_ = addr;
  wire_ = wire ? wire : &Wire;
  // El constructor por defecto no recibe parámetros; la dirección y el bus se
  // pasan a begin().
  ads_ = Adafruit_ADS1115();
  available_ = ads_.begin(addr_, wire_);
  if (available_) ads_.setGain(GAIN_ONE); // ±4.096 V por defecto
  return available_;
}

void Ads1115Driver::setGain(adsGain_t gain) {
  if (available_) ads_.setGain(gain);
}

int16_t Ads1115Driver::readRaw(uint8_t channel) {
  if (!available_) return 0;
  return ads_.readADC_SingleEnded(channel);
}

float Ads1115Driver::readVoltage(uint8_t channel) {
  if (!available_) return 0.0f;
  // computeVolts usa la ganancia configurada internamente.
  return ads_.computeVolts(readRaw(channel));
}

} // namespace gh
