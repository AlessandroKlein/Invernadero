# Implementación — Plataforma modular de invernadero (ESP32)

Implementación del `README.md` como firmware ESP32 (Arduino / PlatformIO).

## Construcción y flasheo

```bash
pio run                 # compila (firmware.bin en .pio/build/<env>/)
pio run -t upload       # graba por puerto serie
pio device monitor      # monitor serie (115200)
```

Requisitos: PlatformIO Core (probado con espressif32 / Arduino core 3.x).

## Estructura de módulos

```
src/
  main.cpp                 # cableado + tarea de automatización (FreeRTOS)
include/ y src/
  config/                  # ConfigManager (JSON en NVS) + Defaults
  storage/                 # History (buffer circular de eventos/alarmas)
  hardware/                # ShiftRegister595 (soft-PWM), Mcp23017 (I²C), ModbusRtu
  sensors/                 # SHT31/AHT20, DS18B20, ADS1115, BH1750, SCD4x(CO₂),
                           # caudal, tanque, lluvia, viento, pH, EC, SensorManager
  actuators/               # ActuatorManager (bomba, válvulas, ventiladores, ...)
  control/                 # Climate, Irrigation, Lighting, Roof, Safety
  network/                 # NetworkManager (WiFi/AP/mDNS/NTP), MqttManager
  api/                     # RestApi (REST), WebSocketServer (/ws puerto 81)
  system/                  # Watchdog, OtaManager, Diagnostics
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

## Trabajo futuro (fuera del alcance de esta entrega)

- Servidor central (PostgreSQL + dashboard multinvernadero + usuarios/permisos).
- UI web completa de configuración por zonas/sensores/actuadores.
- Variante SPI MCP23S17, Ethernet y bus CAN/TWAI distribuido.
