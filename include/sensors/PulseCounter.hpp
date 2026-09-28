#pragma once
// Contador de pulsos por interrupción (caudal, lluvia, viento).
// Sección 15: el caudalímetro entrega pulsos; se usa optoacoplador para adaptar
// niveles de 5/12/24 V a los 3,3 V del ESP32.

#include <Arduino.h>

namespace gh {

class PulseCounter {
public:
  // pin: GPIO de entrada. factor: unidades por pulso (ej. litros/pulso).
  void begin(uint8_t pin, float factor = 1.0f, bool pullup = false);
  // Pulsos acumulados desde el último reset().
  uint32_t pulses() const { return pulses_; }
  // Valor acumulado = pulsos * factor.
  float accumulated() const { return pulses_ * factor_; }
  // Tasa por minuto (calculada con el intervalo entre pulsos recientes).
  float ratePerMinute() const { return ratePerMin_; }
  void reset();                       // Reinicia acumulado y tasa
  void resetAccumulator();            // Reinicia solo el acumulado

private:
  uint8_t pin_ = 0xFF;
  float factor_ = 1.0f;
  volatile uint32_t pulses_ = 0;
  volatile uint32_t lastPulseUs_ = 0;
  volatile float ratePerMin_ = 0.0f;
  static void IRAM_ATTR isrArg(void* arg); // ISR con argumento (instancia)
};

} // namespace gh
