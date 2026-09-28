#include "sensors/SensorManager.hpp"

#include <ArduinoJson.h>
#include "core/PinMap.hpp"

namespace gh {

// Índices de slots internos (layout fijo del firmware).
enum Slot : uint8_t {
  S_TEMP = 0, S_HUM = 1, S_EXT_TEMP = 2, S_EXT_HUM = 3,
  S_DS18 = 4,               // S_DS18 .. S_DS18+3
  S_SOIL = 8,               // S_SOIL .. S_SOIL+3
  S_LIGHT = 12, S_CO2 = 13, S_FLOW = 14, S_TANK = 15,
  S_RAIN = 16, S_WIND = 17, S_PH = 18, S_EC = 19
};

void SensorManager::begin(const SystemConfig& cfg) {
  cfg_ = cfg;
  if (mutex_ == nullptr) mutex_ = xSemaphoreCreateMutex();

  // Inicializar bus I²C principal.
  Wire.begin(pins::I2C_SDA, pins::I2C_SCL, pins::I2C_FREQ);

  // Sensores I²C.
  sht31_.begin(TempHumSensor::Kind::SHT31, pins::I2C_ADDR_SHT31, &Wire);
  exterior_.begin(TempHumSensor::Kind::AHT20, pins::I2C_ADDR_AHT20, &Wire);
  ads_.begin(pins::I2C_ADDR_ADS1115, &Wire);
  light_.begin(pins::I2C_ADDR_BH1750, &Wire);

  // CO₂ (opcional): iniciar medición periódica si el chip responde.
  co2_.begin(pins::I2C_ADDR_SCD41, &Wire);

  // DS18B20 en bus 1-Wire.
  ds18b20_.begin(pins::ONEWIRE_PIN);

  // Entradas de pulsos (caudal, lluvia, viento).
  flow_.begin(pins::FLOW_PIN, cfg_.flowLitersPerPulse, true);
  rain_.begin(pins::RAIN_PIN, cfg_.rainMmPerPulse, true);
  wind_.begin(pins::WIND_PIN, cfg_.windKmhPerPulse, true);

  // Nivel de tanque (ultrasónico) y flotadores de seguridad.
  tank_.begin(pins::TANK_TRIG, pins::TANK_ECHO, cfg_.tankDepthCm);
  pinMode(pins::FLOAT_LOW_PIN, INPUT_PULLUP);
  pinMode(pins::FLOAT_HIGH_PIN, INPUT_PULLUP);

  // RS485 / Modbus RTU para pH y EC industriales.
  rs485_ = &Serial1;
  rs485_->begin(9600, SERIAL_8N1, pins::RS485_RX, pins::RS485_TX);
  modbus_.begin(rs485_, pins::RS485_DE, 9600);

  // Sensores de química.
  ph_.setCalibration(cfg_.ph4Voltage, cfg_.ph7Voltage, cfg_.ph10Voltage);
  ph_.begin(&ads_, &modbus_);
  ec_.begin(&ads_, &modbus_);

  // Estado inicial de slots.
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    values_[i].type = SensorType::NONE;
    values_[i].status = SensorStatus::UNKNOWN;
  }

  reconfigure(cfg);
}

void SensorManager::reconfigure(const SystemConfig& cfg) {
  cfg_ = cfg;
  // Recalibrar pH y tanque.
  ph_.setCalibration(cfg_.ph4Voltage, cfg_.ph7Voltage, cfg_.ph10Voltage);
  tank_.begin(pins::TANK_TRIG, pins::TANK_ECHO, cfg_.tankDepthCm);
  // Actualizar habilitados en los slots.
  for (uint8_t i = 0; i < MAX_SENSORS; i++) values_[i].enabled = false;
}

void SensorManager::setValue(uint8_t idx, SensorType t, const char* name, uint8_t zone,
                             bool enabled, float val, float raw, SensorStatus st, const char* unit) {
  if (idx >= MAX_SENSORS) return;
  values_[idx].type = t;
  values_[idx].name = name;
  values_[idx].zone = zone;
  values_[idx].enabled = enabled;
  values_[idx].value = val;
  values_[idx].raw = raw;
  values_[idx].status = st;
  values_[idx].unit = unit;
  values_[idx].lastReadMs = millis();
}

// --- Getters tipados ---

float SensorManager::temperature() const { return values_[S_TEMP].value; }
float SensorManager::humidity() const { return values_[S_HUM].value; }
float SensorManager::exteriorTemperature() const { return values_[S_EXT_TEMP].value; }
float SensorManager::exteriorHumidity() const { return values_[S_EXT_HUM].value; }

float SensorManager::soilMoisture(uint8_t zone) const {
  if (zone >= MAX_SOIL_ZONES) return NAN;
  return values_[S_SOIL + zone].value;
}

float SensorManager::lightLux() const { return values_[S_LIGHT].value; }
float SensorManager::co2() const { return values_[S_CO2].value; }
float SensorManager::flowRate() const { return values_[S_FLOW].value; }
float SensorManager::flowAccumulated() const { return flow_.accumulated(); }
float SensorManager::tankLevel() const { return values_[S_TANK].value; }
bool SensorManager::floatLow() const { return digitalRead(pins::FLOAT_LOW_PIN) == LOW; }
bool SensorManager::floatHigh() const { return digitalRead(pins::FLOAT_HIGH_PIN) == LOW; }
float SensorManager::rainAccum() const { return values_[S_RAIN].value; }
float SensorManager::windSpeed() const { return values_[S_WIND].value; }
float SensorManager::ph() const { return values_[S_PH].value; }
float SensorManager::ec() const { return values_[S_EC].value; }

SensorStatus SensorManager::statusOf(uint8_t idx) const {
  if (idx >= MAX_SENSORS) return SensorStatus::UNKNOWN;
  return values_[idx].status;
}

// Convierte lectura ADC de humedad de suelo a % usando calibración seca/húmeda.
static float soilToPercent(float raw, float wetRaw, float dryRaw) {
  if (raw >= dryRaw) return 0.0f;   // completamente seco
  if (raw <= wetRaw) return 100.0f; // condición húmeda de calibración
  return 100.0f * (dryRaw - raw) / (dryRaw - wetRaw);
}

void SensorManager::update() {
  uint32_t now = millis();

  // --- Temperatura/Humedad interior (SHT31) ---
  if (cfg_.sensorSht31) {
    float t, h;
    if (sht31_.read(t, h)) {
      setValue(S_TEMP, SensorType::TEMP_SHT31, "Temperatura", 0, true, t, t, SensorStatus::OK, "°C");
      setValue(S_HUM, SensorType::HUM_SHT31, "Humedad", 0, true, h, h, SensorStatus::OK, "%");
    } else {
      setValue(S_TEMP, SensorType::TEMP_SHT31, "Temperatura", 0, true, 0, 0,
               sht31_.available() ? SensorStatus::ERROR : SensorStatus::DISCONNECTED, "°C");
      setValue(S_HUM, SensorType::HUM_SHT31, "Humedad", 0, true, 0, 0,
               sht31_.available() ? SensorStatus::ERROR : SensorStatus::DISCONNECTED, "%");
    }
  } else {
    setValue(S_TEMP, SensorType::TEMP_SHT31, "Temperatura", 0, false, 0, 0, SensorStatus::UNKNOWN, "°C");
    setValue(S_HUM, SensorType::HUM_SHT31, "Humedad", 0, false, 0, 0, SensorStatus::UNKNOWN, "%");
  }

  // --- Temperatura/Humedad exterior (AHT20) ---
  if (cfg_.sensorExterior) {
    float t, h;
    if (exterior_.read(t, h)) {
      setValue(S_EXT_TEMP, SensorType::TEMP_EXTERIOR, "Temp. exterior", 0, true, t, t, SensorStatus::OK, "°C");
      setValue(S_EXT_HUM, SensorType::HUM_EXTERIOR, "Hum. exterior", 0, true, h, h, SensorStatus::OK, "%");
    } else {
      setValue(S_EXT_TEMP, SensorType::TEMP_EXTERIOR, "Temp. exterior", 0, true, 0, 0, SensorStatus::ERROR, "°C");
      setValue(S_EXT_HUM, SensorType::HUM_EXTERIOR, "Hum. exterior", 0, true, 0, 0, SensorStatus::ERROR, "%");
    }
  } else {
    setValue(S_EXT_TEMP, SensorType::TEMP_EXTERIOR, "Temp. exterior", 0, false, 0, 0, SensorStatus::UNKNOWN, "°C");
    setValue(S_EXT_HUM, SensorType::HUM_EXTERIOR, "Hum. exterior", 0, false, 0, 0, SensorStatus::UNKNOWN, "%");
  }

  // --- DS18B20 (bus 1-Wire) ---
  if (cfg_.sensorDs18b20 && ds18b20_.available()) {
    // Disparar conversión cada 5 s y leer 800 ms después (no bloqueante).
    if (now - lastDs18Request_ > 5000) {
      ds18b20_.requestTemperatures();
      lastDs18Request_ = now;
      ds18Pending_ = true;
    }
    if (ds18Pending_ && now - lastDs18Request_ > 800) {
      uint8_t n = ds18b20_.count() < 4 ? ds18b20_.count() : 4;
      for (uint8_t i = 0; i < 4; i++) {
        if (i < n) {
          float t;
          bool ok = ds18b20_.readTemperature(i, t);
          setValue(S_DS18 + i, SensorType::TEMP_DS18B20, "DS18B20", i, true, t, t,
                   ok ? SensorStatus::OK : SensorStatus::ERROR, "°C");
        } else {
          setValue(S_DS18 + i, SensorType::TEMP_DS18B20, "DS18B20", i, false, 0, 0, SensorStatus::UNKNOWN, "°C");
        }
      }
      ds18Pending_ = false;
    }
  } else {
    for (uint8_t i = 0; i < 4; i++)
      setValue(S_DS18 + i, SensorType::TEMP_DS18B20, "DS18B20", i, false, 0, 0, SensorStatus::DISCONNECTED, "°C");
  }

  // --- Humedad de suelo (ADS1115) ---
  if (cfg_.sensorSoil && ads_.available()) {
    for (uint8_t z = 0; z < MAX_SOIL_ZONES; z++) {
      float raw = ads_.readRaw(z);
      float pct = soilToPercent(raw, cfg_.soilWetRaw[z], cfg_.soilDryRaw[z]);
      setValue(S_SOIL + z, SensorType::SOIL_MOISTURE, "Suelo", z + 1, true, pct, raw, SensorStatus::OK, "%");
    }
  } else {
    for (uint8_t z = 0; z < MAX_SOIL_ZONES; z++)
      setValue(S_SOIL + z, SensorType::SOIL_MOISTURE, "Suelo", z + 1, false, 0, 0,
               ads_.available() ? SensorStatus::ERROR : SensorStatus::DISCONNECTED, "%");
  }

  // --- Iluminación (BH1750) ---
  if (cfg_.sensorLight) {
    float lux = light_.readLux();
    if (!isnan(lux)) setValue(S_LIGHT, SensorType::LIGHT_LUX, "Luz", 0, true, lux, lux, SensorStatus::OK, "lux");
    else setValue(S_LIGHT, SensorType::LIGHT_LUX, "Luz", 0, true, 0, 0,
                  light_.available() ? SensorStatus::ERROR : SensorStatus::DISCONNECTED, "lux");
  } else {
    setValue(S_LIGHT, SensorType::LIGHT_LUX, "Luz", 0, false, 0, 0, SensorStatus::UNKNOWN, "lux");
  }

  // --- CO₂ (SCD4x) ---
  if (cfg_.sensorCo2) {
    if (!co2Started_) { co2_.startPeriodicMeasurement(); co2Started_ = true; }
    uint16_t c; float t, h;
    if (co2_.readMeasurement(c, t, h)) setValue(S_CO2, SensorType::CO2, "CO₂", 0, true, c, c, SensorStatus::OK, "ppm");
    else setValue(S_CO2, SensorType::CO2, "CO₂", 0, true, 0, 0, SensorStatus::ERROR, "ppm");
  } else {
    setValue(S_CO2, SensorType::CO2, "CO₂", 0, false, 0, 0, SensorStatus::UNKNOWN, "ppm");
  }

  // --- Caudal ---
  if (cfg_.sensorFlow) {
    float rate = flow_.ratePerMinute();
    setValue(S_FLOW, SensorType::FLOW_RATE, "Caudal", 0, true, rate, rate, SensorStatus::OK, "L/min");
  } else {
    setValue(S_FLOW, SensorType::FLOW_RATE, "Caudal", 0, false, 0, 0, SensorStatus::UNKNOWN, "L/min");
  }

  // --- Tanque ---
  if (cfg_.sensorTank) {
    float lvl = tank_.readLevelPercent();
    if (!isnan(lvl)) setValue(S_TANK, SensorType::TANK_LEVEL, "Tanque", 0, true, lvl, lvl, SensorStatus::OK, "%");
    else setValue(S_TANK, SensorType::TANK_LEVEL, "Tanque", 0, true, 0, 0, SensorStatus::ERROR, "%");
  } else {
    setValue(S_TANK, SensorType::TANK_LEVEL, "Tanque", 0, false, 0, 0, SensorStatus::UNKNOWN, "%");
  }

  // --- Lluvia ---
  if (cfg_.sensorRain) {
    setValue(S_RAIN, SensorType::RAIN_ACCUM, "Lluvia", 0, true, rain_.accumulated(), 0, SensorStatus::OK, "mm");
  } else {
    setValue(S_RAIN, SensorType::RAIN_ACCUM, "Lluvia", 0, false, 0, 0, SensorStatus::UNKNOWN, "mm");
  }

  // --- Viento ---
  if (cfg_.sensorWind) {
    setValue(S_WIND, SensorType::WIND_SPEED, "Viento", 0, true, wind_.ratePerMinute(), 0, SensorStatus::OK, "km/h");
  } else {
    setValue(S_WIND, SensorType::WIND_SPEED, "Viento", 0, false, 0, 0, SensorStatus::UNKNOWN, "km/h");
  }

  // --- pH ---
  if (cfg_.sensorPh) {
    float ph;
    if (ph_.read(ph)) setValue(S_PH, SensorType::PH, "pH", 0, true, ph, ph, SensorStatus::OK, "");
    else setValue(S_PH, SensorType::PH, "pH", 0, true, 0, 0, SensorStatus::ERROR, "");
  } else {
    setValue(S_PH, SensorType::PH, "pH", 0, false, 0, 0, SensorStatus::UNKNOWN, "");
  }

  // --- EC ---
  if (cfg_.sensorEc) {
    float ec;
    if (ec_.read(ec)) setValue(S_EC, SensorType::EC, "EC", 0, true, ec, ec, SensorStatus::OK, "mS/cm");
    else setValue(S_EC, SensorType::EC, "EC", 0, true, 0, 0, SensorStatus::ERROR, "mS/cm");
  } else {
    setValue(S_EC, SensorType::EC, "EC", 0, false, 0, 0, SensorStatus::UNKNOWN, "mS/cm");
  }
}

void SensorManager::snapshot(SensorValue* out, size_t max, size_t& n) const {
  if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
  n = 0;
  for (uint8_t i = 0; i < MAX_SENSORS && n < max; i++) {
    if (values_[i].enabled) out[n++] = values_[i];
  }
  if (mutex_) xSemaphoreGive(mutex_);
}

String SensorManager::toJson() const {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    if (!values_[i].enabled) continue;
    JsonObject o = arr.createNestedObject();
    o["name"] = values_[i].name;
    o["zone"] = values_[i].zone;
    o["value"] = values_[i].value;
    o["status"] = (int)values_[i].status;
    o["unit"] = values_[i].unit;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
