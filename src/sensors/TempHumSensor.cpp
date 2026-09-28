#include "sensors/TempHumSensor.hpp"

namespace gh {

bool TempHumSensor::begin(Kind kind, uint8_t addr, TwoWire* wire) {
  kind_ = kind;
  if (kind_ == Kind::SHT31) {
    // La librería SHT31 fija el bus en el constructor (por defecto Wire global).
    (void)wire; // se usa el bus global configurado en SensorManager
    available_ = sht_.begin(addr);
  } else {
    available_ = aht_.begin(wire);
  }
  return available_;
}

bool TempHumSensor::read(float& temp, float& hum) {
  if (!available_) return false;
  if (kind_ == Kind::SHT31) {
    // SHT31 lee ambas magnitudes; se comprueba validez con isnan.
    temp = sht_.readTemperature();
    hum = sht_.readHumidity();
    return !isnan(temp) && !isnan(hum);
  } else {
    sensors_event_t h, t;
    if (!aht_.getEvent(&h, &t)) return false;
    temp = t.temperature;
    hum = h.relative_humidity;
    return true;
  }
}

} // namespace gh
