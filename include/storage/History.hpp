#pragma once
// Historial local de eventos y alarmas en memoria (sección 73).
// Para históricos extensos se recomienda servidor central o microSD.
// Implementación: buffer circular en RAM.

#include <Arduino.h>
#include "core/Types.hpp"

namespace gh {

class History {
public:
  void begin(size_t capacity = 128);   // Reserva el buffer circular
  void add(uint8_t severity, const char* msg); // Registra un evento
  size_t count() const;                // Eventos almacenados
  LogEvent get(size_t i) const;        // Evento (orden cronológico)
  void clear();                        // Vacía el historial
  // Serializa los últimos `max` eventos a JSON.
  String toJson(size_t max = 64) const;

private:
  LogEvent* ring_ = nullptr;   // Buffer circular
  size_t capacity_ = 0;
  size_t head_ = 0;            // Próxima posición a escribir
  size_t size_ = 0;            // Nº de eventos válidos
};

} // namespace gh
