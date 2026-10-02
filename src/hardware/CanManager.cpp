#include "hardware/CanManager.hpp"

#include <ArduinoJson.h>
#include <driver/twai.h>

namespace gh {

// Mapea un bitrate a la configuración de temporización estándar de ESP-IDF.
static twai_timing_config_t timingFor(uint32_t bitrate) {
  switch (bitrate) {
    case 1000000: return TWAI_TIMING_CONFIG_1MBITS();
    case 800000:  return TWAI_TIMING_CONFIG_800KBITS();
    case 500000:  return TWAI_TIMING_CONFIG_500KBITS();
    case 250000:  return TWAI_TIMING_CONFIG_250KBITS();
    case 125000:  return TWAI_TIMING_CONFIG_125KBITS();
    case 100000:  return TWAI_TIMING_CONFIG_100KBITS();
    case 50000:   return TWAI_TIMING_CONFIG_50KBITS();
    case 25000:   return TWAI_TIMING_CONFIG_25KBITS();
    default:      return TWAI_TIMING_CONFIG_500KBITS();
  }
}

bool CanManager::begin(int txPin, int rxPin, uint32_t bitrate) {
  if (installed_) return false;
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)txPin, (gpio_num_t)rxPin, TWAI_MODE_NORMAL);
  twai_timing_config_t t = timingFor(bitrate);
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) != ESP_OK) return false;
  if (twai_start() != ESP_OK) {
    twai_driver_uninstall();
    return false;
  }
  txPin_ = txPin;
  rxPin_ = rxPin;
  bitrate_ = bitrate;
  installed_ = true;
  return true;
}

void CanManager::end() {
  if (!installed_) return;
  twai_stop();
  twai_driver_uninstall();
  installed_ = false;
}

bool CanManager::send(uint32_t id, const uint8_t* data, uint8_t len, bool extended) {
  if (!installed_) return false;
  twai_message_t msg = {};
  msg.identifier = id;
  msg.data_length_code = (len > 8) ? 8 : len;
  if (data) memcpy(msg.data, data, msg.data_length_code);
  msg.extd = extended ? 1 : 0;
  if (twai_transmit(&msg, pdMS_TO_TICKS(10)) != ESP_OK) {
    errorCount_++;
    return false;
  }
  txCount_++;
  return true;
}

bool CanManager::receive(uint32_t& id, uint8_t* data, uint8_t& len, uint32_t timeoutMs) {
  if (!installed_) return false;
  twai_message_t msg;
  if (twai_receive(&msg, timeoutMs ? pdMS_TO_TICKS(timeoutMs) : 0) != ESP_OK) return false;
  id = msg.identifier;
  len = msg.data_length_code;
  if (data) memcpy(data, msg.data, len);
  rxCount_++;
  touchNode(id);  // capa de aplicación: registrar actividad del nodo emisor
  return true;
}

bool CanManager::registerNode(uint32_t id, const char* name) {
  int8_t idx = findNode(id);
  if (idx < 0) {
    if (nodeCount_ >= MAX_NODES) return false;
    idx = nodeCount_++;
    nodes_[idx].id = id;
  }
  strncpy(nodes_[idx].name, name ? name : "", sizeof(nodes_[idx].name) - 1);
  nodes_[idx].name[sizeof(nodes_[idx].name) - 1] = '\0';
  nodes_[idx].lastSeenMs = 0;
  return true;
}

void CanManager::touchNode(uint32_t id) {
  int8_t idx = findNode(id);
  if (idx >= 0) nodes_[idx].lastSeenMs = millis();
}

bool CanManager::nodeAlive(uint32_t id) const {
  int8_t idx = findNode(id);
  if (idx < 0) return false;
  return nodes_[idx].lastSeenMs != 0 && (millis() - nodes_[idx].lastSeenMs) < 5000UL;
}

String CanManager::toJson() const {
  DynamicJsonDocument doc(1024);
  doc["installed"] = installed_;
  doc["bitrate"] = bitrate_;
  doc["tx_pin"] = txPin_;
  doc["rx_pin"] = rxPin_;
  doc["tx_count"] = (uint32_t)txCount_;
  doc["rx_count"] = (uint32_t)rxCount_;
  doc["errors"] = (uint32_t)errorCount_;
  JsonArray nodes = doc.createNestedArray("nodes");
  for (uint8_t i = 0; i < nodeCount_; i++) {
    JsonObject o = nodes.createNestedObject();
    o["id"] = nodes_[i].id;
    o["name"] = nodes_[i].name;
    o["alive"] = nodeAlive(nodes_[i].id);
    o["last_seen_ms"] = (uint32_t)nodes_[i].lastSeenMs;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

int8_t CanManager::findNode(uint32_t id) const {
  for (uint8_t i = 0; i < nodeCount_; i++) {
    if (nodes_[i].id == id) return i;
  }
  return -1;
}

} // namespace gh
