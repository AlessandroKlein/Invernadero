#include "sensors/UltrasonicLevel.hpp"

namespace gh {

void UltrasonicLevel::begin(uint8_t trigPin, uint8_t echoPin, float tankDepthCm) {
  trig_ = trigPin;
  echo_ = echoPin;
  tankDepthCm_ = tankDepthCm;
  pinMode(trig_, OUTPUT);
  pinMode(echo_, INPUT);
}

float UltrasonicLevel::readDistanceCm() {
  // Disparar pulso de 10 µs.
  digitalWrite(trig_, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_, LOW);

  // Medir el ancho del eco (velocidad del sonido ~0.0343 cm/µs).
  long duration = pulseIn(echo_, HIGH, 30000); // timeout 30 ms
  if (duration == 0) return NAN;
  return duration * 0.0343f / 2.0f;
}

float UltrasonicLevel::readLevelPercent() {
  float d = readDistanceCm();
  if (isnan(d) || tankDepthCm_ <= 0) return NAN;
  // El nivel de agua es la profundidad total menos la distancia al agua.
  float level = 100.0f * (1.0f - (d / tankDepthCm_));
  if (level < 0) level = 0;
  if (level > 100) level = 100;
  return level;
}

} // namespace gh
