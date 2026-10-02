#include "storage/StorageManager.hpp"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <SPIFFS.h>

namespace gh {

bool StorageManager::begin(bool formatOnFail) {
  if (LittleFS.begin(formatOnFail)) {
    fs_ = &LittleFS;
    backend_ = StorageBackend::LITTLEFS;
    mounted_ = true;
    return true;
  }
  if (SPIFFS.begin(formatOnFail)) {
    fs_ = &SPIFFS;
    backend_ = StorageBackend::SPIFFS;
    mounted_ = true;
    return true;
  }
  return false;
}

bool StorageManager::writeFile(const char* path, const char* data) {
  if (!mounted_ || !path || !data) return false;
  File f = fs_->open(path, "w");
  if (!f) return false;
  size_t n = f.print(data);
  f.close();
  return n > 0 || strlen(data) == 0;
}

bool StorageManager::appendFile(const char* path, const char* data) {
  if (!mounted_ || !path || !data) return false;
  File f = fs_->open(path, "a");
  if (!f) return false;
  size_t n = f.print(data);
  f.close();
  return n > 0;
}

String StorageManager::readFile(const char* path) {
  if (!mounted_ || !path) return "";
  File f = fs_->open(path, "r");
  if (!f) return "";
  String s = f.readString();
  f.close();
  return s;
}

bool StorageManager::exists(const char* path) {
  return mounted_ && path && fs_->exists(path);
}

bool StorageManager::remove(const char* path) {
  return mounted_ && path && fs_->remove(path);
}

void StorageManager::listDir(const char* path, String* out, size_t max, size_t& n) {
  n = 0;
  if (!mounted_ || !path) return;
  File root = fs_->open(path);
  if (!root || !root.isDirectory()) return;
  File f = root.openNextFile();
  while (f && n < max) {
    if (out) out[n++] = f.name();
    f = root.openNextFile();
  }
}

uint32_t StorageManager::usedBytes() const {
  if (!mounted_) return 0;
  return (backend_ == StorageBackend::LITTLEFS)
             ? (uint32_t)LittleFS.usedBytes() : (uint32_t)SPIFFS.usedBytes();
}

uint32_t StorageManager::totalBytes() const {
  if (!mounted_) return 0;
  return (backend_ == StorageBackend::LITTLEFS)
             ? (uint32_t)LittleFS.totalBytes() : (uint32_t)SPIFFS.totalBytes();
}

String StorageManager::toJson() const {
  DynamicJsonDocument doc(256);
  doc["mounted"] = mounted_;
  doc["backend"] = storageBackendString(backend_);
  doc["used_bytes"] = usedBytes();
  doc["total_bytes"] = totalBytes();
  String out;
  serializeJson(doc, out);
  return out;
}

} // namespace gh
