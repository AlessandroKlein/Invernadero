#include "system/Logger.hpp"

#include <ArduinoJson.h>

namespace gh {

const char* Logger::levelString(uint8_t lvl) const {
  switch ((LogLevel)lvl) {
    case LogLevel::TRACE:    return "TRACE";
    case LogLevel::DEBUG:    return "DEBUG";
    case LogLevel::INFO:     return "INFO";
    case LogLevel::NOTICE:   return "NOTICE";
    case LogLevel::WARNING:  return "WARNING";
    case LogLevel::ERROR:    return "ERROR";
    case LogLevel::CRITICAL: return "CRITICAL";
    default:                 return "INFO";
  }
}

void Logger::begin(LogLevel minLevel) {
  minLevel_ = minLevel;
  for (uint8_t i = 0; i < MAX_ENTRIES; i++) entries_[i].message[0] = '\0';
  count_ = 0;
  head_ = 0;
}

void Logger::log(LogLevel lvl, const char* module, const char* msg) {
  if ((uint8_t)lvl < (uint8_t)minLevel_) return;

  // Salida a Serial para niveles INFO o superiores.
  if ((uint8_t)lvl >= (uint8_t)LogLevel::INFO) {
    Serial.printf("[%s] %s: %s\n", levelString((uint8_t)lvl),
                  module ? module : "core", msg ? msg : "");
  }

  Entry& e = entries_[head_];
  e.timestamp = millis();
  e.level = (uint8_t)lvl;
  strncpy(e.module, module ? module : "", sizeof(e.module) - 1);
  e.module[sizeof(e.module) - 1] = '\0';
  strncpy(e.message, msg ? msg : "", sizeof(e.message) - 1);
  e.message[sizeof(e.message) - 1] = '\0';
  head_ = (head_ + 1) % MAX_ENTRIES;
  if (count_ < MAX_ENTRIES) count_++;
}

void Logger::snapshot(Entry* out, size_t max, size_t& n) const {
  n = 0;
  // Del más antiguo al más reciente en el anillo.
  uint8_t start = (count_ < MAX_ENTRIES) ? 0 : head_;
  for (uint8_t i = 0; i < count_ && n < max; i++) {
    out[n++] = entries_[(start + i) % MAX_ENTRIES];
  }
}

String Logger::toJson() const {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();
  uint8_t start = (count_ < MAX_ENTRIES) ? 0 : head_;
  for (uint8_t i = 0; i < count_; i++) {
    const Entry& e = entries_[(start + i) % MAX_ENTRIES];
    JsonObject o = arr.createNestedObject();
    o["t"] = e.timestamp;
    o["level"] = levelString(e.level);
    o["module"] = e.module;
    o["msg"] = e.message;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
