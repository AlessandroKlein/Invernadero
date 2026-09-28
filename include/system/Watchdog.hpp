#pragma once
// Watchdog de tarea (sección 58). Reinicia el sistema si una tarea se bloquea.

#include <Arduino.h>
#include <esp_task_wdt.h>

namespace gh {

class Watchdog {
public:
  void begin(uint32_t timeoutSec = 30); // Añade la tarea actual al watchdog
  void feed();                          // Alimenta el watchdog (reset del timer)
};

} // namespace gh
