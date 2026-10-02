#pragma once
// Administrador de buses (README §280). Centraliza el registro, la propiedad y
// el estado de los buses físicos para evitar que cada driver inicialice por su
// cuenta Wire/SPI/UART y para detectar conflictos de recursos.

#include <Arduino.h>
#include <Wire.h>

#include "core/PlatformTypes.hpp"

namespace gh {

class BusManager {
public:
  static constexpr uint8_t MAX_BUSES = 12;

  // Inicializa el bus I²C por defecto y registra los buses conocidos del PCB.
  void begin();

  // Registra/configura un bus. Devuelve false si el tipo es inválido o no hay slots.
  bool registerBus(BusType type, uint8_t index, bool enabled = true);
  // Reclama un bus para un driver (único propietario). False si ya está ocupado.
  bool claim(BusType type, uint8_t index, const char* owner);
  // Libera un bus. Solo lo libera el propietario registrado.
  bool release(BusType type, uint8_t index, const char* owner);

  bool isRegistered(BusType type, uint8_t index = 0) const;
  bool isBusy(BusType type, uint8_t index = 0) const;
  const char* owner(BusType type, uint8_t index = 0) const;
  BusState state(BusType type, uint8_t index = 0) const;

  // Escaneo I²C (README §231): direcciones que responden con ACK.
  uint8_t scanI2c(uint8_t* found, uint8_t maxFound, uint8_t index = 0);

  uint8_t count() const { return count_; }
  String toJson() const;

private:
  struct Slot {
    BusType type = BusType::NONE;
    uint8_t index = 0;
    bool enabled = false;
    BusState state = BusState::UNREGISTERED;
    char owner[24] = "";
  };

  Slot slots_[MAX_BUSES];
  uint8_t count_ = 0;
  int8_t find(BusType type, uint8_t index) const;
};

} // namespace gh
