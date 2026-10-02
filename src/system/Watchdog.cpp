#include "system/Watchdog.hpp"

namespace gh {

void Watchdog::begin(uint32_t timeoutSec) {
  esp_task_wdt_init(timeoutSec, true); // true = panic al expirar
  esp_task_wdt_add(NULL);              // vigilar la tarea actual (loop)
}

void Watchdog::subscribe() {
  // Añade la tarea que invoca este método al watchdog de tareas de ESP-IDF.
  // Cada tarea suscrita debe llamar feed() periódicamente (SEMA §132).
  esp_task_wdt_add(NULL);
}

void Watchdog::feed() {
  esp_task_wdt_reset();
}

} // namespace gh
