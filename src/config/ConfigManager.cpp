#include "config/ConfigManager.hpp"
#include "config/Defaults.hpp"
#include "system/Device.hpp"

#include <ArduinoJson.h>

namespace gh {

void ConfigManager::begin() {
  // Crear mutex de protección de la configuración.
  if (mutex_ == nullptr) mutex_ = xSemaphoreCreateMutex();
  // Abrir el espacio de nombres NVS y cargar el JSON guardado.
  prefs_.begin(NVS_NS, false);
  String json = prefs_.getString(NVS_KEY, "");
  String prev = prefs_.getString(NVS_KEY_PREV, "");
  // Cargar la configuración anterior (rollback, sección 104) si existe.
  if (prev.length() > 0) {
    SystemConfig p;
    if (fromJson(prev, p)) prev_ = p;
  }
  if (json.length() > 0) {
    SystemConfig parsed;
    if (fromJson(json, parsed)) {
      cfg_ = parsed; // Configuración previa válida
      ensureAdminPassword();
      return;
    }
  }
  // Si no había configuración (o estaba corrupta), usar valores de fábrica.
  cfg_ = defaults();
  ensureAdminPassword();
  save();
}

SystemConfig ConfigManager::get() const {
  SystemConfig out;
  if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
  out = cfg_;
  if (mutex_) xSemaphoreGive(mutex_);
  return out;
}

void ConfigManager::set(const SystemConfig& c) {
  if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
  prev_ = cfg_;           // Guardar la actual como anterior (rollback, sección 104)
  cfg_ = c;
  cfg_.configVersion++;   // Versionar la nueva configuración
  if (mutex_) xSemaphoreGive(mutex_);
  save();
}

void ConfigManager::save() {
  // Persistir tanto la configuración actual como la anterior (rollback).
  prefs_.putString(NVS_KEY, toJson(cfg_));
  prefs_.putString(NVS_KEY_PREV, toJson(prev_));
  // El password de admin vive en una key separada (no se exporta en el JSON).
  prefs_.putString(NVS_KEY_ADMIN_PASS, cfg_.adminPass);
}

void ConfigManager::ensureAdminPassword() {
  // Si no hay contraseña local, derivarla del UID del dispositivo (sección 254:
  // evita una contraseña universal idéntica para todos los dispositivos).
  if (cfg_.adminPass[0] == '\0') {
    strncpy(cfg_.adminPass, Device::uid(), sizeof(cfg_.adminPass) - 1);
    cfg_.adminPass[sizeof(cfg_.adminPass) - 1] = '\0';
  }
  // Recuperar un password previamente persistido (tiene prioridad).
  String saved = prefs_.getString(NVS_KEY_ADMIN_PASS, "");
  if (saved.length() > 0) strncpy(cfg_.adminPass, saved.c_str(), sizeof(cfg_.adminPass) - 1);
}

bool ConfigManager::auth(const String& user, const String& pass) const {
  if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
  bool ok = (user == String(cfg_.adminUser)) && (pass == String(cfg_.adminPass));
  if (mutex_) xSemaphoreGive(mutex_);
  return ok;
}

String ConfigManager::adminUser() const {
  if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
  String u(cfg_.adminUser);
  if (mutex_) xSemaphoreGive(mutex_);
  return u;
}

void ConfigManager::factoryReset() {
  // FACTORY RESET: restaura todo y descarta el rollback (sección 155).
  if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
  cfg_ = defaults();
  ensureAdminPassword();  // Re-deriva la contraseña local del UID
  prev_ = cfg_;
  if (mutex_) xSemaphoreGive(mutex_);
  save();
}

void ConfigManager::resetNetwork() {
  // RESET NETWORK: solo parámetros de red/servidor/RS485 (sección 155).
  SystemConfig c = get();
  SystemConfig d = defaults();
  strcpy(c.wifiSsid, d.wifiSsid);
  strcpy(c.wifiPass, d.wifiPass);
  strcpy(c.hostname, d.hostname);
  strcpy(c.apSsid, d.apSsid);
  strcpy(c.apPass, d.apPass);
  strcpy(c.mqttHost, d.mqttHost);
  c.mqttPort = d.mqttPort;
  strcpy(c.mqttUser, d.mqttUser);
  strcpy(c.mqttPass, d.mqttPass);
  strcpy(c.ntpServer, d.ntpServer);
  c.timezoneOffset = d.timezoneOffset;
  strcpy(c.timezone, d.timezone);
  strcpy(c.dnsPrimary, d.dnsPrimary);
  strcpy(c.dnsSecondary, d.dnsSecondary);
  c.managedByCentral = false;
  strcpy(c.centralUrl, d.centralUrl);
  c.centralPort = d.centralPort;
  strcpy(c.centralToken, d.centralToken);
  c.rs485Baud = d.rs485Baud;
  c.rs485Parity = d.rs485Parity;
  c.rs485StopBits = d.rs485StopBits;
  set(c);
}

void ConfigManager::resetAutomation() {
  // RESET AUTOMATION: solo funciones y parámetros de control (sección 155).
  SystemConfig c = get();
  SystemConfig d = defaults();
  c.featureClimate = d.featureClimate;
  c.featureIrrigation = d.featureIrrigation;
  c.featureLighting = d.featureLighting;
  c.featureCo2 = d.featureCo2;
  c.featureHeating = d.featureHeating;
  c.featureHumidification = d.featureHumidification;
  c.featureRoof = d.featureRoof;
  c.featureWindows = d.featureWindows;
  c.featureShade = d.featureShade;
  c.tempMin = d.tempMin; c.tempTarget = d.tempTarget; c.tempMax = d.tempMax;
  c.tempEmergency = d.tempEmergency; c.tempHysteresis = d.tempHysteresis;
  c.humMin = d.humMin; c.humTarget = d.humTarget; c.humMax = d.humMax;
  c.humHysteresis = d.humHysteresis;
  c.soilMin = d.soilMin; c.soilTarget = d.soilTarget;
  c.irrigationMaxTimeMs = d.irrigationMaxTimeMs; c.flowMin = d.flowMin;
  c.flowCheckDelayMs = d.flowCheckDelayMs;
  c.ventOnTemp = d.ventOnTemp; c.ventOffTemp = d.ventOffTemp; c.ventHumMax = d.ventHumMax;
  c.roofOpenTemp = d.roofOpenTemp; c.roofCloseTemp = d.roofCloseTemp;
  c.roofCloseOnRain = d.roofCloseOnRain; c.roofCloseOnWind = d.roofCloseOnWind;
  c.windMaxSpeed = d.windMaxSpeed;
  c.lightMinLux = d.lightMinLux; c.lightStartHour = d.lightStartHour;
  c.lightEndHour = d.lightEndHour; c.lightIntensity = d.lightIntensity;
  c.sensorSht31 = d.sensorSht31; c.sensorDs18b20 = d.sensorDs18b20;
  c.sensorSoil = d.sensorSoil; c.sensorLight = d.sensorLight;
  c.sensorCo2 = d.sensorCo2; c.sensorRain = d.sensorRain; c.sensorWind = d.sensorWind;
  c.sensorTank = d.sensorTank; c.sensorFlow = d.sensorFlow;
  c.sensorPh = d.sensorPh; c.sensorEc = d.sensorEc; c.sensorExterior = d.sensorExterior;
  c.actPump = d.actPump; c.actValves = d.actValves; c.actFans = d.actFans;
  c.actExtractors = d.actExtractors; c.actLights = d.actLights;
  c.actHeater = d.actHeater; c.actHumidifier = d.actHumidifier;
  c.actRoof = d.actRoof; c.actWindow = d.actWindow; c.actShade = d.actShade;
  set(c);
}

bool ConfigManager::rollback() {
  // Intercambia la configuración actual con la anterior (sección 104).
  if (mutex_) xSemaphoreTake(mutex_, portMAX_DELAY);
  SystemConfig tmp = cfg_;
  cfg_ = prev_;
  prev_ = tmp;
  if (mutex_) xSemaphoreGive(mutex_);
  save();
  return true;
}

String ConfigManager::toJson(const SystemConfig& c) {
  DynamicJsonDocument doc(8192);
  doc["device"]["id"] = c.deviceId;
  doc["device"]["name"] = c.deviceName;
  doc["device"]["type"] = (int)c.type;
  doc["device"]["greenhouse_id"] = c.greenhouseId;
  doc["config_version"] = c.configVersion;
  doc["configuration_source"] = (c.configSource == ConfigSource::CENTRAL) ? "CENTRAL" : "LOCAL";
  doc["simulation"] = c.simulation;
  doc["update_channel"] = updateChannelString(c.updateChannel);

  JsonObject f = doc.createNestedObject("features");
  f["climate"] = c.featureClimate;
  f["irrigation"] = c.featureIrrigation;
  f["lighting"] = c.featureLighting;
  f["co2"] = c.featureCo2;
  f["heating"] = c.featureHeating;
  f["humidification"] = c.featureHumidification;
  f["roof"] = c.featureRoof;
  f["windows"] = c.featureWindows;
  f["shade"] = c.featureShade;

  JsonObject cl = doc.createNestedObject("climate");
  cl["temp_min"] = c.tempMin;
  cl["temp_target"] = c.tempTarget;
  cl["temp_max"] = c.tempMax;
  cl["temp_emergency"] = c.tempEmergency;
  cl["temp_hysteresis"] = c.tempHysteresis;
  cl["hum_min"] = c.humMin;
  cl["hum_target"] = c.humTarget;
  cl["hum_max"] = c.humMax;
  cl["hum_hysteresis"] = c.humHysteresis;

  JsonObject ir = doc.createNestedObject("irrigation");
  ir["soil_min"] = c.soilMin;
  ir["soil_target"] = c.soilTarget;
  ir["max_time_ms"] = c.irrigationMaxTimeMs;
  ir["flow_min"] = c.flowMin;
  ir["flow_check_delay_ms"] = c.flowCheckDelayMs;

  JsonObject v = doc.createNestedObject("ventilation");
  v["on_temp"] = c.ventOnTemp;
  v["off_temp"] = c.ventOffTemp;
  v["hum_max"] = c.ventHumMax;

  JsonObject r = doc.createNestedObject("roof");
  r["open_temp"] = c.roofOpenTemp;
  r["close_temp"] = c.roofCloseTemp;
  r["close_on_rain"] = c.roofCloseOnRain;
  r["close_on_wind"] = c.roofCloseOnWind;
  r["wind_max"] = c.windMaxSpeed;

  JsonObject li = doc.createNestedObject("lighting");
  li["min_lux"] = c.lightMinLux;
  li["start_hour"] = c.lightStartHour;
  li["end_hour"] = c.lightEndHour;
  li["intensity"] = c.lightIntensity;

  JsonObject cal = doc.createNestedObject("calibration");
  JsonArray sd = cal.createNestedArray("soil_dry");
  JsonArray sw = cal.createNestedArray("soil_wet");
  for (int i = 0; i < 4; i++) { sd.add(c.soilDryRaw[i]); sw.add(c.soilWetRaw[i]); }
  cal["ph4"] = c.ph4Voltage;
  cal["ph7"] = c.ph7Voltage;
  cal["ph10"] = c.ph10Voltage;
  cal["flow_lpp"] = c.flowLitersPerPulse;
  cal["rain_mmp"] = c.rainMmPerPulse;
  cal["wind_khpp"] = c.windKmhPerPulse;
  cal["tank_depth"] = c.tankDepthCm;

  JsonObject net = doc.createNestedObject("network");
  net["ssid"] = c.wifiSsid;
  net["pass"] = c.wifiPass;
  net["hostname"] = c.hostname;
  net["ap_ssid"] = c.apSsid;
  net["ap_pass"] = c.apPass;
  net["mqtt_host"] = c.mqttHost;
  net["mqtt_port"] = c.mqttPort;
  net["mqtt_user"] = c.mqttUser;
  net["mqtt_pass"] = c.mqttPass;
  net["ntp"] = c.ntpServer;
  net["tz"] = c.timezoneOffset;
  net["timezone"] = c.timezone;
  net["dns1"] = c.dnsPrimary;
  net["dns2"] = c.dnsSecondary;
  net["central_managed"] = c.managedByCentral;
  net["central_url"] = c.centralUrl;
  net["central_port"] = c.centralPort;
  net["central_token"] = c.centralToken;
  net["rs485_baud"] = c.rs485Baud;
  net["rs485_parity"] = c.rs485Parity;
  net["rs485_stop"] = c.rs485StopBits;

  JsonObject s = doc.createNestedObject("sensors");
  s["sht31"] = c.sensorSht31;
  s["ds18b20"] = c.sensorDs18b20;
  s["soil"] = c.sensorSoil;
  s["light"] = c.sensorLight;
  s["co2"] = c.sensorCo2;
  s["rain"] = c.sensorRain;
  s["wind"] = c.sensorWind;
  s["tank"] = c.sensorTank;
  s["flow"] = c.sensorFlow;
  s["ph"] = c.sensorPh;
  s["ec"] = c.sensorEc;
  s["exterior"] = c.sensorExterior;

  JsonObject a = doc.createNestedObject("actuators");
  a["pump"] = c.actPump;
  a["valves"] = c.actValves;
  a["fans"] = c.actFans;
  a["extractors"] = c.actExtractors;
  a["lights"] = c.actLights;
  a["heater"] = c.actHeater;
  a["humidifier"] = c.actHumidifier;
  a["roof"] = c.actRoof;
  a["window"] = c.actWindow;
  a["shade"] = c.actShade;

  // Autenticación local (sección 154): solo el usuario, nunca el password.
  JsonObject auth = doc.createNestedObject("auth");
  auth["user"] = c.adminUser;

  // Zonas (sección 120).
  JsonArray zones = doc.createNestedArray("zones");
  for (uint8_t i = 0; i < c.zoneCount && i < 8; i++) zones.add(c.zoneNames[i]);

  String out;
  serializeJson(doc, out);
  return out;
}

bool ConfigManager::fromJson(const String& json, SystemConfig& out) {
  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, json);
  if (err) return false;

  // Copia segura de cadenas acotada a su buffer.
  auto copyStr = [](const char* src, char* dst, size_t n) {
    if (src) { strncpy(dst, src, n - 1); dst[n - 1] = '\0'; }
  };

  if (doc["device"].is<JsonObject>()) {
    copyStr(doc["device"]["id"] | "GH001", out.deviceId, sizeof(out.deviceId));
    copyStr(doc["device"]["name"] | "Invernadero", out.deviceName, sizeof(out.deviceName));
    out.type = (GreenhouseType)(doc["device"]["type"] | 1);
    copyStr(doc["device"]["greenhouse_id"] | "GREENHOUSE-001", out.greenhouseId, sizeof(out.greenhouseId));
  }
  out.configVersion = doc["config_version"] | out.configVersion;
  if (doc["configuration_source"].is<const char*>()) {
    out.configSource = (String(doc["configuration_source"] | "LOCAL") == "CENTRAL")
                         ? ConfigSource::CENTRAL : ConfigSource::LOCAL;
  }
  out.simulation = doc["simulation"] | out.simulation;
  if (doc["update_channel"].is<const char*>()) {
    String ch = doc["update_channel"] | "stable";
    if (ch == "beta") out.updateChannel = UpdateChannel::BETA;
    else if (ch == "development") out.updateChannel = UpdateChannel::DEVELOPMENT;
    else out.updateChannel = UpdateChannel::STABLE;
  }
  if (doc["features"].is<JsonObject>()) {
    JsonObject f = doc["features"];
    out.featureClimate = f["climate"] | true;
    out.featureIrrigation = f["irrigation"] | true;
    out.featureLighting = f["lighting"] | false;
    out.featureCo2 = f["co2"] | false;
    out.featureHeating = f["heating"] | false;
    out.featureHumidification = f["humidification"] | false;
    out.featureRoof = f["roof"] | false;
    out.featureWindows = f["windows"] | false;
    out.featureShade = f["shade"] | false;
  }
  if (doc["climate"].is<JsonObject>()) {
    JsonObject c = doc["climate"];
    out.tempMin = c["temp_min"] | out.tempMin;
    out.tempTarget = c["temp_target"] | out.tempTarget;
    out.tempMax = c["temp_max"] | out.tempMax;
    out.tempEmergency = c["temp_emergency"] | out.tempEmergency;
    out.tempHysteresis = c["temp_hysteresis"] | out.tempHysteresis;
    out.humMin = c["hum_min"] | out.humMin;
    out.humTarget = c["hum_target"] | out.humTarget;
    out.humMax = c["hum_max"] | out.humMax;
    out.humHysteresis = c["hum_hysteresis"] | out.humHysteresis;
  }
  if (doc["irrigation"].is<JsonObject>()) {
    JsonObject ir = doc["irrigation"];
    out.soilMin = ir["soil_min"] | out.soilMin;
    out.soilTarget = ir["soil_target"] | out.soilTarget;
    out.irrigationMaxTimeMs = ir["max_time_ms"] | out.irrigationMaxTimeMs;
    out.flowMin = ir["flow_min"] | out.flowMin;
    out.flowCheckDelayMs = ir["flow_check_delay_ms"] | out.flowCheckDelayMs;
  }
  if (doc["ventilation"].is<JsonObject>()) {
    JsonObject v = doc["ventilation"];
    out.ventOnTemp = v["on_temp"] | out.ventOnTemp;
    out.ventOffTemp = v["off_temp"] | out.ventOffTemp;
    out.ventHumMax = v["hum_max"] | out.ventHumMax;
  }
  if (doc["roof"].is<JsonObject>()) {
    JsonObject r = doc["roof"];
    out.roofOpenTemp = r["open_temp"] | out.roofOpenTemp;
    out.roofCloseTemp = r["close_temp"] | out.roofCloseTemp;
    out.roofCloseOnRain = r["close_on_rain"] | out.roofCloseOnRain;
    out.roofCloseOnWind = r["close_on_wind"] | out.roofCloseOnWind;
    out.windMaxSpeed = r["wind_max"] | out.windMaxSpeed;
  }
  if (doc["lighting"].is<JsonObject>()) {
    JsonObject li = doc["lighting"];
    out.lightMinLux = li["min_lux"] | out.lightMinLux;
    out.lightStartHour = li["start_hour"] | out.lightStartHour;
    out.lightEndHour = li["end_hour"] | out.lightEndHour;
    out.lightIntensity = li["intensity"] | out.lightIntensity;
  }
  if (doc["calibration"].is<JsonObject>()) {
    JsonObject cal = doc["calibration"];
    if (cal["soil_dry"].is<JsonArray>()) {
      JsonArray a = cal["soil_dry"];
      for (int i = 0; i < 4 && i < (int)a.size(); i++) out.soilDryRaw[i] = a[i];
    }
    if (cal["soil_wet"].is<JsonArray>()) {
      JsonArray a = cal["soil_wet"];
      for (int i = 0; i < 4 && i < (int)a.size(); i++) out.soilWetRaw[i] = a[i];
    }
    out.ph4Voltage = cal["ph4"] | out.ph4Voltage;
    out.ph7Voltage = cal["ph7"] | out.ph7Voltage;
    out.ph10Voltage = cal["ph10"] | out.ph10Voltage;
    out.flowLitersPerPulse = cal["flow_lpp"] | out.flowLitersPerPulse;
    out.rainMmPerPulse = cal["rain_mmp"] | out.rainMmPerPulse;
    out.windKmhPerPulse = cal["wind_khpp"] | out.windKmhPerPulse;
    out.tankDepthCm = cal["tank_depth"] | out.tankDepthCm;
  }
  if (doc["network"].is<JsonObject>()) {
    JsonObject n = doc["network"];
    copyStr(n["ssid"], out.wifiSsid, sizeof(out.wifiSsid));
    copyStr(n["pass"], out.wifiPass, sizeof(out.wifiPass));
    copyStr(n["hostname"], out.hostname, sizeof(out.hostname));
    copyStr(n["ap_ssid"], out.apSsid, sizeof(out.apSsid));
    copyStr(n["ap_pass"], out.apPass, sizeof(out.apPass));
    copyStr(n["mqtt_host"], out.mqttHost, sizeof(out.mqttHost));
    out.mqttPort = n["mqtt_port"] | out.mqttPort;
    copyStr(n["mqtt_user"], out.mqttUser, sizeof(out.mqttUser));
    copyStr(n["mqtt_pass"], out.mqttPass, sizeof(out.mqttPass));
    copyStr(n["ntp"], out.ntpServer, sizeof(out.ntpServer));
    out.timezoneOffset = n["tz"] | out.timezoneOffset;
    copyStr(n["timezone"], out.timezone, sizeof(out.timezone));
    copyStr(n["dns1"], out.dnsPrimary, sizeof(out.dnsPrimary));
    copyStr(n["dns2"], out.dnsSecondary, sizeof(out.dnsSecondary));
    out.managedByCentral = n["central_managed"] | out.managedByCentral;
    copyStr(n["central_url"], out.centralUrl, sizeof(out.centralUrl));
    out.centralPort = n["central_port"] | out.centralPort;
    copyStr(n["central_token"], out.centralToken, sizeof(out.centralToken));
    out.rs485Baud = n["rs485_baud"] | out.rs485Baud;
    out.rs485Parity = n["rs485_parity"] | out.rs485Parity;
    out.rs485StopBits = n["rs485_stop"] | out.rs485StopBits;
  }
  if (doc["sensors"].is<JsonObject>()) {
    JsonObject s = doc["sensors"];
    out.sensorSht31 = s["sht31"] | out.sensorSht31;
    out.sensorDs18b20 = s["ds18b20"] | out.sensorDs18b20;
    out.sensorSoil = s["soil"] | out.sensorSoil;
    out.sensorLight = s["light"] | out.sensorLight;
    out.sensorCo2 = s["co2"] | out.sensorCo2;
    out.sensorRain = s["rain"] | out.sensorRain;
    out.sensorWind = s["wind"] | out.sensorWind;
    out.sensorTank = s["tank"] | out.sensorTank;
    out.sensorFlow = s["flow"] | out.sensorFlow;
    out.sensorPh = s["ph"] | out.sensorPh;
    out.sensorEc = s["ec"] | out.sensorEc;
    out.sensorExterior = s["exterior"] | out.sensorExterior;
  }
  if (doc["actuators"].is<JsonObject>()) {
    JsonObject a = doc["actuators"];
    out.actPump = a["pump"] | out.actPump;
    out.actValves = a["valves"] | out.actValves;
    out.actFans = a["fans"] | out.actFans;
    out.actExtractors = a["extractors"] | out.actExtractors;
    out.actLights = a["lights"] | out.actLights;
    out.actHeater = a["heater"] | out.actHeater;
    out.actHumidifier = a["humidifier"] | out.actHumidifier;
    out.actRoof = a["roof"] | out.actRoof;
    out.actWindow = a["window"] | out.actWindow;
    out.actShade = a["shade"] | out.actShade;
  }
  // Autenticación local (sección 154): solo el usuario (el password viaja por NVS separado).
  if (doc["auth"].is<JsonObject>()) {
    copyStr(doc["auth"]["user"] | "admin", out.adminUser, sizeof(out.adminUser));
  }
  // Zonas (sección 120).
  if (doc["zones"].is<JsonArray>()) {
    JsonArray z = doc["zones"];
    out.zoneCount = 0;
    for (uint8_t i = 0; i < z.size() && i < 8; i++) {
      copyStr(z[i] | "", out.zoneNames[i], sizeof(out.zoneNames[i]));
      out.zoneCount++;
    }
  }
  return true;
}

} // namespace gh
