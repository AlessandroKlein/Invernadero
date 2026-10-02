# Changelog

Todas las modificaciones notables de este proyecto se documentan en este archivo.
El formato se basa en [Keep a Changelog](https://keepachangelog.com/es/1.1.0/)
y el proyecto sigue [Versionado Semántico](https://semver.org/lang/es/).

## [3.8.0] - 2026-10-02

### Added
- Base de la plataforma configurable (V8): `CapabilityRegistry`, `ModuleRegistry`,
  `BusManager` (I²C/SPI/UART/RS485/1-Wire/GPIO/CAN), `HardwareManager`,
  `SensorRegistry` y `ActuatorRegistry`.
- Tipos de plataforma en `core/PlatformTypes.hpp` (buses, módulos, nodos de
  hardware, catálogos de sensores/actuadores).
- Escaneo I²C desde `BusManager` y arranque del bus I²C centralizado en él.
- Registro en tiempo de ejecución de capacidades, módulos embebidos y catálogos
  derivados de `SystemConfig` (con resumen por Serial en el arranque).

### Changed
- La inicialización del bus I²C (`Wire.begin`) se centraliza en `BusManager`
  en lugar de hacerse directamente en `main.cpp`.
- Decisiones arquitectónicas consolidadas en `docs/DUDAS-Y-DECISIONES.md`.

## [3.7.0] - 2026-09-29

### Added
- OTA remota desde el servidor central: descarga del `.bin` por HTTP/HTTPS con verificación SHA-256 e instalación en la partición OTA inactiva (rollback automático).
- Procesamiento de comandos MQTT (`greenhouse/{id}/cmd`) para disparar la actualización.
- La configuración (NVS) y los datos (SPIFFS) se preservan entre actualizaciones.

## [3.6.0] - 2026-09-29

### Added
- Estación meteorológica externa: el ESP32 consulta un URL que publica JSON (HTTP/HTTPS).
- Mapeo configurable de campos (`key_temp`, `key_hum`, `key_wind`, `key_rain`, `key_pressure`, `key_light`) y subobjeto raíz opcional (`root`).
- Endpoint `GET /api/v1/weather` y publicación MQTT `greenhouse/{id}/weather`.

## [3.5.0] - 2026-09-28

### Added
- Tema claro/oscuro en la web local embebida (toggle + persistencia en `localStorage`).

## [3.4.0] - 2026-09-28

### Added
- Autenticación local de la web: login de administrador + token de sesión (expira 1 h).
- Endpoints de modificación (`PUT/POST/DELETE`) protegidos; lectura (`GET`) pública (§19/§154/§204).
- Contraseña de admin derivada del UID del dispositivo y persistida en NVS separado (no se exporta en el JSON).

## [3.3.0] - 2026-09-28

### Added
- Variables calculadas: VPD y punto de rocío (en `SensorManager` y `/api/v1/status`) (§235).
- Motor de reglas configurable (`control/RuleEngine`) con condiciones `variable op umbral → actuador` y API `GET/POST/DELETE /api/v1/automation` (§121/122/237).

## [3.2.0] - 2026-09-28

### Added
- Identificación del hardware (SoC/flash/PSRAM/MAC/temperatura) en `/api/v1/device` (§206).
- Escaneo WiFi (`POST /api/v1/network/scan`): SSID, RSSI, canal y seguridad (§107).
- Access Point con SSID identificable derivado de la MAC (`INVERNADERO-XXXXXX`) (§253).
- Export/import de configuración (`GET /api/v1/config/export`, `POST /api/v1/config/import`) con `schema_version` (§15/221/257-260).

## [3.1.1] - 2026-09-28

### Changed
- Documentación del proyecto (README ampliado a 286 secciones y pendientes en `docs/DUDAS-Y-DECISIONES.md`).
- Repositorio separado en ramas: `main` (firmware) y `server` (servidor central).

## [3.1.0] - 2026-09-28

### Added
- Identidad permanente del dispositivo (UID derivado de la MAC) y perfil de hardware.
- Máquina de estados del dispositivo (BOOTING/INITIALIZING/SELF_TEST/NETWORK/RUN/...).
- Registro de la causa del último reinicio (POWER_ON/SOFTWARE_RESET/WATCHDOG/BROWNOUT/PANIC/...).
- Modelo de calidad de datos por lectura (GOOD/WARNING/INVALID/TIMEOUT/OUT_OF_RANGE/CALIBRATION/DISCONNECTED).
- Versionado de configuración (`config_version`), fuente propietaria (LOCAL/CENTRAL) y rollback (CONFIG ACTUAL/ANTERIOR).
- Niveles de reset: RESET NETWORK / RESET AUTOMATION / FACTORY RESET.
- Abstracción de zonas y lista de zonas en configuración.
- Estadísticas RS485/Modbus (TX/RX/CRC/timeouts) y escaneo de bus (auto descubrimiento).
- Herramienta Modbus de mantenimiento.
- Modo simulación sin hardware.
- Manifest de firmware (`firmware_manifest.json`) y confirmación de rollback OTA (doble partición).
- Ampliación de la API REST: `/device`, `/capabilities`, `/config/schema`, `/network`, `/rs485`, `/rs485/scan`, `/modbus`, `/firmware`, `/ota`, `/zones`, `/diagnostics`, `/reset`, `/config/rollback`.

### Changed
- Serialización JSON de configuración ampliada a 8 KB con nuevos campos (invernadero, servidor central, RS485, zonas, zona horaria IANA, DNS).
- Versiones de firmware/hardware/esquema centralizadas en `core/Version.hpp`.

## [3.0.0] - 2026-09-28

### Added
- Plataforma modular de automatización de invernadero (base 100 % nueva).
- Firmware modular: configuración (NVS/JSON), sensores, actuadores, controladores,
  red (WiFi/AP/mDNS/NTP/MQTT), API REST, WebSocket, OTA y watchdog.
