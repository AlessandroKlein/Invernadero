#pragma once
// Maestro Modbus RTU sobre RS485 (sección 18.2).
// Permite leer sensores industriales (pH, EC, temperatura, etc.) con un único
// bus RS485. Controla el pin DE/RE del transceiver (SP3485/MAX485/ADM2483).

#include <Arduino.h>
#include <HardwareSerial.h>

namespace gh {

class ModbusRtu {
public:
  // Inicializa el puerto serie y el pin de dirección DE/RE.
  void begin(HardwareSerial* serial, int dePin, uint32_t baud = 9600);

  // Lee N registros de retención (función 0x03).
  bool readHoldingRegisters(uint8_t slaveId, uint16_t addr, uint16_t count, uint16_t* out);
  // Lee N registros de entrada (función 0x04).
  bool readInputRegisters(uint8_t slaveId, uint16_t addr, uint16_t count, uint16_t* out);

private:
  HardwareSerial* serial_ = nullptr;
  int dePin_ = -1;
  static uint16_t crc16(const uint8_t* data, size_t len);
  bool transceive(uint8_t* req, size_t reqLen, uint8_t* resp, size_t respLen, uint32_t timeoutMs = 200);
};

} // namespace gh
