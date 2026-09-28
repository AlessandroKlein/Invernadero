#pragma once
// Bus 1-Wire con múltiples DS18B20 (sección 8).
// Cada sensor tiene una dirección única de 64 bits; permite medir temperatura
// distribuida (agua, tanque, sustrato, exterior).

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

namespace gh {

class Ds18b20Sensor {
public:
  void begin(uint8_t pin);
  bool available() const { return found_ > 0; }
  uint8_t count() const { return found_; }
  void requestTemperatures();        // Dispara conversión en todos
  bool readTemperature(uint8_t index, float& temp); // Lee el sensor i
  float getTempC(uint8_t index);     // Acceso directo a la última lectura

private:
  OneWire oneWire_{2};
  DallasTemperature bus_{&oneWire_};
  uint8_t found_ = 0;
};

} // namespace gh
