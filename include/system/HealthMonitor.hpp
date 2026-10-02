#pragma once
// Monitor de salud por tareas (inspirado en SEMA §131-134/158-159). Cada tarea
// crítica registra actividad (heartbeat); el monitor expone si responde, el
// high-water-mark de su pila y métricas de heap para detectar bloqueos.

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace gh {

class HealthMonitor {
public:
  static constexpr uint8_t MAX_TASKS = 6;

  void begin();
  // Registra una tarea con su handle (para leer el high-water-mark de pila).
  void registerTask(const char* name, TaskHandle_t handle);
  // Heartbeat que la propia tarea invoca en cada ciclo.
  void touch(const char* name);
  String toJson() const;

private:
  struct Entry {
    char name[20] = "";
    TaskHandle_t handle = nullptr;
    volatile uint32_t lastActivityMs = 0;
  };
  Entry tasks_[MAX_TASKS];
  uint8_t count_ = 0;
  int8_t find(const char* name) const;
};

} // namespace gh
