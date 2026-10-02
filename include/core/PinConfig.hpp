#pragma once
// Mapa de pines en tiempo de ejecución (modularidad completa, README §205/§263).
// Sustituye a PinMap.hpp: los pines y direcciones I²C se guardan en NVS y son
// editables desde la web protegida, sin recompilar el firmware.

#include <Arduino.h>
#include <Preferences.h>

namespace gh {

struct PinConfig {
  // Bus I²C
  int i2cSda = 21;
  int i2cScl = 22;
  uint32_t i2cFreq = 100000;
  // 74HC595 (SPI bit-banged)
  int hc595Mosi = 23;
  int hc595Sclk = 18;
  int hc595Latch = 5;
  int hc595Count = 4;
  // 74HC165 (entradas, SPI bit-banged; por defecto deshabilitado)
  int hc165Data = 12;
  int hc165Clock = 13;
  int hc165Latch = 14;
  int hc165Count = 1;
  // 1-Wire
  int oneWire = 4;
  // Entradas de pulsos
  int flowPin = 34;
  int rainPin = 35;
  int windPin = 36;
  // Tanque
  int tankTrig = 25;
  int tankEcho = 26;
  int floatLow = 32;
  int floatHigh = 33;
  // Seguridad
  int emergencyStop = 27;
  // RS485 / Modbus
  int rs485Rx = 16;
  int rs485Tx = 17;
  int rs485De = 14;
  // SPI nativo
  int spiSck = 18;
  int spiMiso = 19;
  int spiMosi = 23;
  // Direcciones I²C
  uint8_t i2cAddrSht31 = 0x44;
  uint8_t i2cAddrAht20 = 0x38;
  uint8_t i2cAddrAds1115 = 0x48;
  uint8_t i2cAddrBh1750 = 0x23;
  uint8_t i2cAddrScd41 = 0x62;
  uint8_t i2cAddrMcp23017 = 0x20;
};

// Valores por defecto (equivalentes al PinMap de referencia).
PinConfig defaultPinConfig();

// Serialización JSON (para la web y la persistencia NVS).
String pinConfigToJson(const PinConfig& p);
bool pinConfigFromJson(const String& json, PinConfig& out);

// Persistencia en NVS (namespace separado, local-only — no se exporta con la
// configuración funcional).
class PinConfigManager {
public:
  void begin();                      // carga (o crea) el mapa de pines
  PinConfig get() const;
  void set(const PinConfig& p);      // guarda en NVS
  void reset();                      // vuelve a defaults

private:
  mutable Preferences prefs_;
  PinConfig cfg_;
  static constexpr const char* NVS_NS = "ghpins";
  static constexpr const char* NVS_KEY = "pins";
};

} // namespace gh
