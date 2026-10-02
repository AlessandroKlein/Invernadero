#pragma once
// Bus interno de eventos (SEMA §204-205). Permite que módulos independientes
// reaccionen a eventos (lluvia, alarma, red, configuración) sin acoplarse entre
// sí. Implementado con una cola FreeRTOS thread-safe.

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace gh {

enum class EventType : uint8_t {
  NONE = 0,
  RAIN_START = 1,
  ALARM = 2,
  SENSOR_ERROR = 3,
  NETWORK_UP = 4,
  NETWORK_DOWN = 5,
  CONFIG_CHANGED = 6,
  SYSTEM_BOOT = 7,
  LOW_BATTERY = 8
};

inline const char* eventTypeString(EventType t) {
  switch (t) {
    case EventType::RAIN_START:    return "RAIN_START";
    case EventType::ALARM:         return "ALARM";
    case EventType::SENSOR_ERROR:  return "SENSOR_ERROR";
    case EventType::NETWORK_UP:    return "NETWORK_UP";
    case EventType::NETWORK_DOWN:  return "NETWORK_DOWN";
    case EventType::CONFIG_CHANGED:return "CONFIG_CHANGED";
    case EventType::SYSTEM_BOOT:   return "SYSTEM_BOOT";
    case EventType::LOW_BATTERY:   return "LOW_BATTERY";
    default:                       return "NONE";
  }
}

struct Event {
  EventType type = EventType::NONE;
  uint32_t timestamp = 0;
  float value = 0.0f;
  char message[32] = "";
};

class EventBus {
public:
  // Crea la cola. Devuelve false si ya existía o no se pudo crear.
  bool begin(uint8_t queueDepth = 16);

  // Publica un evento (no bloqueante). False si la cola está llena.
  bool publish(const Event& e);
  // Publica un evento simple (sin payload).
  bool publish(EventType type, const char* message = "");

  // Consume un evento (timeoutMs=0 → no bloqueante).
  bool poll(Event& e, uint32_t timeoutMs = 0);

  uint8_t pending() const { return queue_ ? (uint8_t)uxQueueMessagesWaiting(queue_) : 0; }

private:
  QueueHandle_t queue_ = nullptr;
};

} // namespace gh
