#include "storage/History.hpp"

#include <ArduinoJson.h>

namespace gh {

void History::begin(size_t capacity) {
  // Liberar el buffer previo (por si se llama más de una vez).
  if (ring_) { delete[] ring_; ring_ = nullptr; }
  capacity_ = capacity;
  ring_ = new LogEvent[capacity_];
  head_ = 0;
  size_ = 0;
}

void History::add(uint8_t severity, const char* msg) {
  if (!ring_ || capacity_ == 0) return;
  // Asignar timestamp y copiar el mensaje de forma segura.
  LogEvent& e = ring_[head_];
  e.timestamp = (uint32_t)time(nullptr);
  e.severity = severity;
  strncpy(e.message, msg, sizeof(e.message) - 1);
  e.message[sizeof(e.message) - 1] = '\0';
  head_ = (head_ + 1) % capacity_;
  if (size_ < capacity_) size_++;
}

size_t History::count() const { return size_; }

LogEvent History::get(size_t i) const {
  // El evento más antiguo está en (head_ - size_) % capacity_.
  if (i >= size_) return LogEvent();
  size_t idx = (head_ + capacity_ - size_ + i) % capacity_;
  return ring_[idx];
}

void History::clear() { size_ = 0; }

String History::toJson(size_t max) const {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();
  size_t n = size_ < max ? size_ : max;
  // Recorrer desde el más reciente hacia atrás.
  for (size_t i = 0; i < n; i++) {
    LogEvent e = get(size_ - 1 - i);
    JsonObject o = arr.createNestedObject();
    o["t"] = e.timestamp;
    o["sev"] = e.severity;
    o["msg"] = e.message;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
