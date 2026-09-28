#include "hardware/ModbusRtu.hpp"

namespace gh {

void ModbusRtu::begin(HardwareSerial* serial, int dePin, uint32_t baud) {
  serial_ = serial;
  dePin_ = dePin;
  if (dePin_ >= 0) {
    pinMode(dePin_, OUTPUT);
    digitalWrite(dePin_, LOW); // recepción por defecto
  }
}

uint16_t ModbusRtu::crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) crc = (crc >> 1) ^ 0xA001;
      else crc >>= 1;
    }
  }
  return crc;
}

bool ModbusRtu::transceive(uint8_t* req, size_t reqLen, uint8_t* resp, size_t respLen, uint32_t timeoutMs) {
  // Calcular CRC y añadirlo a la trama de petición.
  uint16_t crc = crc16(req, reqLen);
  req[reqLen] = crc & 0xFF;
  req[reqLen + 1] = crc >> 8;

  // Limpiar buffer de recepción.
  while (serial_->available()) serial_->read();

  // Habilitar transmisión y enviar.
  if (dePin_ >= 0) digitalWrite(dePin_, HIGH);
  serial_->write(req, reqLen + 2);
  serial_->flush(); // esperar a que se envíe todo
  if (dePin_ >= 0) digitalWrite(dePin_, LOW);

  // Esperar respuesta.
  uint32_t start = millis();
  size_t got = 0;
  while (got < respLen && (millis() - start) < timeoutMs) {
    while (serial_->available() && got < respLen) {
      resp[got++] = serial_->read();
      start = millis(); // reiniciar timeout por byte recibido
    }
    delay(1);
  }
  if (got < respLen) return false;

  // Verificar CRC de la respuesta.
  uint16_t respCrc = resp[respLen - 1] << 8 | resp[respLen - 2];
  return crc16(resp, respLen - 2) == respCrc;
}

bool ModbusRtu::readHoldingRegisters(uint8_t slaveId, uint16_t addr, uint16_t count, uint16_t* out) {
  if (count == 0 || count > 125) return false;
  uint8_t req[8];
  req[0] = slaveId;
  req[1] = 0x03;                 // Leer registros de retención
  req[2] = addr >> 8;
  req[3] = addr & 0xFF;
  req[4] = count >> 8;
  req[5] = count & 0xFF;
  uint8_t resp[256];
  size_t respLen = 5 + count * 2;
  if (!transceive(req, 6, resp, respLen)) return false;
  if (resp[1] & 0x80) return false; // excepción Modbus
  for (uint16_t i = 0; i < count; i++) {
    out[i] = resp[3 + i * 2] << 8 | resp[4 + i * 2];
  }
  return true;
}

bool ModbusRtu::readInputRegisters(uint8_t slaveId, uint16_t addr, uint16_t count, uint16_t* out) {
  if (count == 0 || count > 125) return false;
  uint8_t req[8];
  req[0] = slaveId;
  req[1] = 0x04;                 // Leer registros de entrada
  req[2] = addr >> 8;
  req[3] = addr & 0xFF;
  req[4] = count >> 8;
  req[5] = count & 0xFF;
  uint8_t resp[256];
  size_t respLen = 5 + count * 2;
  if (!transceive(req, 6, resp, respLen)) return false;
  if (resp[1] & 0x80) return false;
  for (uint16_t i = 0; i < count; i++) {
    out[i] = resp[3 + i * 2] << 8 | resp[4 + i * 2];
  }
  return true;
}

} // namespace gh
