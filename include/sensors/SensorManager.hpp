#pragma once
// Orquestador de sensores. Abstrae el hardware: los controladores solo leen
// magnitudes tipadas (temperature(), soilMoisture(), ...) sin conocer el modelo
// físico del sensor (sección 81).

#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "core/Types.hpp"
#include "control/CalculatedVariables.hpp"
#include "sensors/TempHumSensor.hpp"
#include "sensors/Ds18b20Sensor.hpp"
#include "sensors/Ads1115Driver.hpp"
#include "sensors/LightSensor.hpp"
#include "sensors/Co2Sensor.hpp"
#include "sensors/PulseCounter.hpp"
#include "sensors/UltrasonicLevel.hpp"
#include "sensors/PhSensor.hpp"
#include "sensors/EcSensor.hpp"
#include "hardware/ModbusRtu.hpp"

namespace gh {

class SensorManager {
public:
  static constexpr uint8_t MAX_SENSORS = 20;
  static constexpr uint8_t MAX_SOIL_ZONES = 4;

  void begin(const SystemConfig& cfg);
  void reconfigure(const SystemConfig& cfg); // Actualiza habilitados/calibración
  void update();                             // Lee todos los sensores habilitados

  // Snapshot para API/WebSocket (protegido por mutex).
  void snapshot(SensorValue* out, size_t max, size_t& n) const;
  String toJson() const;

  // Getters tipados (lecturas atómicas; llamar tras update()).
  float temperature() const;
  float humidity() const;
  float exteriorTemperature() const;
  float exteriorHumidity() const;
  float soilMoisture(uint8_t zone) const; // 0..MAX_SOIL_ZONES-1
  float lightLux() const;
  float co2() const;
  float flowRate() const;       // L/min
  float flowAccumulated() const; // L
  float tankLevel() const;      // %
  bool floatLow() const;
  bool floatHigh() const;
  float rainAccum() const;      // mm
  float rainRate() const { return rain_.ratePerMinute(); } // mm/min (para detectar lluvia activa)
  float windSpeed() const;      // km/h
  float ph() const;
  float ec() const;             // mS/cm
  // Variables calculadas (sección 235).
  float vpd() const;
  float dewPoint() const;
  SensorStatus statusOf(uint8_t idx) const;

  // Acceso a drivers (para diagnóstico).
  bool sht31Available() const { return sht31_.available(); }
  bool adsAvailable() const { return ads_.available(); }
  uint8_t ds18b20Count() const { return ds18b20_.count(); }
  bool lightAvailable() const { return light_.available(); }
  bool co2Available() const { return co2_.available(); }

  // Diagnóstico RS485/Modbus (secciones 115/177).
  uint8_t scanModbus(uint8_t* found, uint8_t maxFound) { return modbus_.scan(found, maxFound); }
  ModbusStats modbusStats() const { return modbus_.stats(); }

  // Herramienta Modbus de mantenimiento (sección 178).
  bool modbusReadHolding(uint8_t slaveId, uint16_t addr, uint16_t count, uint16_t* out) {
    return modbus_.readHoldingRegisters(slaveId, addr, count, out);
  }
  bool modbusReadInput(uint8_t slaveId, uint16_t addr, uint16_t count, uint16_t* out) {
    return modbus_.readInputRegisters(slaveId, addr, count, out);
  }

private:
  // Drivers de hardware.
  TempHumSensor sht31_;       // interior (SHT31)
  TempHumSensor exterior_;    // exterior (AHT20)
  Ds18b20Sensor ds18b20_;
  Ads1115Driver ads_;
  LightSensor light_;
  Co2Sensor co2_;
  PulseCounter flow_, rain_, wind_;
  UltrasonicLevel tank_;
  PhSensor ph_;
  EcSensor ec_;
  ModbusRtu modbus_;
  HardwareSerial* rs485_ = nullptr;

  // Valores en tiempo de ejecución.
  SensorValue values_[MAX_SENSORS];
  SystemConfig cfg_;
  mutable SemaphoreHandle_t mutex_ = nullptr;
  uint32_t lastDs18Request_ = 0;
  bool ds18Pending_ = false;
  bool co2Started_ = false;

  void setValue(uint8_t idx, SensorType t, const char* name, uint8_t zone,
                bool enabled, float val, float raw, SensorStatus st, const char* unit);
};

} // namespace gh
