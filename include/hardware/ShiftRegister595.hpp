#pragma once
// Expansión de salidas mediante registros de desplazamiento 74HC595/74HCT595
// en cascada (daisy-chain) usando solo 3 pines (DATA, CLOCK, LATCH).
// Implementa además Soft-PWM por temporizador de hardware (secciones 21.x).
//
// Nota de diseño: el 74HC595 NO alimenta cargas directamente; solo entrega
// las señales lógicas a los drivers de potencia (MOSFET/SSR/relé). Para PWM de
// alta frecuencia o alta resolución se recomienda usar LEDC (GPIO directo) o un
// controlador dedicado (TLC5947), como indica el README.

#include <Arduino.h>

namespace gh {

class ShiftRegister595 {
public:
  // Inicializa pines y reserva buffers. numChips define cuántos 595 hay en cascada.
  void begin(int dataPin, int clockPin, int latchPin, uint8_t numChips);

  // Establece el ciclo de trabajo de un canal (0..255).
  void setChannel(uint16_t ch, uint16_t duty);
  // Establece el ciclo de trabajo en porcentaje (0..100).
  void setChannelPercent(uint16_t ch, float pct);
  // Salida digital ON/OFF (duty = 255/0).
  void setDigital(uint16_t ch, bool on);

  // Habilita/deshabilita la generación de Soft-PWM y fija frecuencia/resolución.
  // resolutionBits soportado: 8 (recomendado), 10 o 12.
  void setPwmEnabled(bool enabled, uint32_t freqHz = 200, uint8_t resolutionBits = 8);

  // Fuerza la escritura inmediata del frame actual (estado seguro).
  void commit();
  // Apaga todos los canales y escribe el frame (estado seguro, sección 21.14).
  void allOff();

  uint16_t numChannels() const { return numChips_ * 8; }

private:
  int dataPin_ = -1, clockPin_ = -1, latchPin_ = -1;
  uint8_t numChips_ = 0;
  uint16_t* duty_ = nullptr;   // Ciclo de trabajo por canal (0..top-1)
  uint8_t* frame_ = nullptr;   // Bytes a desplazar (MSB first)
  uint16_t top_ = 256;         // 1 << resolutionBits
  hw_timer_t* timer_ = nullptr;
  volatile uint16_t counter_ = 0;
  bool pwmEnabled_ = false;

  static void IRAM_ATTR onTimer(); // ISR del temporizador
  void tick();                     // Lógica de un tick de PWM
  void shiftOutFrame();            // Desplaza frame_ a los 595
};

} // namespace gh
