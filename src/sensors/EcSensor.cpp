#include "sensors/EcSensor.hpp"

namespace gh {

void EcSensor::setModbus(uint8_t slaveId, uint16_t reg, float scale) {
  slaveId_ = slaveId;
  reg_ = reg;
  modbusScale_ = scale;
}

void EcSensor::begin(Ads1115Driver* ads, ModbusRtu* modbus) {
  ads_ = ads;
  modbus_ = modbus;
}

bool EcSensor::read(float& ecMsCm) {
  if (iface_ == Interface::MODBUS_IF) {
    if (!modbus_) return false;
    uint16_t raw = 0;
    if (!modbus_->readHoldingRegisters(slaveId_, reg_, 1, &raw)) return false;
    ecMsCm = raw * modbusScale_; // µS/cm -> mS/cm
    return ecMsCm >= 0.0f;
  } else {
    if (!ads_ || !ads_->available()) return false;
    float v = ads_->readVoltage(adsChannel_);
    if (v <= 0.0f) return false;
    // Estimación básica proporcional (debe calibrarse con solución patrón).
    ecMsCm = v * 1.0f;
    return true;
  }
}

} // namespace gh
