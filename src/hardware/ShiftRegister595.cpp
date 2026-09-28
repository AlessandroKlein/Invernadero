#include "hardware/ShiftRegister595.hpp"

namespace gh {

// Instancia única usada por la ISR (solo hay una cadena de 595 en el sistema).
static ShiftRegister595* s_instance = nullptr;

void ShiftRegister595::begin(int dataPin, int clockPin, int latchPin, uint8_t numChips) {
  dataPin_ = dataPin;
  clockPin_ = clockPin;
  latchPin_ = latchPin;
  numChips_ = numChips;

  // Configurar pines de control como salidas.
  pinMode(dataPin_, OUTPUT);
  pinMode(clockPin_, OUTPUT);
  pinMode(latchPin_, OUTPUT);

  // Reservar buffers: un ciclo de trabajo por canal y un byte por chip.
  duty_ = new uint16_t[numChannels()];
  frame_ = new uint8_t[numChips_];
  for (uint16_t i = 0; i < numChannels(); i++) duty_[i] = 0;

  s_instance = this;      // Registrar para la ISR
  allOff();               // Estado seguro inicial
}

void ShiftRegister595::setChannel(uint16_t ch, uint16_t duty) {
  if (ch >= numChannels()) return;
  if (duty > top_ - 1) duty = top_ - 1;
  duty_[ch] = duty;
  if (!pwmEnabled_) commit(); // En modo digital, aplicar inmediatamente
}

void ShiftRegister595::setChannelPercent(uint16_t ch, float pct) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  setChannel(ch, (uint16_t)(pct / 100.0f * (top_ - 1)));
}

void ShiftRegister595::setDigital(uint16_t ch, bool on) {
  setChannel(ch, on ? (top_ - 1) : 0);
}

void ShiftRegister595::setPwmEnabled(bool enabled, uint32_t freqHz, uint8_t resolutionBits) {
  // Detener el temporizador anterior si existía.
  if (timer_) { timerAlarmDisable(timer_); timerDetachInterrupt(timer_); timerEnd(timer_); timer_ = nullptr; }

  // Limitar resolución al rango 1..12 bits.
  if (resolutionBits < 1) resolutionBits = 1;
  if (resolutionBits > 12) resolutionBits = 12;
  top_ = 1 << resolutionBits;

  // Ajustar los ciclos de trabajo actuales al nuevo rango (reescalado simple a 8 bits).
  // (En la práctica se reescriben desde ActuatorManager al cambiar la configuración.)

  pwmEnabled_ = enabled;
  if (!enabled) { commit(); return; }

  // Crear temporizador hardware: tick = freq * top.
  timer_ = timerBegin(0, 80, true); // 80 MHz / 80 = 1 MHz base
  uint64_t alarmCount = 1000000ULL / ((uint64_t)freqHz * top_);
  if (alarmCount < 1) alarmCount = 1;
  timerAttachInterrupt(timer_, &ShiftRegister595::onTimer, true);
  timerAlarmWrite(timer_, (uint64_t)alarmCount, true); // auto-reload
  timerAlarmEnable(timer_);
}

void ShiftRegister595::commit() {
  // Construir el frame a partir de los ciclos de trabajo y desplazarlo.
  shiftOutFrame();
}

void ShiftRegister595::allOff() {
  for (uint16_t i = 0; i < numChannels(); i++) duty_[i] = 0;
  shiftOutFrame();
}

void ShiftRegister595::tick() {
  // Incrementar contador de fase y calcular el bit de cada canal.
  counter_++;
  if (counter_ >= top_) counter_ = 0;
  shiftOutFrame();
}

void ShiftRegister595::shiftOutFrame() {
  // Construir el byte de cada chip. El canal 0 es el bit más significativo
  // del último chip desplazado, así que se empaqueta en orden inverso.
  for (uint8_t b = 0; b < numChips_; b++) frame_[b] = 0;

  uint16_t total = numChannels();
  for (uint16_t i = 0; i < total; i++) {
    bool on = pwmEnabled_ ? (duty_[i] > counter_) : (duty_[i] > 0);
    if (!on) continue;
    uint8_t chip = i / 8;
    uint8_t bit = i % 8;
    frame_[chip] |= (1 << bit);
  }

  // Desplazar MSB-first con LATCH bajo.
  digitalWrite(latchPin_, LOW);
  // Se desplaza del último chip al primero (el primer byte enviado llega al chip final).
  for (int b = numChips_ - 1; b >= 0; b--) {
    shiftOut(dataPin_, clockPin_, MSBFIRST, frame_[b]);
  }
  // Pulso de LATCH para presentar las salidas.
  digitalWrite(latchPin_, HIGH);
  digitalWrite(latchPin_, LOW);
}

void IRAM_ATTR ShiftRegister595::onTimer() {
  if (s_instance) s_instance->tick();
}

} // namespace gh
