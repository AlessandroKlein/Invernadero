#include "system/Watchdog.hpp"

namespace gh {

void Watchdog::begin(uint32_t timeoutSec) {
  esp_task_wdt_init(timeoutSec, true); // true = panic al expirar
  esp_task_wdt_add(NULL);              // vigilar la tarea actual (loop)
}

void Watchdog::feed() {
  esp_task_wdt_reset();
}

} // namespace gh
