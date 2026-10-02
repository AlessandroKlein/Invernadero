#pragma once
// Coordinador de hardware (decisiones §20 / README §285). Agrupa el BusManager
// y el catálogo de nodos de hardware detectados/configurados (expansores, ADC,
// SD, W5500...). Separa "qué hardware hay" (configuración) de "qué hace"
// (automatización), el principio rector de la plataforma configurable.

#include <Arduino.h>

#include "core/PlatformTypes.hpp"
#include "core/PinConfig.hpp"
#include "hardware/BusManager.hpp"

namespace gh {

class HardwareManager {
public:
  static constexpr uint8_t MAX_NODES = 24;

  // Inicializa los buses y registra los nodos estáticos del PCB de referencia.
  void begin(const PinConfig& pins);

  BusManager& buses() { return buses_; }
  const BusManager& buses() const { return buses_; }

  bool registerNode(const HardwareNode& n);
  bool setNodeEnabled(const char* id, bool enabled);
  bool getNode(const char* id, HardwareNode& out) const;
  bool hasNode(const char* id) const { return findNode(id) >= 0; }
  uint8_t nodeCount() const { return nodeCount_; }

  // Escaneo I²C (delega en BusManager, README §231).
  uint8_t scanI2c(uint8_t* found, uint8_t maxFound) {
    return buses_.scanI2c(found, maxFound);
  }
  // Autodetección guiada (SEMA §210): escanea I²C y mapea direcciones a tipos.
  String detectI2cJson();

  void snapshot(HardwareNode* out, size_t max, size_t& n) const;
  String toJson() const;

private:
  BusManager buses_;
  HardwareNode nodes_[MAX_NODES];
  uint8_t nodeCount_ = 0;
  int8_t findNode(const char* id) const;
};

} // namespace gh
