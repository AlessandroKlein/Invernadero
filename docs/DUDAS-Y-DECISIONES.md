# Pendientes de implementación

> **Estado:** En desarrollo | **Fecha:** 2026-09-28
>
> Documento que consolida **lo que falta implementar**, resultado de corroborar el
> `README.md` (286 secciones) y las decisiones de arquitectura. Lo ya implementado se
> lista únicamente como referencia de estado.

---

## 1. Estado actual (ya implementado)

| Bloque | Rama | Estado |
|--------|------|--------|
| Firmware de campo v3.1.0 (README §1–200) | `main` | ✅ sensores, actuadores, control, config, red, OTA, RS485, identidad/capacidades, máquina de estados, simulación |
| Servidor central (PHP + PostgreSQL + MQTT + dashboard) | `server` | ✅ API REST + JWT, worker MQTT→DB, dashboard |
| RBAC del servidor | `server` | ✅ 8 roles, permisos granulares, scope, login, dashboard de administración de usuarios, exposición (Cloudflare/nginx) |

---

## 2. Por implementar — Firmware V8 (plataforma configurable, README §201–285)

- **Configuration Engine** + **Hardware Manager** + **Bus Manager** (SPI/I²C/UART/RS485/1-Wire).
- **Sensor Registry / Actuator Registry** + catálogo ampliable de sensores/actuadores.
- **Rule Engine** configurable + **Safety Engine** + variables calculadas (VPD, punto de rocío, …).
- **Almacenamiento**: LittleFS + SD (por SPI) y estructura de archivos `/greenhouse/`.
- **Red**: Ethernet **W5500** configurable + administrador de buses SPI (CS únicos).
- **Expansión configurable**: 74HC165 (entradas), MCP23017/MCP23S17, **ADC Manager** (MCP3008/3208/ADS8688/ADS8332).
- **Import/export avanzado**: clonado, plantillas y **migraciones** (`schema_version`) — el export/import básico ya está (`/api/v1/config/export|import`).
- **Multi-board**: ESP32-S2/S3/C3/C5/C6 + particiones dinámicas + `#if CONFIG_IDF_TARGET_*`.
- **OTA**: manifest por plataforma (targets) + validación SHA-256 + health check/rollback.
- **Web local**: autenticación local + hardware avanzado protegido (ADMIN + PIN/botón) — escaneo WiFi y AP por MAC ya implementados.

---

## 3. Por implementar — Firmware V9 (industrial, README §112–119, 244–246)

- **Modbus**: perfiles genéricos + descubrimiento/scan + **commissioning** (UID propio, Vendor/Product ID).
- **Sensores industriales**: pH/EC/ORP/etc. vía RS485/Modbus con abstracción `SensorDriver → SensorProfile → SensorInstance → Measurement`.
- **Estación meteorológica externa** (`WeatherManager`: REST/MQTT/Modbus).
- **Gateway multi-bus** (Ethernet + RS485 + CAN/TWAI).
- **PCNT**: migrar a `PulseCounterManager` (caudal/lluvia/anemómetro).

---

## 4. Por implementar — Servidor central

- **Retención de históricos**: particionado por mes + agregación (5 min / 1 h / día) + columnas `source`/`sequence`/`status`.
- **Device Shadow**: añadir campo `actual` (hoy `desired`/`reported`).
- **`greenhouses.elevation`** (hoy solo latitude/longitude/timezone).
- **MFA/TOTP**: tablas `user_mfa`/`sessions`/`refresh_tokens` (arquitectura preparada).
- **Auditoría (UI)** + **backups** automatizados + **notificaciones**.
- **OTA centralizado** por grupos + canales (stable/beta/dev) + rollback.
- **Discovery/provisioning MQTT** (`greenhouse/discovery/#` + estados DISCOVERED → ACTIVE/BLOCKED).

---

## 5. Principios que se mantienen (invariantes)

1. El dispositivo funciona sin servidor.
2. El servidor nunca es necesario para una función automática crítica.
3. La automatización se ejecuta localmente.
4. Las comunicaciones son una capa independiente de la lógica de control.
5. El servidor administra y coordina; el ESP32 controla y protege.
