#include "system/OtaManager.hpp"
#include "system/Device.hpp"

#include <esp_ota_ops.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <mbedtls/md.h>

namespace gh {

// --- Helpers estáticos ---

// Lee una línea HTTP terminada en \n (descarta el \r previo).
static String readHttpLine(Client* c) {
  String line;
  uint32_t start = millis();
  while (millis() - start < 8000) {
    if (c->available()) {
      char ch = (char)c->read();
      if (ch == '\n') return line;
      if (ch != '\r') line += ch;
    } else {
      delay(1);
    }
  }
  return line;
}

// Parsea "http://host[:port]/path".
static bool parseUrl(const String& url, String& host, uint16_t& port, String& path) {
  String u = url;
  if (u.startsWith("http://")) u = u.substring(7);
  int slash = u.indexOf('/');
  if (slash < 0) { host = u; path = "/"; } else { host = u.substring(0, slash); path = u.substring(slash); }
  int colon = host.indexOf(':');
  if (colon > 0) { port = (uint16_t)host.substring(colon + 1).toInt(); host = host.substring(0, colon); }
  else port = 80;
  return host.length() > 0;
}

// Alimenta Update + SHA desde un Client* (Content-Length o chunked).
static bool streamToUpdate(Client* c, int contentLength, bool chunked,
                           bool doHash, mbedtls_md_context_t* ctx) {
  bool ok = true;
  uint8_t buf[1024];

  if (chunked) {
    for (;;) {
      String sizeLine = readHttpLine(c);
      sizeLine.trim();
      long chunkSize = strtol(sizeLine.c_str(), nullptr, 16);
      if (chunkSize <= 0) { readHttpLine(c); break; }  // 0 = fin
      long remaining = chunkSize;
      while (remaining > 0) {
        if (!c->available()) { if (!c->connected()) { ok = false; goto done; } delay(1); continue; }
        size_t n = c->available();
        if (n > sizeof(buf)) n = sizeof(buf);
        if ((long)n > remaining) n = (size_t)remaining;
        int got = c->read(buf, n);
        if (got <= 0) { ok = false; goto done; }
        if (doHash) mbedtls_md_update(ctx, buf, got);
        if (Update.write(buf, got) != (size_t)got) { ok = false; goto done; }
        remaining -= got;
      }
      readHttpLine(c);  // CRLF tras el chunk
    }
  } else {
    long remaining = contentLength;
    while (remaining > 0) {
      if (!c->available()) { if (!c->connected()) { ok = false; goto done; } delay(1); continue; }
      size_t n = c->available();
      if (n > sizeof(buf)) n = sizeof(buf);
      if ((long)n > remaining) n = (size_t)remaining;
      int got = c->read(buf, n);
      if (got <= 0) { ok = false; goto done; }
      if (doHash) mbedtls_md_update(ctx, buf, got);
      if (Update.write(buf, got) != (size_t)got) { ok = false; goto done; }
      remaining -= got;
    }
    if (remaining != 0) ok = false;
  }
done:
  return ok;
}

// GET HTTP manual sobre un Client* genérico (WiFi o Ethernet/W5500).
static bool httpDownload(Client* c, const String& host, uint16_t port,
                         const String& path, bool doHash, mbedtls_md_context_t* ctx) {
  if (!c->connect(host.c_str(), port)) return false;
  c->print("GET " + path + " HTTP/1.1\r\n");
  c->print("Host: " + host + "\r\n");
  c->print("User-Agent: Invernadero-OTA\r\n");
  c->print("Connection: close\r\n\r\n");

  String status = readHttpLine(c);
  int code = status.substring(9, 12).toInt();
  if (code != 200) { c->stop(); return false; }

  int contentLength = -1;
  bool chunked = false;
  for (;;) {
    String line = readHttpLine(c);
    line.trim();
    if (line.length() == 0) break;
    String lower = line;
    lower.toLowerCase();
    if (lower.startsWith("content-length:")) contentLength = lower.substring(15).toInt();
    else if (lower.startsWith("transfer-encoding:") && lower.indexOf("chunked") >= 0) chunked = true;
  }

  if (contentLength <= 0 && !chunked) { c->stop(); return false; }

  if (!Update.begin(chunked ? 0 : (size_t)contentLength, U_FLASH)) { c->stop(); return false; }
  Device::setState(DeviceState::UPDATING);

  bool ok = streamToUpdate(c, contentLength, chunked, doHash, ctx);
  c->stop();
  return ok;
}

// --- OtaManager ---

void OtaManager::begin(const char* hostname) {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.onStart([]() {
    Device::setState(DeviceState::UPDATING);
    String type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
    Serial.printf("[OTA] Inicio: %s\n", type.c_str());
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("[OTA] Fin");
    esp_ota_mark_app_valid_cancel_rollback();
    Device::setState(DeviceState::RUN);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error %u\n", error);
  });
  ArduinoOTA.begin();
}

bool OtaManager::applyFromUrl(const String& url, const String& expectedSha, Client* netClient) {
  if (updating_) return false;
  updating_ = true;

  String u = url;
  bool https = u.startsWith("https://");
  bool doHash = expectedSha.length() == 64;

  mbedtls_md_context_t ctx;
  if (doHash) {
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&ctx);
  }

  bool ok = false;

  if (https) {
    // TLS: HTTPClient + WiFiClientSecure (solo WiFi).
    static WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, u);
    http.setTimeout(15000);
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
      Serial.printf("[OTA] HTTP %d\n", code);
    } else {
      int contentLength = http.getSize();
      if (contentLength <= 0) {
        Serial.println("[OTA] tamaño inválido");
      } else if (!Update.begin((size_t)contentLength, U_FLASH)) {
        Serial.println("[OTA] Update.begin error");
      } else {
        Device::setState(DeviceState::UPDATING);
        WiFiClient* stream = http.getStreamPtr();
        uint8_t buf[8192];
        int remaining = contentLength;
        ok = true;
        while (remaining > 0) {
          size_t avail = stream->available();
          if (avail == 0) { if (!stream->connected()) { ok = false; break; } delay(1); continue; }
          size_t n = avail > sizeof(buf) ? sizeof(buf) : avail;
          if ((size_t)remaining < n) n = remaining;
          int got = stream->read(buf, n);
          if (got <= 0) { ok = false; break; }
          if (doHash) mbedtls_md_update(&ctx, buf, got);
          if (Update.write(buf, got) != (size_t)got) { ok = false; break; }
          remaining -= got;
        }
        if (remaining != 0) ok = false;
      }
    }
    http.end();
  } else {
    // HTTP genérico: GET manual sobre el cliente activo (WiFi o Ethernet).
    String host, path;
    uint16_t port;
    if (!netClient || !parseUrl(u, host, port, path)) {
      Serial.println("[OTA] cliente de red no disponible o URL inválida");
    } else {
      ok = httpDownload(netClient, host, port, path, doHash, &ctx);
    }
  }

  // Verificar SHA-256 y finalizar la instalación.
  uint8_t digest[32];
  if (doHash) {
    mbedtls_md_finish(&ctx, digest);
    mbedtls_md_free(&ctx);
  }
  if (ok && doHash) {
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

  if (ok && Update.end()) {
    Serial.println("[OTA] instalado, reiniciando...");
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
