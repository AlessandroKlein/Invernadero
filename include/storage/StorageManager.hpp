#pragma once
// Abstracción de almacenamiento (V8.4, README §212-216). Monta LittleFS o SPIFFS
// sobre la partición `spiffs` y reserva SD (backend futuro). Proporciona E/S de
// archivos, listado y estado; base para HistoryManager e import/export.

#include <Arduino.h>
#include <FS.h>

#include "core/PlatformTypes.hpp"

namespace gh {

class StorageManager {
public:
  // Monta el backend (LittleFS primero; SPIFFS como fallback). formatOnFail
  // formatea la partición si el montaje falla.
  bool begin(bool formatOnFail = true);
  bool mounted() const { return mounted_; }
  StorageBackend backend() const { return backend_; }

  bool writeFile(const char* path, const char* data);
  bool appendFile(const char* path, const char* data);
  String readFile(const char* path);
  bool exists(const char* path);
  bool remove(const char* path);
  void listDir(const char* path, String* out, size_t max, size_t& n);

  uint32_t usedBytes() const;
  uint32_t totalBytes() const;
  String toJson() const;

private:
  fs::FS* fs_ = nullptr;
  StorageBackend backend_ = StorageBackend::NONE;
  bool mounted_ = false;
};

} // namespace gh
