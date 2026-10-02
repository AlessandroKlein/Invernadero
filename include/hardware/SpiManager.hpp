#pragma once
// Administrador del bus SPI (V8.1, README §249). Un único SPIClass compartido con
// control de CS por dispositivo, para W5500/SD/MCP23S17/ADC sin conflictos de
// recursos. Los drivers usan select()/deselect() + beginTransaction()/transfer().

#include <Arduino.h>
#include <SPI.h>

namespace gh {

class SpiManager {
public:
  // Inicializa el bus SPI. Devuelve false si ya estaba inicializado o falla.
  bool begin(int sck, int miso, int mosi, uint32_t freq = 4000000);
  void end();

  SPIClass& bus() { return spi_; }
  bool isReady() const { return ready_; }

  void select(uint8_t cs) { digitalWrite(cs, LOW); }
  void deselect(uint8_t cs) { digitalWrite(cs, HIGH); }
  void beginTransaction() { spi_.beginTransaction(SPISettings(freq_, MSBFIRST, SPI_MODE0)); }
  void endTransaction() { spi_.endTransaction(); }

private:
  SPIClass spi_;
  bool ready_ = false;
  uint32_t freq_ = 4000000;
};

} // namespace gh
