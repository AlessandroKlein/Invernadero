# Changelog

Todas las modificaciones notables de este proyecto se documentan en este archivo.
El formato se basa en [Keep a Changelog](https://keepachangelog.com/es/1.1.0/)
y el proyecto sigue [Versionado Semántico](https://semver.org/lang/es/).

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
