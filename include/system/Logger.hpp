#pragma once
// Registro estructurado (SEMA §218-220). Buffer circular de entradas con
// timestamp, nivel, módulo y mensaje; nivel mínimo configurable y salida a Serial
// para niveles >= INFO. Base para observabilidad y logs remotos (MQTT/HTTP).

#include <Arduino.h>

namespace gh {

enum class LogLevel : uint8_t {
  TRACE = 0, DEBUG = 1, INFO = 2, NOTICE = 3,
  WARNING = 4, ERROR = 5, CRITICAL = 6
};

class Logger {
public:
  static constexpr uint8_t MAX_ENTRIES = 64;

  struct Entry {
    uint32_t timestamp = 0;
    uint8_t level = 0;
    char module[16] = "";
    char message[64] = "";
  };

  void begin(LogLevel minLevel = LogLevel::INFO);
  void log(LogLevel lvl, const char* module, const char* msg);

  void trace(const char* m, const char* msg) { log(LogLevel::TRACE, m, msg); }
  void debug(const char* m, const char* msg) { log(LogLevel::DEBUG, m, msg); }
  void info(const char* m, const char* msg) { log(LogLevel::INFO, m, msg); }
  void warn(const char* m, const char* msg) { log(LogLevel::WARNING, m, msg); }
  void error(const char* m, const char* msg) { log(LogLevel::ERROR, m, msg); }
  void critical(const char* m, const char* msg) { log(LogLevel::CRITICAL, m, msg); }

  void snapshot(Entry* out, size_t max, size_t& n) const;
  String toJson() const;
  uint8_t count() const { return count_; }

private:
  Entry entries_[MAX_ENTRIES];
  uint8_t count_ = 0;
  uint8_t head_ = 0;               // posición de escritura (anillo)
  LogLevel minLevel_ = LogLevel::INFO;
  const char* levelString(uint8_t lvl) const;
};

} // namespace gh
