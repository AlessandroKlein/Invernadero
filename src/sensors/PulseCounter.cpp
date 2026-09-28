#include "sensors/PulseCounter.hpp"

namespace gh {

void PulseCounter::begin(uint8_t pin, float factor, bool pullup) {
  pin_ = pin;
  factor_ = factor;
  pulses_ = 0;
  lastPulseUs_ = 0;
  ratePerMin_ = 0.0f;
  pinMode(pin_, pullup ? INPUT_PULLUP : INPUT);
  // attachInterruptArg permite pasar la instancia a la ISR (multi-instancia).
  attachInterruptArg(digitalPinToInterrupt(pin_), isrArg, this, FALLING);
}

void IRAM_ATTR PulseCounter::isrArg(void* arg) {
  PulseCounter* pc = (PulseCounter*)arg;
  pc->pulses_++;
  uint32_t now = micros();
  if (pc->lastPulseUs_ != 0) {
    uint32_t dt = now - pc->lastPulseUs_;
    if (dt > 0) {
      // Frecuencia de pulsos -> unidades por minuto.
      pc->ratePerMin_ = (60000000.0f / dt) * pc->factor_;
    }
  }
  pc->lastPulseUs_ = now;
}

void PulseCounter::reset() {
  pulses_ = 0;
  lastPulseUs_ = 0;
  ratePerMin_ = 0.0f;
}

void PulseCounter::resetAccumulator() {
  pulses_ = 0;
}

} // namespace gh
