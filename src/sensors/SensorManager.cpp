#include "sensors/SensorManager.hpp"

#include <ArduinoJson.h>
#include <math.h>
#include "core/PinConfig.hpp"
#include "sensors/SensorRegistry.hpp"

namespace gh {

// Índices de slots internos (layout fijo del firmware).
enum Slot : uint8_t {
  S_TEMP = 0, S_HUM = 1, S_EXT_TEMP = 2, S_EXT_HUM = 3,
  S_DS18 = 4,               // S_DS18 .. S_DS18+3
  S_SOIL = 8,               // S_SOIL .. S_SOIL+3
  S_LIGHT = 12, S_CO2 = 13, S_FLOW = 14, S_TANK = 15,
  S_RAIN = 16, S_WIND = 17, S_PH = 18, S_EC = 19
};

void SensorManager::begin(const SystemConfig& cfg, const PinConfig& pins, SensorRegistry* registry) {
  cfg_ = cfg;
  pins_ = pins;
  registry_ = registry;
  if (mutex_ == nullptr) mutex_ = xSemaphoreCreateMutex();

  // Inicializar bus I²C principal.
  Wire.begin(pins_.i2cSda, pins_.i2cScl, pins_.i2cFreq);

  // Resuelve la dirección I²C desde el catálogo (editable por web); si no hay
  // catálogo o la entrada no existe, usa el default del PinConfig.
  auto addr = [&](const char* id, uint8_t defAddr) -> uint8_t {
    SensorEntry e;
    if (registry_ && registry_->get(id, e)) return e.address;
    return defAddr;
  };

  // Sensores I²C (dirección desde el catálogo instanciable).
  sht31_.begin(TempHumSensor::Kind::SHT31, addr("temp_interior", pins_.i2cAddrSht31), &Wire);
  exterior_.begin(TempHumSensor::Kind::AHT20, addr("temp_exterior", pins_.i2cAddrAht20), &Wire);
  ads_.begin(addr("soil_0", pins_.i2cAddrAds1115), &Wire);
  light_.begin(addr("light", pins_.i2cAddrBh1750), &Wire);

  // CO₂ (opcional): iniciar medición periódica si el chip responde.
  co2_.begin(addr("co2", pins_.i2cAddrScd41), &Wire);

  // DS18B20 en bus 1-Wire.
  ds18b20_.begin(pins_.oneWire);

  // Entradas de pulsos (caudal, lluvia, viento).
  flow_.begin(pins_.flowPin, cfg_.flowLitersPerPulse, true);
  rain_.begin(pins_.rainPin, cfg_.rainMmPerPulse, true);
  wind_.begin(pins_.windPin, cfg_.windKmhPerPulse, true);

  // Nivel de tanque (ultrasónico) y flotadores de seguridad.
  tank_.begin(pins_.tankTrig, pins_.tankEcho, cfg_.tankDepthCm);
  pinMode(pins_.floatLow, INPUT_PULLUP);
  pinMode(pins_.floatHigh, INPUT_PULLUP);

  // RS485 / Modbus RTU para pH y EC industriales.
  rs485_ = &Serial1;
  rs485_->begin(9600, SERIAL_8N1, pins_.rs485Rx, pins_.rs485Tx);
  modbus_.begin(rs485_, pins_.rs485De, 9600);

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
  tank_.begin(pins_.tankTrig, pins_.tankEcho, cfg_.tankDepthCm);
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

// Lectura protegida: permite que ControlTask (núcleo 0) lea mientras SensorTask
// escribe. Se protege cada acceso individual; un ciclo de control puede ver una
// mezcla de valores de dos ciclos consecutivos (cadencia de 2 s), lo cual es
// aceptable para control por histéresis.
float SensorManager::valueAt(uint8_t idx) const {
  if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
  float v = values_[idx].value;
  if (mutex_) xSemaphoreGive(mutex_);
  return v;
}

float SensorManager::temperature() const { return valueAt(S_TEMP); }
float SensorManager::humidity() const { return valueAt(S_HUM); }
float SensorManager::exteriorTemperature() const { return valueAt(S_EXT_TEMP); }
float SensorManager::exteriorHumidity() const { return valueAt(S_EXT_HUM); }

float SensorManager::soilMoisture(uint8_t zone) const {
  if (zone >= MAX_SOIL_ZONES) return NAN;
  return valueAt(S_SOIL + zone);
}

float SensorManager::lightLux() const { return valueAt(S_LIGHT); }
float SensorManager::co2() const { return valueAt(S_CO2); }
float SensorManager::flowRate() const { return valueAt(S_FLOW); }
float SensorManager::flowAccumulated() const { return flow_.accumulated(); }
float SensorManager::tankLevel() const { return valueAt(S_TANK); }
bool SensorManager::floatLow() const { return digitalRead(pins_.floatLow) == LOW; }
bool SensorManager::floatHigh() const { return digitalRead(pins_.floatHigh) == LOW; }
float SensorManager::rainAccum() const { return valueAt(S_RAIN); }
float SensorManager::windSpeed() const { return valueAt(S_WIND); }
float SensorManager::ph() const { return valueAt(S_PH); }
float SensorManager::ec() const { return valueAt(S_EC); }

// Variables calculadas (sección 235): derivadas de temperatura + humedad.
float SensorManager::vpd() const { return calc::vpd(temperature(), humidity()); }
float SensorManager::dewPoint() const { return calc::dewPoint(temperature(), humidity()); }

SensorStatus SensorManager::statusOf(uint8_t idx) const {
  if (idx >= MAX_SENSORS) return SensorStatus::UNKNOWN;
  if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
  SensorStatus s = values_[idx].status;
  if (mutex_) xSemaphoreGive(mutex_);
  return s;
}

// Convierte lectura ADC de humedad de suelo a % usando calibración seca/húmeda.
static float soilToPercent(float raw, float wetRaw, float dryRaw) {
  if (raw >= dryRaw) return 0.0f;   // completamente seco
  if (raw <= wetRaw) return 100.0f; // condición húmeda de calibración
  return 100.0f * (dryRaw - raw) / (dryRaw - wetRaw);
}

void SensorManager::update() {
  uint32_t now = millis();

  // --- Modo simulación (sección 191): genera datos sintéticos sin hardware. ---
  if (cfg_.simulation) {
    float t = now / 1000.0f;
    setValue(S_TEMP, SensorType::TEMP_SHT31, "Temperatura", 0, true,
             23.0f + 3.0f * sinf(t / 45.0f), 0, SensorStatus::OK, "°C");
    setValue(S_HUM, SensorType::HUM_SHT31, "Humedad", 0, true,
             65.0f + 10.0f * sinf(t / 60.0f), 0, SensorStatus::OK, "%");
    for (uint8_t z = 0; z < MAX_SOIL_ZONES; z++)
      setValue(S_SOIL + z, SensorType::SOIL_MOISTURE, "Suelo", z + 1, true,
               50.0f + 10.0f * sinf(t / 30.0f + z), 0, SensorStatus::OK, "%");
    float day = (sinf(t / 120.0f) + 1.0f) / 2.0f; // ciclo día/noche simulado
    setValue(S_LIGHT, SensorType::LIGHT_LUX, "Luz", 0, true, day * 30000.0f, 0, SensorStatus::OK, "lux");
    setValue(S_CO2, SensorType::CO2, "CO₂", 0, true, 500.0f + 200.0f * sinf(t / 50.0f), 0, SensorStatus::OK, "ppm");
    setValue(S_TANK, SensorType::TANK_LEVEL, "Tanque", 0, true, 72.0f, 0, SensorStatus::OK, "%");
    setValue(S_FLOW, SensorType::FLOW_RATE, "Caudal", 0, true, 0.0f, 0, SensorStatus::OK, "L/min");
    setValue(S_PH, SensorType::PH, "pH", 0, true, 6.5f + 0.3f * sinf(t / 40.0f), 0, SensorStatus::OK, "");
    setValue(S_EC, SensorType::EC, "EC", 0, true, 1.8f, 0, SensorStatus::OK, "mS/cm");
    return;
  }

  // El catálogo instanciable controla el "enabled" de cada sensor (editable por
  // web); si no hay catálogo o entrada, se usa el flag de SystemConfig.
  auto on = [&](const char* id, bool def) -> bool {
    SensorEntry e;
    if (registry_ && registry_->get(id, e)) return e.enabled;
    return def;
  };

  // --- Temperatura/Humedad interior (SHT31) ---
  if (on("temp_interior", cfg_.sensorSht31)) {
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
  if (on("temp_exterior", cfg_.sensorExterior)) {
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
  if (on("temp_18b20", cfg_.sensorDs18b20) && ds18b20_.available()) {
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
  if (on("soil_0", cfg_.sensorSoil) && ads_.available()) {
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
  if (on("light", cfg_.sensorLight)) {
    float lux = light_.readLux();
    if (!isnan(lux)) setValue(S_LIGHT, SensorType::LIGHT_LUX, "Luz", 0, true, lux, lux, SensorStatus::OK, "lux");
    else setValue(S_LIGHT, SensorType::LIGHT_LUX, "Luz", 0, true, 0, 0,
                  light_.available() ? SensorStatus::ERROR : SensorStatus::DISCONNECTED, "lux");
  } else {
    setValue(S_LIGHT, SensorType::LIGHT_LUX, "Luz", 0, false, 0, 0, SensorStatus::UNKNOWN, "lux");
  }

  // --- CO₂ (SCD4x) ---
  if (on("co2", cfg_.sensorCo2)) {
    if (!co2Started_) { co2_.startPeriodicMeasurement(); co2Started_ = true; }
    uint16_t c; float t, h;
    if (co2_.readMeasurement(c, t, h)) setValue(S_CO2, SensorType::CO2, "CO₂", 0, true, c, c, SensorStatus::OK, "ppm");
    else setValue(S_CO2, SensorType::CO2, "CO₂", 0, true, 0, 0, SensorStatus::ERROR, "ppm");
  } else {
    setValue(S_CO2, SensorType::CO2, "CO₂", 0, false, 0, 0, SensorStatus::UNKNOWN, "ppm");
  }

  // --- Caudal ---
  if (on("flow", cfg_.sensorFlow)) {
    float rate = flow_.ratePerMinute();
    setValue(S_FLOW, SensorType::FLOW_RATE, "Caudal", 0, true, rate, rate, SensorStatus::OK, "L/min");
  } else {
    setValue(S_FLOW, SensorType::FLOW_RATE, "Caudal", 0, false, 0, 0, SensorStatus::UNKNOWN, "L/min");
  }

  // --- Tanque ---
  if (on("tank", cfg_.sensorTank)) {
    float lvl = tank_.readLevelPercent();
    if (!isnan(lvl)) setValue(S_TANK, SensorType::TANK_LEVEL, "Tanque", 0, true, lvl, lvl, SensorStatus::OK, "%");
    else setValue(S_TANK, SensorType::TANK_LEVEL, "Tanque", 0, true, 0, 0, SensorStatus::ERROR, "%");
  } else {
    setValue(S_TANK, SensorType::TANK_LEVEL, "Tanque", 0, false, 0, 0, SensorStatus::UNKNOWN, "%");
  }

  // --- Lluvia ---
  if (on("rain", cfg_.sensorRain)) {
    setValue(S_RAIN, SensorType::RAIN_ACCUM, "Lluvia", 0, true, rain_.accumulated(), 0, SensorStatus::OK, "mm");
  } else {
    setValue(S_RAIN, SensorType::RAIN_ACCUM, "Lluvia", 0, false, 0, 0, SensorStatus::UNKNOWN, "mm");
  }

  // --- Viento ---
  if (on("wind", cfg_.sensorWind)) {
    setValue(S_WIND, SensorType::WIND_SPEED, "Viento", 0, true, wind_.ratePerMinute(), 0, SensorStatus::OK, "km/h");
  } else {
    setValue(S_WIND, SensorType::WIND_SPEED, "Viento", 0, false, 0, 0, SensorStatus::UNKNOWN, "km/h");
  }

  // --- pH ---
  if (on("ph", cfg_.sensorPh)) {
    float ph;
    if (ph_.read(ph)) setValue(S_PH, SensorType::PH, "pH", 0, true, ph, ph, SensorStatus::OK, "");
    else setValue(S_PH, SensorType::PH, "pH", 0, true, 0, 0, SensorStatus::ERROR, "");
  } else {
    setValue(S_PH, SensorType::PH, "pH", 0, false, 0, 0, SensorStatus::UNKNOWN, "");
  }

  // --- EC ---
  if (on("ec", cfg_.sensorEc)) {
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
    o["quality"] = sensorStatusString(values_[i].status); // sección 176
    o["unit"] = values_[i].unit;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
