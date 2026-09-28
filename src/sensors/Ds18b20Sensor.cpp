#include "sensors/Ds18b20Sensor.hpp"

namespace gh {

void Ds18b20Sensor::begin(uint8_t pin) {
  // Inicializar el bus 1-Wire con su pull-up externo de 4.7 kΩ a 3.3 V.
  oneWire_ = OneWire(pin);
  bus_ = DallasTemperature(&oneWire_);
  bus_.begin();
  found_ = bus_.getDeviceCount();
  bus_.setResolution(12);           // Máxima resolución (±0.0625 °C)
}

void Ds18b20Sensor::requestTemperatures() {
  if (found_ == 0) return;
  bus_.requestTemperatures();
}

bool Ds18b20Sensor::readTemperature(uint8_t index, float& temp) {
  if (index >= found_) return false;
  temp = bus_.getTempCByIndex(index);
  return temp != DEVICE_DISCONNECTED_C; // -127 indica sensor desconectado
}

float Ds18b20Sensor::getTempC(uint8_t index) {
  if (index >= found_) return DEVICE_DISCONNECTED_C;
  return bus_.getTempCByIndex(index);
}

} // namespace gh
