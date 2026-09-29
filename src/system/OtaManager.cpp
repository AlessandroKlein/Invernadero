#include "system/OtaManager.hpp"
#include "system/Device.hpp"

#include <esp_ota_ops.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <mbedtls/md.h>

namespace gh {

void OtaManager::begin(const char* hostname) {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.onStart([]() {
    Device::setState(DeviceState::UPDATING); // sección 175
    String type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
    Serial.printf("[OTA] Inicio: %s\n", type.c_str());
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("[OTA] Fin");
    // Confirmar el nuevo firmware para cancelar el rollback automático del
    // bootloader (sección 146). Si la app no arranca correctamente, el
    // bootloader restaurará la partición anterior.
    esp_ota_mark_app_valid_cancel_rollback();
    Device::setState(DeviceState::RUN); // sección 175
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error %u\n", error);
  });
  ArduinoOTA.begin();
}

bool OtaManager::applyFromUrl(const String& url, const String& expectedSha) {
  if (updating_) return false;
  updating_ = true;

  String u = url;
  bool https = u.startsWith("https://");

  HTTPClient http;
  if (https) {
    // Sin validación estricta de certificado en esta versión (secciones 152/153).
    static WiFiClientSecure client;
    client.setInsecure();
    http.begin(client, u);
  } else {
    http.begin(u);
  }
  http.setTimeout(15000);

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[OTA] HTTP %d\n", code);
    http.end();
    updating_ = false;
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Serial.println("[OTA] tamaño inválido");
    http.end();
    updating_ = false;
    return false;
  }

  if (!Update.begin(contentLength, U_FLASH)) {
    Serial.println("[OTA] Update.begin error");
    http.end();
    updating_ = false;
    return false;
  }
  Device::setState(DeviceState::UPDATING);

  // SHA-256 opcional (cuando el servidor envía el checksum del firmware).
  bool doHash = expectedSha.length() == 64;
  mbedtls_md_context_t ctx;
  if (doHash) {
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&ctx);
  }

  WiFiClient* stream = http.getStreamPtr();
  uint8_t buf[8192];
  int remaining = contentLength;
  bool ok = true;

  while (remaining > 0) {
    size_t avail = stream->available();
    if (avail == 0) {
      if (!stream->connected()) { ok = false; break; }
      delay(1);
      continue;
    }
    size_t n = avail > sizeof(buf) ? sizeof(buf) : avail;
    if ((size_t)remaining < n) n = remaining;
    int got = stream->read(buf, n);
    if (got <= 0) { ok = false; break; }
    if (doHash) mbedtls_md_update(&ctx, buf, got);
    if (Update.write(buf, got) != (size_t)got) { ok = false; break; }
    remaining -= got;
  }
  http.end();

  uint8_t digest[32];
  if (doHash) {
    mbedtls_md_finish(&ctx, digest);
    mbedtls_md_free(&ctx);
  }

  // Verificar SHA-256 antes de confirmar la instalación.
  if (ok && remaining == 0 && doHash) {
    String expected = expectedSha;
    expected.toUpperCase();
    String got;
    got.reserve(64);
    for (int i = 0; i < 32; i++) {
      char h[3];
      snprintf(h, sizeof(h), "%02X", digest[i]);
      got += h;
    }
    if (got != expected) {
      Serial.println("[OTA] SHA-256 no coincide");
      ok = false;
    }
  }

  if (ok && remaining == 0 && Update.end()) {
    Serial.println("[OTA] instalado, reiniciando...");
    // Confirmar para cancelar el rollback automático del bootloader (sección 146).
    // Si el nuevo firmware no arranca, el bootloader restaura la partición anterior.
    esp_ota_mark_app_valid_cancel_rollback();
    Device::setState(DeviceState::UPDATING);
    delay(200);
    ESP.restart();
    return true;
  }

  Update.abort();
  Device::setState(DeviceState::RUN);
  updating_ = false;
  return false;
}

} // namespace gh
