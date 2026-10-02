#include "core/EventBus.hpp"

namespace gh {

bool EventBus::begin(uint8_t queueDepth) {
  if (queue_ != nullptr) return true;
  queue_ = xQueueCreate(queueDepth, sizeof(Event));
  return queue_ != nullptr;
}

bool EventBus::publish(const Event& e) {
  if (queue_ == nullptr) return false;
  return xQueueSend(queue_, &e, 0) == pdTRUE;  // no bloqueante
}

bool EventBus::publish(EventType type, const char* message) {
  Event e;
  e.type = type;
  e.timestamp = millis();
  if (message) {
    strncpy(e.message, message, sizeof(e.message) - 1);
    e.message[sizeof(e.message) - 1] = '\0';
  }
  return publish(e);
}

bool EventBus::poll(Event& e, uint32_t timeoutMs) {
  if (queue_ == nullptr) return false;
  return xQueueReceive(queue_, &e, timeoutMs ? pdMS_TO_TICKS(timeoutMs) : 0) == pdTRUE;
}

} // namespace gh
