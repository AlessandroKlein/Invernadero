# Dudas, decisiones y correcciones del proyecto

> Fecha: 2026-09-28 · Para revisar y completar por el autor.

Este documento reúne (1) la **corroboración** del `README.md` (qué está
implementado y qué falta), (2) las **decisiones de arquitectura** tomadas para el
**servidor central** y (3) las **dudas/correcciones conceptuales** detectadas en la
idea principal para que sean resueltas.

---

## 1. Corroboración del README (286 secciones)

### 1.1 Implementado en el firmware (v3.1.0)

- **Sensores**: SHT31/AHT20 (§7), DS18B20 (§8), humedad de suelo + ADS1115 (§9-10),
  BH1750 (§11), CO₂ SCD4x (§13), nivel de tanque + flotadores (§14), caudal (§15),
  lluvia (§16), viento (§69), pH analógico + Modbus (§18), EC (§19).
- **Actuadores/expansión**: 74HC595 con Soft-PWM (§20-21), MCP23017 (§23),
  entradas de seguridad (§25), control techo/ventanas (§26).
- **Control**: histéresis de clima (§31), humedad (§32), suelo (§33), secuencia de
  riego + protección de bomba (§34-36), iluminación (§29-30), techo (§26, 68, 70).
- **Configuración**: por web (§37-40), JSON (§82), valores por defecto (§84),
  factory reset (§85), config versionada + rollback (§104), niveles de reset (§155).
- **Red**: WiFi/AP/mDNS/NTP (§86, 105-108), MQTT (§45), REST (§41-43), WebSocket (§44).
- **Sistema**: watchdog (§58), OTA + rollback (§59, 146), identidad/capacidades
  (§50, 166), máquina de estados (§175), calidad de datos (§176), causa de reinicio
  (§150), RS485 stats + scan + herramienta Modbus (§177-178), simulación (§191).

### 1.2 Pendiente (el propio README lo etapa como V8/V9/V10)

| Área | Secciones | Estado |
|------|-----------|--------|
| Motor de configuración tipo Tasmota (bus/hardware manager, GPIO/SPI/I²C configurables, registros de sensores/actuadores, rule engine) | 201-236, 280-286 | ❌ Pendiente (V8) |
| Ethernet W5500 y administrador de buses SPI | 87, 109-110, 247-249 | ❌ Pendiente |
| Almacenamiento LittleFS / SD | 212-216 | ❌ Pendiente (parcial: NVS + RAM) |
| Entradas 74HC165 | 227-228 | ❌ Pendiente |
| ADCs MCP3008/3208/ADS8688/ADS8332 | 250-252 | ❌ Pendiente (solo ADS1115) |
| Motor de reglas configurable | 121, 237 | ❌ Pendiente |
| Estación meteorológica externa (REST/MQTT/Modbus) | 244-246 | ❌ Pendiente |
| Device Shadow / provisioning / commissioning | 116-117, 140, 169, 271, 279 | ❌ Pendiente |
| Certificados / TLS mutuo | 153 | ❌ Pendiente |
| **Servidor central (PHP + PostgreSQL + MQTT + dashboard)** | 47-49, 130-145, 269-273 | ✅ **En esta entrega** |

---

## 2. Decisiones del servidor central (implementadas en `server/`)

1. **Stack**: PHP 8.2 + PostgreSQL 16 + Mosquitto (MQTT) + dashboard HTML/CSS/JS,
   orquestado con Docker Compose. Se eligió PHP por pedido explícito.
2. **API REST** versionada (`/api/v1/...`) con front controller `public/index.php`.
3. **Autenticación**: JWT (Bearer token) vía `firebase/php-jwt`.
4. **Bridge MQTT → PostgreSQL**: worker PHP CLI (`mqtt/worker.php`) con
   `php-mqtt/client` que se suscribe a `greenhouse/+/#` y persiste.
5. **Tiempo real en el dashboard**: MQTT over WebSocket (Mosquitto en puerto 9001)
   con `mqtt.js` (CDN), con `polling` a la API como fallback.
6. **Device Shadow**: tabla `device_shadow` con `desired` / `reported` (JSONB).
7. **Telemetría**: tabla `sensor_readings` con índice `(sensor_id, timestamp)` y
   retención configurable (ver dudas abajo).

---

## 3. Dudas y decisiones abiertas (para que completes)

1. **Retención de históricos** (§160): ¿retener crudo 30 días y luego agregar a
   5 min/1 h? ¿Usar particionado de PostgreSQL o un job de borrado? *(Propuesto:
   tabla particionada por mes + job de agregación.)*
2. **Métricas de VPD / variables calculadas** (§235): ¿calcular VPD en el ESP32 o
   en el servidor? *(Propuesto: servidor, a partir de T y RH ya almacenadas.)*
3. **`sunrise`/`sunset`** (§239): requiere latitud/longitud. ¿Agregar campos
   `latitude`/`longitude` a `greenhouses`? *(Propuesto: sí.)*
4. **Estación meteorológica** (§244-246): ¿fuente concreta (Open-Meteo, estación
   Davis/Modbus, MQTT propio)? Definir esquema de tópicos y autenticación.
5. **AP con SSID derivado de MAC** (§253-254): hoy el AP es fijo
   (`Invernadero-AP`/`invernadero`). ¿Cambiar a `INVERNADERO-XXXXXX` + clave derivada
   del UID? *(Propuesto: sí, en la próxima versión de firmware.)*
6. **Escaneo WiFi y `esp_wifi_scan`** (§107): hoy no está expuesto en la API local.
   ¿Es prioritario? *(Propuesto: media.)*
7. **Autenticación de la web local del ESP32** (§154): hoy es abierta. ¿Agregar
   usuario/clave local? *(Propuesto: sí, rol único admin por defecto.)*
8. **TLS en el ESP32** (§153): requiere CA + certificado por dispositivo. ¿Necesario
   para v1 o solo en LAN? *(Propuesto: LAN primero, TLS después.)*
9. **MQTT central vs local**: el ESP32 publica en `greenhouse/<id>/...`; el servidor
   debe conocer el `device_id` de antemano o registrarlo al primer mensaje.
   *(Propuesto: auto-registro por UID al recibir `/state`.)*
10. **Cifrado de contraseñas en el servidor**: se usa `password_hash()` (bcrypt).
    ¿Necesitás OAuth/2FA? *(Propuesto: no en v1.)*

---

## 4. Correcciones conceptuales detectadas (investigación)

1. **Soft-PWM sobre 74HC595 (§20-21)**: el 74HC595 no genera PWM real; es un
   desplazamiento de bits refrescado por software. Solo sirve para conmutación de
   potencia de baja frecuencia (100 Hz–1 kHz). Para PWM real usar LEDC (hardware)
   o un driver PWM dedicado (TLC5947). El README ya lo aclara; se reafirma.
2. **"Compartibles con N8 o N16" (§6)**: N8 = Flash 8 MB, N16 = 16 MB. El
   ESP32-WROOM-32 estándar trae 4 MB. Ya se proveen tablas de partición
   `default_8MB.csv` / `default_16MB.csv`. Con 4 MB el firmware actual va al ~73 %.
3. **Sensor interno de temperatura (§206)**: el ESP32 clásico **no** expone un
   sensor interno utilizable por Arduino; sí lo tienen ESP32-S3/C3/C5/C6. El README
   lo indica correctamente; debe tratarse como diagnóstico, no medición ambiental.
4. **TWAI/CAN (§88)**: el ESP32 tiene el periférico TWAI, pero **requiere un
   transceptor externo** (ej. SN65HVD230). No está implementado.
5. **Contador de pulsos (PCNT)**: el ESP32 dispone de PCNT por hardware; el firmware
   actual usa interrupciones + `millis()`. Para caudal/lluvia/viento a alta frecuencia
   convendría migrar a PCNT.
6. **Modbus RTU no tiene autodescubrimiento universal (§115-116, 278)**: correcto;
   se resuelve con UID propio + commissioning. A implementar.
7. **ADC de pH/EC (§18)**: el ADC interno del ESP32 es ruidoso y no lineal en los
   extremos; usar ADS1115 es correcto. Calibración pH 4/7/10 almacenada en NVS.
8. **Lux ≠ PPFD (§12)**: correcto; no asumir equivalencia.
9. **VPD** requiere T y RH; fórmula de Tetens/Magnus para presión de vapor.
