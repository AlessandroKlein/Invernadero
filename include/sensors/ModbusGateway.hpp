#pragma once
// Gateway RS485/Modbus (V9): polling multi-esclavo según los perfiles e
// instancias del ModbusProfileRegistry. Convierte registros (tipo, escala,
// offset) y expone estado por esclavo (OK / TIMEOUT / CRC / DISCONNECTED).

#include <Arduino.h>

#include "core/PlatformTypes.hpp"
#include "hardware/ModbusRtu.hpp"
#include "sensors/ModbusProfileRegistry.hpp"

namespace gh {

class ModbusGateway {
public:
  static constexpr uint8_t MAX_SLAVES = 24;

  enum SlaveState : uint8_t { UNKNOWN = 0, OK = 1, TIMEOUT = 2, CRC_ERROR = 3, DISCONNECTED = 4 };

  struct Slave {
    char instanceId[24] = "";
    uint8_t slaveId = 1;
    uint16_t registerAddress = 0;
    ModbusDataType dataType = ModbusDataType::UINT16;
    float scale = 1.0f;
    float offset = 0.0f;
    SensorType magnitude = SensorType::NONE;
    uint32_t pollIntervalMs = 5000;
    uint32_t lastPollMs = 0;
    float value = 0.0f;
    bool valid = false;
    SlaveState state = UNKNOWN;
    uint32_t failCount = 0;
  };

  void begin(ModbusRtu* rtu, ModbusProfileRegistry* registry);

  // Reconstruye la tabla de esclavos desde las instancias del registro.
  void rebuild();

  // Polling: consulta los esclavos vencidos (llamar periódicamente).
  void tick();

  // Convierte registros de 16 bits según el tipo y aplica escala/offset.
  static float convert(const uint16_t* regs, ModbusDataType type, float scale, float offset);

  uint8_t count() const { return count_; }
  void snapshot(Slave* out, size_t max, size_t& n) const;
  String toJson() const;

private:
  ModbusRtu* rtu_ = nullptr;
  ModbusProfileRegistry* registry_ = nullptr;
  Slave slaves_[MAX_SLAVES];
  uint8_t count_ = 0;

  static uint8_t registersFor(ModbusDataType t);  // nº de registros de 16 bits
  bool readSlave(Slave& s);
};

} // namespace gh
