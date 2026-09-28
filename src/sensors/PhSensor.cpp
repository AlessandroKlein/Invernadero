#include "sensors/PhSensor.hpp"

namespace gh {

void PhSensor::setModbus(uint8_t slaveId, uint16_t reg, float scale) {
  slaveId_ = slaveId;
  reg_ = reg;
  modbusScale_ = scale;
}

void PhSensor::setCalibration(float v4, float v7, float v10) {
  v4_ = v4;
  v7_ = v7;
  v10_ = v10;
}

void PhSensor::begin(Ads1115Driver* ads, ModbusRtu* modbus) {
  ads_ = ads;
  modbus_ = modbus;
}

float PhSensor::voltageToPh(float v) const {
  // El electrodo entrega mayor tensión a menor pH (Nernst).
  // Interpolación lineal por tramos pH4..pH7 y pH7..pH10.
  if (v > v7_) {
    return 7.0f - (v - v7_) * 3.0f / (v4_ - v7_);   // zona ácida
  } else {
    return 7.0f + (v7_ - v) * 3.0f / (v7_ - v10_);  // zona básica
  }
}

bool PhSensor::read(float& ph) {
  if (iface_ == Interface::ANALOG_IF) {
    if (!ads_ || !ads_->available()) return false;
    float v = ads_->readVoltage(adsChannel_);
    // Detección de condiciones anormales: electrodo desconectado / fuera de rango.
    if (v <= 0.0f || v > 4.5f) return false;
    ph = voltageToPh(v);
    // Rango razonable de pH.
    if (ph < 0.0f || ph > 14.0f) return false;
    return true;
  } else {
    if (!modbus_) return false;
    uint16_t raw = 0;
    if (!modbus_->readHoldingRegisters(slaveId_, reg_, 1, &raw)) return false;
    ph = raw * modbusScale_;
    if (ph < 0.0f || ph > 14.0f) return false;
    return true;
  }
}

} // namespace gh
