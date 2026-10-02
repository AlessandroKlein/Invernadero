# Changelog

Todas las modificaciones notables de este proyecto se documentan en este archivo.
El formato se basa en [Keep a Changelog](https://keepachangelog.com/es/1.1.0/)
y el proyecto sigue [Versionado Semántico](https://semver.org/lang/es/).

## [3.12.0] - 2026-10-02

### Added
- Multi-board en `platformio.ini`: entornos para ESP32 / S2 / S3 / C3 / C6 (con
  `default_envs`) y guardas `CONFIG_IDF_TARGET_*` para TWAI (CAN solo en
  ESP32/S2/S3).
- Página web `/pins` (HTML, solo accesible desde la IP del dispositivo, no API)
  con el mapa de pines y direcciones I²C.
- Pines del bus SPI nativo (`SPI_SCK/MISO/MOSI`) y nota de compatibilidad de
  strapping en `PinMap`.

### Changed
- `firmware_manifest.json` actualizado a v3.12.0 con SHA-256 real.

### Removed
- `.clinerules` (eliminado del repositorio).

## [3.11.0] - 2026-10-02

### Added
- `Logger` estructurado (`system/Logger`): buffer circular con nivel/módulo/mensaje
  y salida a Serial; expuesto en `GET /api/v1/logs`.
- `EventBus` (`core/EventBus`): bus interno de eventos con cola FreeRTOS
  (`SYSTEM_BOOT`, `ALARM`, `RAIN_START`, `NETWORK_UP/DOWN`, ...).
- `Scheduler` (`core/Scheduler`): tareas periódicas por capacidades; la
  publicación MQTT ahora es una tarea programada (cada 10 s).
- `StorageManager::beginSD()`: backend SD por SPI (librería `SD`).
- Tipos de configuración por capas (`ConfigLayer`: FACTORY→USER) en
  `PlatformTypes`.
- `docs/ESTANDAR-DOCUMENTACION.md`: estándar reutilizable de wiki/documentación
  (estructura, conexiones con componentes, registro de cambios, ADR, publicación).

### Changed
- La publicación MQTT periódica se mueve del `loop()` al `Scheduler`.

## [3.10.0] - 2026-10-02

### Added
- División de tareas FreeRTOS: `SensorTask` (adquisición) + `ControlTask`
  (seguridad/control/salidas) con semáforo, accesores de `SensorManager`
  protegidos por mutex (MEJORAS §2-3).
- V8.1: `SpiManager` (bus SPI compartido), `ShiftRegister165` (74HC165),
  `Mcp23s17` (expansor SPI) y `AdcManager` (MCP3008/MCP3208 por SPI).
- V8.4: `StorageManager` (LittleFS/SPIFFS) con E/S de archivos y estado,
  expuesto en `GET /api/v1/storage`.
- V9: `ModbusProfileRegistry` (perfiles + instancias + provisioning),
  expuesto en `GET /api/v1/modbus/profiles`; tipos `ModbusProfile`,
  `SensorInstance`, `ProvisioningState`, `ModbusDataType`, `AdcKind`.
- V9: capa de aplicación CAN en `CanManager` (registro de nodos por CAN ID,
  actividad y estado).
- Frontend: tabla de datos responsive (scroll + tarjetas apiladas en móvil) y
  `docs/frontend-preview.html` actualizado con token de API y salud.

### Changed
- `SensorManager` expone lectura protegida de slots (`valueAt`).

## [3.9.0] - 2026-10-02

### Added
- Partición de 8 MB por defecto (margen de flash para la plataforma V8/V9).
- Exposición de los registros V8 por REST: `/api/v1/modules`, `/api/v1/buses`,
  `/api/v1/hardware`, `/api/v1/sensors/catalog` y `/api/v1/actuators/catalog`.
- Token de API generable desde la web de configuración (`ConfigManager` +
  endpoints `/api/v1/token/{status,rotate,revoke}`) para autorizar control y
  cambios desde el servidor central; `requireAuth` acepta el token como Bearer.
- Health monitor por tareas (`system/HealthMonitor`) con heartbeat, stack
  high-water-mark y heap, expuesto en `GET /api/v1/health`.
- Contadores de reinicio en NVS (`system/BootCounters`), expuestos en
  `GET /api/v1/boot`.
- Suscripción de la tarea de automatización al watchdog de tareas (ESP-IDF).
- Abstracción CAN/TWAI (`hardware/CanManager`, transceptor SN65HVD23X externo).
- `docs/MEJORAS.md` con el roadmap de mejoras derivado de SEMA.

### Changed
- El `Watchdog` ahora permite suscribir tareas adicionales (`subscribe()`).
- Documentación de implementación y estructura de módulos actualizada.

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
