# Implementación — Plataforma modular de invernadero (ESP32)

Implementación del `README.md` como firmware ESP32 (Arduino / PlatformIO).

## Construcción y flasheo

```bash
pio run                 # compila (firmware.bin en .pio/build/<env>/)
pio run -t upload       # graba por puerto serie
pio device monitor      # monitor serie (115200)
```

Requisitos: PlatformIO Core (probado con espressif32 / Arduino core 3.x).
Nota: si el `pio` del PATH intenta descargar `tool-scons`, usar el core
coincidente con el paquete cacheado (ej. `~/.platformio/penv/Scripts/pio.exe`).

## Particiones de memoria

Tabla de particiones explícita en `partitions/` (doble app OTA + SPIFFS + coredump):

| Archivo | Flash | app0/app1 | SPIFFS |
|---------|-------|-----------|--------|
| `default.csv` | 4 MB | 1,25 MB c/u | 1,375 MB |
| `default_8MB.csv` | 8 MB | 3,19 MB c/u | 1,5 MB |
| `default_16MB.csv` | 16 MB | 6,25 MB c/u | 3,375 MB |

Selección en `platformio.ini` vía `board_build.partitions`. Con 4 MB el firmware
actual ocupa ~73 % de la partición de app; para más margen usar N8/N16.

## Estructura de módulos

```
src/
  main.cpp                 # cableado + tarea de automatización (FreeRTOS)
include/ y src/
  core/                    # Types, PinMap, Version (FW/HW/esquema centralizados)
  config/                  # ConfigManager (JSON en NVS, versionado + rollback) + Defaults
  storage/                 # History (buffer circular de eventos/alarmas)
  hardware/                # ShiftRegister595 (soft-PWM), Mcp23017 (I²C), ModbusRtu (scan+stats)
  sensors/                 # SHT31/AHT20, DS18B20, ADS1115, BH1750, SCD4x(CO₂),
                           # caudal, tanque, lluvia, viento, pH, EC, SensorManager
  actuators/               # ActuatorManager (bomba, válvulas, ventiladores, ...)
  control/                 # Climate, Irrigation, Lighting, Roof, Safety
  network/                 # NetworkManager (WiFi/AP/mDNS/NTP), MqttManager
  api/                     # RestApi (REST), WebSocketServer (/ws puerto 81)
  system/                  # Watchdog, OtaManager, Diagnostics, Device
  web/                     # WebAssets (interfaz embebida servida en /)
```

## Pines por defecto

Definidos en `include/core/PinMap.hpp` (I²C, SPI para 74HC595, 1-Wire, pulsos,
ultrasónico, flotadores, parada de emergencia y RS485). Modificables sin tocar la
lógica.

## Alcance implementado

- Configuración no volátil (JSON en NVS), editable por API (`PUT /api/v1/config`).
- Sensores económicos (SHT31/AHT20, DS18B20, capacitivo, BH1750, ADS1115) y
  avanzados (CO₂ SCD4x, pH/EC por ADS1115 o Modbus RTU/RS485).
- Expansión de salidas por 74HC595 (daisy-chain) y MCP23017 (I²C). Soft-PWM por
  temporizador (deshabilitado por defecto; para PWM de alta frecuencia usar LEDC
  en GPIO o un controlador dedicado tipo TLC5947).
- Control por histéresis (clima), máquina de estados de riego con protección de
  bomba, iluminación por horario/luz, techo por temperatura/lluvia/viento y
  sistema de seguridad con jerarquía (emergencia > seguridad > manual > auto).
- Red: WiFi STA + AP de configuración, mDNS, NTP, MQTT.
- API REST (`/api/v1/...`), WebSocket en tiempo real, OTA con rollback (doble
  partición), watchdog e interfaz web embebida.

## Evolución de la plataforma (secciones 101–200)

- **Identidad y capacidades**: UID permanente (derivado de MAC), perfil de
  hardware, versiones de firmware/hardware/esquema/protocolo y lista de
  capacidades (`WIFI`, `I2C`, `SPI`, `ONEWIRE`, `RS485`, `MODBUS`, `PWM`, ...).
- **Máquina de estados**: BOOTING → INITIALIZING → SELF_TEST → NETWORK → RUN
  (y DEGRADED/ERROR/UPDATING/RECOVERY para OTA y fallos).
- **Causa de reinicio**: POWER_ON / SOFTWARE_RESET / WATCHDOG / BROWNOUT / PANIC.
- **Calidad de datos** por lectura: GOOD / WARNING / INVALID / TIMEOUT /
  OUT_OF_RANGE / CALIBRATION / DISCONNECTED.
- **Configuración versionada**: `config_version`, `configuration_source`
  (LOCAL/CENTRAL) y rollback (CONFIG ACTUAL / CONFIG ANTERIOR).
- **Niveles de reset**: RESET NETWORK / RESET AUTOMATION / FACTORY RESET.
- **RS485/Modbus generalizado**: estadísticas (TX/RX/CRC/timeouts), escaneo de
  bus (auto descubrimiento) y herramienta de lectura de registros.
- **Modo simulación**: genera datos sintéticos sin hardware para pruebas.
- **OTA con manifest**: `firmware_manifest.json` (versión, canal, SHA-256) y
  confirmación de rollback (doble partición).

### Endpoints REST ampliados

```
GET  /api/v1/device           GET  /api/v1/capabilities
GET  /api/v1/config/schema    GET  /api/v1/network
GET  /api/v1/rs485            POST /api/v1/rs485/scan
GET  /api/v1/modbus           GET  /api/v1/firmware
GET  /api/v1/ota              GET  /api/v1/zones
GET  /api/v1/diagnostics      POST /api/v1/reset
POST /api/v1/config/rollback
```

## Documentación (wiki)

La documentación completa está publicada en el **wiki** del repositorio:
https://github.com/AlessandroKlein/Invernadero/wiki

Páginas: Arquitectura, Hardware y pines, Materiales, Sensores, Actuadores y
salidas, Configuración, Variables modificables (JSON), API REST, MQTT/WebSocket,
Identidad y estados, OTA y actualización, y Compilación y flasheo.

## Trabajo futuro (fuera del alcance de esta entrega)

- Servidor central (PostgreSQL + dashboard multinvernadero + usuarios/permisos +
  Device Shadow + auditoría).
- UI web completa de configuración/asistente por zonas/sensores/actuadores.
- Variante SPI MCP23S17, Ethernet (interfaz de red intercambiable) y bus CAN/TWAI.
- Módulos I/O propios Modbus RTU y gateway multi-bus (Ethernet + RS485 + CAN).

