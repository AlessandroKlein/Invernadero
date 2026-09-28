# Decisiones de Arquitectura — v2.0

> **Tipo:** Documento de arquitectura | **Estado:** Cerrado | **Fecha:** 2026-09-28
>
> Sustituye a la versión 1.0 («Dudas y decisiones abiertas»). Las dudas pendientes
> quedan **resueltas** aquí como decisiones oficiales. Cualquier implementación
> posterior (firmware o servidor) debe respetar estas definiciones para no tener que
> rehacer la base de datos, la API, MQTT o el sistema de configuración.

---

## 1. Principio rector

> **El servidor administra y coordina. El ESP32 controla y protege.**
> El servidor puede desaparecer; el invernadero **no** debe dejar de funcionar.

Esta es una **invariante del sistema**, no una funcionalidad opcional (ver decisión 11).

---

## 2. Alcance y frontera de versiones

| Bloque | Estado |
|--------|--------|
| Firmware de campo v3.1.0 (secciones 1–200 del `README.md`) | ✅ Implementado |
| Servidor central (`server/`: PHP + PostgreSQL + MQTT + dashboard) | ✅ Implementado |
| Secciones 201–286 (config engine tipo Tasmota, W5500, SD, 74HC165, ADCs extra, rule engine, estación meteorológica, multi-board, provisioning) | ⏳ Roadmap V8/V9/V10 — **no implementado** |

---

## 3. Resumen de decisiones cerradas

| # | Decisión |
|---|----------|
| 1 | PostgreSQL particionado por mes + agregación progresiva + retención configurable |
| 2 | Variables calculadas: local (automatización) + servidor (históricos/análisis) |
| 3 | `latitude`/`longitude`/`elevation`/`timezone` por invernadero; sunrise/sunset local |
| 4 | `WeatherManager` con fuentes REST / MQTT / Modbus (no una única) |
| 5 | AP `INVERNADERO-XXXXXX` + contraseña de primera configuración almacenada (no derivada de MAC) |
| 6 | Escaneo WiFi en la web local (prioridad V8) |
| 7 | Autenticación local obligatoria + hardware avanzado protegido (ADMIN + PIN/botón) |
| 8 | HTTP local inicialmente; TLS obligatorio para comunicación remota de producción |
| 9 | Discovery automático sí; alta como dispositivo confiable solo tras aprobación/provisioning |
| 10 | `password_hash()` (bcrypt/Argon2) + RBAC + arquitectura preparada para TOTP; sin OAuth en V1 |
| 11 | Offline-first como invariante |
| 12 | Device Shadow `desired`/`reported`/`actual` |
| 13 | Configuración de hardware local-authoritative (solo lectura en servidor) |
| 14 | Import/export con separación clonable / no clonable |
| 15 | Configuración versionada con migraciones (`schema_version`) |
| 16 | Manifest OTA por plataforma (targets) + validación de compatibilidad |

---

## 4. Decisiones detalladas

### 4.1 Retención de históricos (§160)

**PostgreSQL particionado por mes + agregación progresiva + retención configurable por
instalación.** No se borran registros con un simple cron.

```text
sensor_readings_raw  →  crudo 0–30 días
        │
        ├── >30 días  →  agregación 5 min  (hasta 180 días)
        └── >180 días →  agregación 1 h    (hasta 2 años / 730 días)
agregación diaria → permanente (mientras haya espacio)
```

| Datos | Retención |
|-------|-----------|
| Crudos | 30 días |
| Agregados 5 min | 180 días |
| Agregados 1 h | 730 días (2 años) |
| Diarios | 0 (sin expiración) |

Configurable:

```json
{ "retention": { "raw_days": 30, "five_min_days": 180, "hour_days": 730, "daily_days": 0 } }
```

Cada medición conserva, como mínimo: `timestamp`, `device_id`, `sensor_id`, `value`,
`unit`, `quality`, `status`, `source`, `sequence`.

> **Impacto:** la tabla `sensor_readings` ya existe; se añadirán particionado por mes,
> jobs de agregación y las columnas `source` / `sequence` / `status`.

### 4.2 VPD y variables calculadas (§235)

**Cálculo local para automatización + cálculo central para históricos/análisis.**

- **ESP32:** calcula lo necesario para las reglas locales (VPD, punto de rocío,
  promedios). La automatización **no** puede depender del servidor.
- **Servidor:** recalcula para gráficos, estadísticas, comparación entre invernaderos,
  informes y análisis.

Concepto genérico `CalculatedVariable`: `VPD`, `DewPoint`, `TemperatureAverage`,
`HumidityAverage`, `SoilMoistureAverage`, `WaterConsumption`, `PhotoperiodProgress`;
posteriormente variables definidas por el usuario.

### 4.3 Sunrise / Sunset (§239)

**Sí a coordenadas por invernadero; el ESP32 calcula sunrise/sunset localmente.**

Modelo `greenhouse`: `latitude`, `longitude`, `timezone`, `elevation`.

```json
{ "latitude": -31.4488, "longitude": -60.9317, "elevation": 40,
  "timezone": "America/Argentina/Buenos_Aires" }
```

No depender de una API externa para conocer sunrise/sunset.

> **Impacto:** `greenhouses` ya tiene `latitude` / `longitude` / `timezone`;
> falta añadir `elevation`.

### 4.4 Estación meteorológica externa (§244-246)

**`WeatherManager` con tres fuentes (REST, MQTT, Modbus), no una única.**

- **API** (Open-Meteo u otra): pronóstico y datos exteriores. Opcional, nunca requisito.
- **MQTT** (estación propia): tópicos `weather/<station_id>/state`, `/status`,
  `/availability`.
- **Modbus RTU** (estación industrial): usa el mismo `ModbusManager` de los sensores
  industriales.

Las estaciones meteorológicas **no** se mezclan con `greenhouse/<device_id>/...`
(son dispositivos conceptualmente distintos).

Selección: `○ Ninguna · ○ API · ○ MQTT · ○ Modbus RTU · ○ Estación local`.

### 4.5 AP con SSID derivado de identidad (§253-254)

**SSID `INVERNADERO-XXXXXX` (últimos 3 bytes de MAC) + contraseña de primera
configuración almacenada en NVS. La contraseña NO se deriva de la MAC** (la MAC no es
un secreto). Luego el usuario puede definir una contraseña persistente.

Recuperación: botón físico / código de recuperación / factory reset / mantenimiento.

Modos: AP permanente, AP solo sin WiFi, o AP bajo demanda.

> **Impacto:** hoy el AP es fijo (`Invernadero-AP` / `invernadero`); se cambiará en V8.

### 4.6 Escaneo WiFi (§107)

**Implementar (prioridad V8/V8.1).** La web local muestra `SSID`, `RSSI`, `canal`,
`seguridad` y permite seleccionar red sin escribir el SSID. El escaneo **no** guarda
contraseñas de redes descubiertas.

### 4.7 Autenticación de la web local (§154)

**Autenticación local obligatoria + hardware avanzado protegido.**

Niveles: `VIEWER` / `OPERATOR` / `ADMIN` / `MAINTENANCE` (V1: `ADMIN`; luego roles).

Separación:
- Usuario normal: ver sensores/actuadores, cambiar modo, activar manualmente, ver alarmas.
- **No** puede: GPIO, SPI, I²C, dirección Modbus, hardware, particiones.

El hardware avanzado requiere `ADMIN + PIN` y/o `ADMIN + botón físico`.

> **Impacto:** hoy la web local es abierta; se añadirá autenticación.

### 4.8 TLS en ESP32 (§153)

**HTTP local inicialmente; TLS obligatorio para comunicación remota/producción.**

- Local (ESP32 ↔ navegador): HTTP en V1.
- Remoto (ESP32 ↔ servidor vía Internet): MQTT sobre TLS (`mqtts://:8883`).

No implementar infraestructura compleja de certificados sin provisioning. Futuro:
`device_uid`, `device_certificate`, `private_key`, `CA` con flujo
registro → autorización → certificado → conexión TLS. No enviar credenciales
permanentes en texto plano.

### 4.9 MQTT y registro automático (§140, 169)

**Discovery automático sí; alta como dispositivo confiable no.**

Estados: `DISCOVERED → REGISTERED → PROVISIONED → ACTIVE` (y `BLOCKED`).

Primer contacto: `greenhouse/discovery/<UID>` con `uid`, `mac`, `firmware`, `hardware`,
`protocol`. El administrador aprueba; recién entonces se asignan `device_id` y
`greenhouse_id`.

Separar identidades: `UID` físico (MAC) → `device_id` (`DEV-000123`) → `greenhouse_id`
(`GH-00007`).

> **Impacto:** el worker actual se suscribe a `greenhouse/+/#`; se añadirá
> `greenhouse/discovery/#` y la máquina de estados de aprovisionamiento.

### 4.10 Contraseñas / OAuth / 2FA (§134, 152)

- Contraseñas: `password_hash()` (bcrypt/Argon2). Nunca almacenar reversibles.
- OAuth: no en V1.
- MFA/TOTP: arquitectura preparada (`users`, `user_credentials`, `user_mfa`,
  `sessions`, `refresh_tokens`); TOTP posterior (Google Authenticator/Authy/FreeOTP).
- Roles: `SUPERADMIN`, `ADMIN`, `MANAGER`, `OPERATOR`, `VIEWER`, `MAINTENANCE` +
  permisos explícitos (`device.read/write/restart/ota`, `hardware.read/write`,
  `config.read/write`, `automation.read/write`, `user.manage`).

> **Impacto:** `roles` / `users` ya existen (bcrypt) y JWT; se ampliará RBAC.

---

## 5. Decisiones adicionales

### 5.11 Offline-first (invariante)

El servidor puede caer; el ESP32 sigue con reglas locales, sensores, actuadores,
seguridad y automatización; almacena eventos/históricos localmente y sincroniza al
volver. Es una **invariante**, no opcional.

### 5.12 Device Shadow

`desired` / `reported` / `actual`. Sincronización: `desired → validar → aplicar →
guardar → reported`. Si `desired != reported`, el dispositivo sincroniza.

> **Impacto:** `device_shadow` ya existe (`desired` / `reported`); se añadirá `actual`.

### 5.13 Configuración local vs servidor (autoridad)

- **ESP32 autoridad** para: GPIO, SPI, I²C, UART, RS485, 74HC595/165, MCP23017,
  sensores/actuadores físicos, hardware.
- **Servidor autoridad** para: usuarios, permisos, grupos, invernaderos, históricos,
  dashboard, reportes, firmware, inventario.
- Configuración operativa se sincroniza (`desired` → `reported`) pero el ESP32
  **valida antes de aplicar**.

La configuración de hardware es **solo lectura** en el servidor por defecto (requiere
`ADMIN + PIN + confirmación física` localmente).

### 5.14 Import / Export

`EXPORT → config.json` y `IMPORT → VALIDATE → MIGRATE → BACKUP → APPLY → RESTART`.

Separación:
- **Clonable:** sensores, actuadores, reglas, umbrales, zonas, programaciones, calibraciones.
- **No clonable:** `device_uid`, `MAC`, `device_id`, certificados, claves privadas,
  contraseñas WiFi, credenciales MQTT/servidor.

### 5.15 Configuración versionada

```json
{ "schema_version": 8, "config_id": "...", "created_at": "...", "device": {},
  "hardware": {}, "network": {}, "sensors": [], "actuators": [], "automation": {} }
```

Migraciones `schema 1 → … → schema N`. Nunca asumir estructura fija.

> **Impacto:** el firmware ya guarda `config_version`; se añadirá `schema_version`
> y migraciones explícitas.

### 5.16 Manifest OTA por plataforma

```json
{ "version": "8.2.0", "channel": "stable",
  "targets": { "esp32":   { "flash": "4MB", "url": "...", "sha256": "..." },
               "esp32s3": { "flash": "8MB", "psram": true, "url": "...", "sha256": "..." } } }
```

Además `min_bootloader`, `min_schema`, `min_protocol`, `hardware_revision` para evitar
firmware incompatible.

---

## 6. Correcciones conceptuales (cerradas)

1. **74HC595 y PWM:** no genera PWM real; es un registro de desplazamiento de salidas
   digitales. PWM real = ESP32 LEDC o driver dedicado (TLC5947). La modulación
   temporizada sobre el 595 es posible, pero su frecuencia/resolución dependen de
   cantidad de registros, SPI, frecuencia de actualización y canales. **No** afirmar
   «100 Hz–1 kHz» como límite absoluto ni «carga de CPU = 0 %».
2. **N8/N16:** es una característica de la **placa/flash**, no del SoC. Mismo código
   fuente, binarios/particiones distintos según Flash/PSRAM. Detectar
   SoC/Flash/PSRAM/revisión.
3. **Sensor interno de temperatura:** categoría `DIAGNOSTIC`, no `ENVIRONMENTAL`.
4. **TWAI/CAN:** requiere transceptor externo (ej. SN65HVD230). Bus opcional, no
   sustituto de RS485.
5. **PCNT:** migrar a `PulseCounterManager` con soporte `GPIO interrupt` + `PCNT` y
   selección por plataforma (caudal/lluvia/anemómetro).
6. **Modbus:** sin autodescubrimiento universal. Flujo `SCAN → PROFILE →
   IDENTIFICATION → COMMISSIONING`. Dispositivos propios exponen `UID`, `Vendor ID`,
   `Product ID`, `Firmware`, `Capabilities`.
7. **pH/EC:** abstraer con `SensorDriver → SensorProfile → SensorInstance →
   Measurement` (sin `if (sensor == PH) …`). Un transmisor nuevo se incorpora por perfil.
8. **VPD:** `calculated_value`, `temperature_source`, `humidity_source`, `formula`,
   `quality`. Si falta RH → `VPD = INVALID` (nunca `0`).
9. **Lux ≠ PPFD:** no asumir equivalencia.
10. **ADC de pH/EC:** usar ADS1115 (el ADC interno es ruidoso/no lineal); calibración
    4/7/10 en NVS.

---

## 7. Invariantes del sistema

1. El dispositivo funciona sin servidor.
2. El servidor nunca es necesario para una función automática crítica.
3. Toda configuración importante se almacena localmente.
4. El servidor mantiene copia de la configuración.
5. Las configuraciones tienen versión.
6. Las actualizaciones se pueden revertir.
7. Los sensores se agregan sin modificar la lógica principal.
8. RS485 es un bus industrial general (no solo pH).
9. Modbus usa perfiles configurables.
10. Identidad permanente independiente de la dirección Modbus.
11. Ethernet y WiFi son interfaces intercambiables.
12. La web local permite recuperar/mantener el dispositivo sin servidor.
13. El servidor administra múltiples dispositivos e instalaciones.
14. La automatización se ejecuta localmente.
15. Las comunicaciones son una capa independiente de la lógica de control.

---

## 8. Mapeo a la implementación actual

| Decisión | Ya existe | Pendiente |
|----------|-----------|-----------|
| 1 Retención | tabla `sensor_readings` | particionado, agregación, `source`/`sequence`/`status` |
| 3 Coordenadas | `latitude` / `longitude` / `timezone` | `elevation` |
| 9 Discovery | worker `greenhouse/+/#` | `discovery/#`, estados de aprovisionamiento |
| 10 Auth | `roles` + `users` (bcrypt), JWT | RBAC ampliado, `user_mfa`, `sessions` |
| 12 Shadow | `device_shadow` (`desired`/`reported`) | campo `actual` |
| 15 Versionado | `configurations` versionada + `config_version` | `schema_version` + migraciones |
| 16 OTA | `firmware_versions`, `ota_jobs` | manifest por targets (multi-SoC) |

