#include "network/WeatherStation.hpp"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <math.h>

namespace gh {

void WeatherStation::reconfigure(const SystemConfig& cfg) {
  cfg_ = cfg;
  // Reinicia el estado si cambió la configuración (URL/intervalo/keys).
  ok_ = false;
  lastError_ = "";
}

// Lee una magnitud numérica desde un objeto JSON usando una key configurable.
static float readKey(JsonObjectConst obj, const char* key) {
  if (!obj.containsKey(key)) return NAN;
  JsonVariantConst v = obj[key];
  if (v.isNull()) return NAN;
  return v.as<float>();
}

void WeatherStation::loop() {
  if (!cfg_.weatherEnabled) return;
  if (cfg_.weatherUrl[0] == '\0') return;
  uint32_t now = millis();
  if (lastFetchMs_ != 0 && (now - lastFetchMs_) < cfg_.weatherIntervalMs) return;
  lastFetchMs_ = now;
  fetch();
}

bool WeatherStation::fetch() {
  ok_ = false;
  lastError_ = "";

  String url(cfg_.weatherUrl);
  bool https = url.startsWith("https://");

  HTTPClient http;
  if (https) {
    // En esta versión no se valida el certificado (estaciones locales/privadas).
    // Para instalaciones públicas usar certificado CA (secciones 152/153).
    static WiFiClientSecure client;
    client.setInsecure();
    http.begin(client, url);
  } else {
    http.begin(url);
  }

  http.setTimeout(5000);
  int code = http.GET();
  if (code <= 0) {
    lastError_ = String("HTTP ") + code + " (" + HTTPClient::errorToString(code) + ")";
    http.end();
    return false;
  }
  if (code != HTTP_CODE_OK) {
    lastError_ = String("HTTP ") + code;
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    lastError_ = String("JSON inválido: ") + err.c_str();
    return false;
  }

  // Seleccionar el subobjeto raíz si está configurado (p.ej. "weather"/"observations").
  JsonObjectConst root = doc.as<JsonObjectConst>();
  if (cfg_.weatherRoot[0] != '\0') {
    JsonVariantConst sub = doc[cfg_.weatherRoot];
    if (!sub.is<JsonObjectConst>()) {
      lastError_ = String("raíz no encontrada: ") + cfg_.weatherRoot;
      return false;
    }
    root = sub.as<JsonObjectConst>();
  }
  if (root.isNull()) {
    lastError_ = "el JSON no es un objeto";
    return false;
  }

  temp_ = readKey(root, cfg_.weatherKeyTemp);
  hum_ = readKey(root, cfg_.weatherKeyHum);
  wind_ = readKey(root, cfg_.weatherKeyWind);
  rain_ = readKey(root, cfg_.weatherKeyRain);
  pressure_ = readKey(root, cfg_.weatherKeyPressure);
  light_ = readKey(root, cfg_.weatherKeyLight);

  ok_ = true;
  return true;
}

String WeatherStation::toJson() const {
  DynamicJsonDocument doc(512);
  doc["enabled"] = cfg_.weatherEnabled;
  doc["available"] = ok_;
  doc["url"] = cfg_.weatherUrl;
  doc["last_fetch_ms"] = lastFetchMs_;
  if (!isnan(temp_)) doc["temperature"] = temp_;
  if (!isnan(hum_)) doc["humidity"] = hum_;
  if (!isnan(wind_)) doc["wind_speed"] = wind_;
  if (!isnan(rain_)) doc["rain"] = rain_;
  if (!isnan(pressure_)) doc["pressure"] = pressure_;
  if (!isnan(light_)) doc["light"] = light_;
  if (lastError_.length()) doc["error"] = lastError_;
  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
