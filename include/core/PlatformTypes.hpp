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

// --- V8.1: ADC externos -------------------------------------------------------

enum class AdcKind : uint8_t {
  NONE = 0,
  ADS1115 = 1,   // I²C 16 bits
  MCP3008 = 2,   // SPI 10 bits, 8 canales
  MCP3208 = 3,   // SPI 12 bits, 8 canales
  ADS8688 = 4    // SPI 16 bits, 8 canales (industrial)
};

inline const char* adcKindString(AdcKind k) {
  switch (k) {
    case AdcKind::ADS1115: return "ADS1115";
    case AdcKind::MCP3008: return "MCP3008";
    case AdcKind::MCP3208: return "MCP3208";
    case AdcKind::ADS8688: return "ADS8688";
    default:               return "NONE";
  }
}

// --- V9: Modbus / provisioning (decisiones §11/§17) ---------------------------

enum class ProvisioningState : uint8_t {
  DISCOVERED = 0,
  PENDING = 1,
  COMMISSIONED = 2,
  ACTIVE = 3,
  BLOCKED = 4,
  REVOKED = 5
};

inline const char* provisioningStateString(ProvisioningState s) {
  switch (s) {
    case ProvisioningState::PENDING:      return "PENDING";
    case ProvisioningState::COMMISSIONED: return "COMMISSIONED";
    case ProvisioningState::ACTIVE:       return "ACTIVE";
    case ProvisioningState::BLOCKED:      return "BLOCKED";
    case ProvisioningState::REVOKED:      return "REVOKED";
    default:                              return "DISCOVERED";
  }
}

enum class ModbusDataType : uint8_t {
  UINT16 = 0, INT16 = 1, UINT32 = 2, INT32 = 3, FLOAT32 = 4
};

inline const char* modbusDataTypeString(ModbusDataType t) {
  switch (t) {
    case ModbusDataType::INT16:   return "INT16";
    case ModbusDataType::UINT32:  return "UINT32";
    case ModbusDataType::INT32:   return "INT32";
    case ModbusDataType::FLOAT32: return "FLOAT32";
    default:                      return "UINT16";
  }
}

// Perfil declarativo de un sensor Modbus (cómo se lee y se convierte).
struct ModbusProfile {
  char id[24] = "";
  char vendor[24] = "";
  char product[24] = "";
  uint16_t vendorId = 0;
  uint16_t productId = 0;
  uint8_t slaveId = 1;
  uint16_t registerAddress = 0;
  ModbusDataType dataType = ModbusDataType::UINT16;
  float scale = 1.0f;
  float offset = 0.0f;
  char unit[8] = "";
  SensorType magnitude = SensorType::NONE;
};

// Instancia de un sensor Modbus en una instalación (referencia a un perfil).
struct SensorInstance {
  char id[24] = "";
  char name[32] = "";
  char profileId[24] = "";
  uint8_t slaveId = 1;
  uint8_t zone = 0;
  bool enabled = false;
  ProvisioningState state = ProvisioningState::DISCOVERED;
};

// --- Configuración por capas (README §205 / decisiones) ----------------------
// El ConfigManager plano evoluciona hacia capas superpuestas; la capa inferior
// fija valores y la superior los sobrescribe.

enum class ConfigLayer : uint8_t {
  FACTORY = 0,     // valores de fábrica (base, no editables)
  HARDWARE = 1,    // buses, pines, direcciones (solo local)
  DRIVERS = 2,     // configuración de drivers (SHT31, Modbus, ...)
  INSTALLATION = 3,// qué hardware hay en esta instalación
  AUTOMATION = 4,  // reglas, umbrales, horarios, zonas
  USER = 5         // preferencias del operador
};

inline const char* configLayerString(ConfigLayer l) {
  switch (l) {
    case ConfigLayer::HARDWARE:     return "HARDWARE";
    case ConfigLayer::DRIVERS:      return "DRIVERS";
    case ConfigLayer::INSTALLATION: return "INSTALLATION";
    case ConfigLayer::AUTOMATION:   return "AUTOMATION";
    case ConfigLayer::USER:         return "USER";
    default:                        return "FACTORY";
  }
}

} // namespace gh
