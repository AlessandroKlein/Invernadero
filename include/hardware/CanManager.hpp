#pragma once
// Abstracción del bus CAN/TWAI (V9; SEMA §25). El ESP32 dispone del periférico
// TWAI (CAN 2.0) pero requiere un transceptor externo (p. ej. SN65HVD230/231/232)
// conectado a los pines TX/RX. La capa de aplicación se mantiene separada de la
// capa física: este driver expone solo enviar/recibir tramas y métricas.
//
// NOTA: TWAI solo existe en ESP32 / ESP32-S2 / ESP32-S3.

#include <Arduino.h>

namespace gh {

class CanManager {
public:
  // Inicializa TWAI en modo normal. False si ya está instalado o falla.
  bool begin(int txPin, int rxPin, uint32_t bitrate = 500000);
  void end();

  // Envía una trama (id de 11 o 29 bits, hasta 8 bytes de datos).
  bool send(uint32_t id, const uint8_t* data, uint8_t len, bool extended = false);
  // Recibe una trama (timeoutMs=0 -> no bloqueante).
  bool receive(uint32_t& id, uint8_t* data, uint8_t& len, uint32_t timeoutMs = 0);

  bool installed() const { return installed_; }
  uint32_t bitrate() const { return bitrate_; }
  uint32_t txCount() const { return txCount_; }
  uint32_t rxCount() const { return rxCount_; }
  uint32_t errorCount() const { return errorCount_; }

  // Capa de aplicación (V9): nodos identificados por CAN ID + estado de actividad.
  // La capa física (TWAI) queda separada de la lógica de nodos remotos.
  static constexpr uint8_t MAX_NODES = 8;
  bool registerNode(uint32_t id, const char* name);
  void touchNode(uint32_t id);          // marca actividad de un nodo
  bool nodeAlive(uint32_t id) const;    // ¿respondió en los últimos 5 s?
  uint8_t nodeCount() const { return nodeCount_; }

  String toJson() const;

private:
  bool installed_ = false;
  uint32_t bitrate_ = 0;
  int txPin_ = -1;
  int rxPin_ = -1;
  volatile uint32_t txCount_ = 0;
  volatile uint32_t rxCount_ = 0;
  volatile uint32_t errorCount_ = 0;

  struct Node {
    char name[24] = "";
    uint32_t id = 0;
    volatile uint32_t lastSeenMs = 0;
  };
  Node nodes_[MAX_NODES];
  uint8_t nodeCount_ = 0;
  int8_t findNode(uint32_t id) const;
};

} // namespace gh
