#pragma once
// Tipos de la plataforma configurable (V8). Complementan core/Types.hpp con las
// estructuras del sistema de módulos, capacidades, buses y registros dinámicos.
// Se mantienen aquí (y no en Types.hpp) para no acoplar el firmware de campo con
// la plataforma configurable: un target mínimo puede excluir estos módulos.

#include <Arduino.h>

#include "core/Types.hpp"

namespace gh {

// --- Buses (README §280) -----------------------------------------------------

enum class BusType : uint8_t {
  NONE = 0,
  I2C = 1,
  SPI = 2,
  UART = 3,
  RS485 = 4,
  ONEWIRE = 5,
  GPIO = 6,
  CAN = 7  // TWAI: abstracción preparada, implementación en V9
};

enum class BusState : uint8_t {
  UNREGISTERED = 0,  // Slot libre / no configurado
  READY = 1,         // Bus inicializado y disponible
  BUSY = 2,          // Reclamado por un driver
  ERROR = 3          // Inicialización fallida
};

inline const char* busTypeString(BusType t) {
  switch (t) {
    case BusType::I2C:     return "I2C";
    case BusType::SPI:     return "SPI";
    case BusType::UART:    return "UART";
    case BusType::RS485:   return "RS485";
    case BusType::ONEWIRE: return "ONEWIRE";
    case BusType::GPIO:    return "GPIO";
    case BusType::CAN:     return "CAN";
    default:               return "NONE";
  }
}

inline const char* busStateString(BusState s) {
  switch (s) {
    case BusState::READY:       return "READY";
    case BusState::BUSY:        return "BUSY";
    case BusState::ERROR:       return "ERROR";
    default:                    return "UNREGISTERED";
  }
}

// --- Sistema de módulos (decisiones §21) -------------------------------------

// NOTA: los enumeradores van prefijados con MODULE_ porque ENABLED/DISABLED
// colisionan con macros del núcleo Arduino de ESP32 (esp32-hal-gpio.h).
enum class ModuleState : uint8_t {
  NOT_INSTALLED = 0,
  INSTALLED = 1,
  MODULE_ENABLED = 2,
  MODULE_DISABLED = 3,
  MODULE_ERROR = 4
};

inline const char* moduleStateString(ModuleState s) {
  switch (s) {
    case ModuleState::INSTALLED:      return "INSTALLED";
    case ModuleState::MODULE_ENABLED: return "ENABLED";
    case ModuleState::MODULE_DISABLED:return "DISABLED";
    case ModuleState::MODULE_ERROR:   return "ERROR";
    default:                          return "NOT_INSTALLED";
  }
}

// Descriptor de un módulo del firmware: agrupa drivers/capacidades bajo un id
// versionado con dependencias explícitas (decisiones §21).
struct ModuleDescriptor {
  char id[24] = "";
  char version[12] = "";
  char dependencies[4][24] = {};  // ids de los módulos de los que depende
  uint8_t dependencyCount = 0;
  char capabilities[6][16] = {};  // capacidades que provee
  uint8_t capabilityCount = 0;
  ModuleState state = ModuleState::NOT_INSTALLED;
};

// --- Hardware -----------------------------------------------------------------

// Tipo de nodo gestionado por HardwareManager.
enum class HardwareKind : uint8_t {
  NONE = 0,
  HC595 = 1,     // 74HC595 (salidas, SPI bit-banged)
  HC165 = 2,     // 74HC165 (entradas, SPI bit-banged)
  MCP23017 = 3,  // Expansor I²C
  MCP23S17 = 4,  // Expansor SPI
  ADC = 5,       // ADC externo (ADS1115/MCP3008/MCP3208/...)
  SD = 6,        // Tarjeta SD por SPI
  W5500 = 7,     // Ethernet por SPI
  SENSOR = 8,    // Sensor directo en un bus
  ACTUATOR = 9   // Actuador directo en GPIO
};

inline const char* hardwareKindString(HardwareKind k) {
  switch (k) {
    case HardwareKind::HC595:     return "HC595";
    case HardwareKind::HC165:     return "HC165";
    case HardwareKind::MCP23017:  return "MCP23017";
    case HardwareKind::MCP23S17:  return "MCP23S17";
    case HardwareKind::ADC:       return "ADC";
    case HardwareKind::SD:        return "SD";
    case HardwareKind::W5500:     return "W5500";
    case HardwareKind::SENSOR:    return "SENSOR";
    case HardwareKind::ACTUATOR:  return "ACTUATOR";
    default:                      return "NONE";
  }
}

// Nodo de hardware registrado: instancia física detectable/configurable.
struct HardwareNode {
  char id[24] = "";              // id estable del nodo ("hc595-0", "mcp23017-1", ...)
  HardwareKind kind = HardwareKind::NONE;
  BusType bus = BusType::NONE;
  uint8_t busIndex = 0;          // nº de bus (0 = primero de su tipo)
  uint8_t address = 0;           // dirección I²C o pin CS
  bool enabled = false;
  char owner[24] = "";           // módulo/driver que lo reclama
};

// --- Registros de sensores/actuadores (catálogo configurable, README §274-276) --

// Entrada del catálogo de sensores: instancia lógica definida por configuración,
// independiente del driver compilado.
struct SensorEntry {
  char id[24] = "";              // id lógico ("temp_interior")
  char name[32] = "";            // nombre visible
  char driver[24] = "";          // driver físico ("sht31", "ds18b20", ...)
  SensorType type = SensorType::NONE;
  BusType bus = BusType::NONE;
  uint8_t busIndex = 0;
  uint8_t address = 0;
  uint8_t zone = 0;
  bool enabled = false;
  uint32_t readIntervalMs = 5000;
};

// Entrada del catálogo de actuadores.
struct ActuatorEntry {
  char id[24] = "";              // id lógico ("bomba_riego")
  char name[32] = "";            // nombre visible
  ActuatorRole role = ActuatorRole::GENERIC;
  OutputKind kind = OutputKind::DIGITAL;
  BusType bus = BusType::NONE;
  uint8_t busIndex = 0;
  uint8_t channel = 0;           // canal lógico en el hardware
  uint8_t zone = 0;
  bool enabled = false;
  bool safeState = false;
};

// --- Almacenamiento (reservado para V8.4) -------------------------------------

enum class StorageBackend : uint8_t {
  NONE = 0,
  NVS = 1,
  SPIFFS = 2,
  LITTLEFS = 3,
  SD = 4
};

inline const char* storageBackendString(StorageBackend b) {
  switch (b) {
    case StorageBackend::NVS:      return "NVS";
    case StorageBackend::SPIFFS:   return "SPIFFS";
    case StorageBackend::LITTLEFS: return "LITTLEFS";
    case StorageBackend::SD:       return "SD";
    default:                       return "NONE";
  }
}

} // namespace gh
