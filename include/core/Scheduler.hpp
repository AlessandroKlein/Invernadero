#pragma once
// Scheduler por capacidades (SEMA §206-208). Tabla de tareas periódicas simples
// que se ejecutan solo si están habilitadas; se tica desde loop() o una tarea.
// Permite intervalos configurables sin reservar tareas FreeRTOS permanentes.

#include <Arduino.h>

namespace gh {

class Scheduler {
public:
  static constexpr uint8_t MAX_TASKS = 8;
  using TaskFn = void (*)(void* ctx);

  struct Task {
    char name[16] = "";
    uint32_t intervalMs = 0;
    uint32_t lastRunMs = 0;
    bool enabled = true;
    TaskFn fn = nullptr;
    void* ctx = nullptr;
  };

  bool add(const char* name, uint32_t intervalMs, TaskFn fn, void* ctx = nullptr);
  bool remove(const char* name);
  bool setEnabled(const char* name, bool enabled);
  uint8_t count() const { return count_; }

  // Ejecuta las tareas vencidas. Debe llamarse periódicamente.
  void tick();

  String toJson() const;

private:
  Task tasks_[MAX_TASKS];
  uint8_t count_ = 0;
  int8_t find(const char* name) const;
};

} // namespace gh
