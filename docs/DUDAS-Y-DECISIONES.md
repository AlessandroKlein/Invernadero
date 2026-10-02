# Dudas, decisiones y pendientes de implementación

> **Estado:** En desarrollo
> **Fecha:** 2026-09-29
> **Propósito:** consolidar las decisiones arquitectónicas que deben respetarse durante la implementación de V8, V9 y del servidor central.

---

# 1. Principios invariantes

Estas reglas no deben modificarse sin una decisión arquitectónica explícita:

1. El ESP32 funciona sin servidor.
2. El servidor nunca es necesario para una función automática crítica.
3. La automatización se ejecuta localmente.
4. Las comunicaciones son independientes de la lógica de control.
5. El servidor administra, registra, visualiza y coordina.
6. El ESP32 mide, decide, controla y protege.
7. Una pérdida de comunicación no debe detener una automatización crítica.
8. Las funcionalidades opcionales deben poder habilitarse/deshabilitarse.
9. Las páginas deben componerse mediante módulos y bloques.
10. El Core no debe depender de módulos opcionales.

---

# 2. Históricos

## Decisión

Utilizar PostgreSQL con particionado mensual.

### Retención

| Datos           |                                 Retención |
| --------------- | ----------------------------------------: |
| Crudos          |                                   30 días |
| Agregado 5 min  |                                     1 año |
| Agregado 1 h    |                                    5 años |
| Agregado diario | Configurable / preferentemente permanente |

### Campos adicionales

```text
source
sequence
status
quality
```

### Proceso

```text
Raw
 ↓
5 min
 ↓
1 h
 ↓
Daily
 ↓
retención
```

Nunca eliminar datos crudos antes de comprobar que fueron agregados correctamente.

---

# 3. Variables calculadas

## Decisión

Las variables calculadas importantes deben poder ejecutarse localmente.

Crear:

```text
CalculatedVariableEngine
```

Variables iniciales:

```text
VPD
DewPoint
AbsoluteHumidity
HeatIndex
ET0
```

Si una variable participa de una automatización crítica:

```text
ESP32 → cálculo local
```

El servidor también puede recalcularla para:

```text
históricos
gráficos
reportes
validación
analytics
```

---

# 4. Ubicación

La entidad `greenhouses` debe contener:

```text
latitude
longitude
elevation
timezone
```

La ubicación debe utilizarse para:

```text
sunrise
sunset
solar calculations
weather
ET0
```

`sunrise` y `sunset` no deben almacenarse como valores permanentes.

Deben calcularse según fecha y ubicación.

---

# 5. WeatherManager

Crear:

```text
WeatherManager
```

con adaptadores:

```text
OpenMeteoAdapter
RESTWeatherAdapter
MQTTWeatherAdapter
ModbusWeatherAdapter
```

Separar:

```text
FORECAST
OBSERVATION
```

Open-Meteo será el proveedor inicial para pronósticos.

Una estación física se tratará como una fuente de observaciones independiente.

---

# 6. MQTT Weather

Formato estándar:

```text
greenhouse/{greenhouse_id}/weather/{source}/state
greenhouse/{greenhouse_id}/weather/{source}/telemetry
greenhouse/{greenhouse_id}/weather/{source}/status
```

El payload debe contener timestamp, variables medidas y estado/calidad cuando corresponda.

---

# 7. WiFi AP

El SSID debe ser único:

```text
INVERNADERO-XXXXXX
```

El identificador se obtiene del dispositivo.

No utilizar la MAC como base de una contraseña determinista.

La contraseña del AP debe ser:

```text
aleatoria
única
persistente
```

Debe existir un mecanismo seguro de commissioning/recovery.

---

# 8. WiFi Scan

El escaneo WiFi ya se considera implementado.

No es prioridad de V8.

Debe permanecer dentro de:

```text
NetworkManager
```

y estar disponible para:

```text
WiFi configuration
Commissioning
Diagnostics
```

---

# 9. Web local

La web local debe requerir autenticación.

Inicialmente:

```text
ADMIN
```

Posteriormente:

```text
ADMIN
OPERATOR
VIEWER
```

La configuración avanzada de hardware debe requerir permisos elevados y, para operaciones críticas, confirmación adicional mediante PIN, botón físico u otro mecanismo equivalente.

---

# 10. TLS

La arquitectura debe soportar TLS desde V8.

### V8

Implementar:

```text
TLS abstraction
CA validation
HTTPS client
MQTT TLS
OTA HTTPS
```

### V9

Implementar:

```text
mTLS
device certificates
certificate rotation
```

No bloquear V8 esperando el sistema completo de PKI.

---

# 11. MQTT Provisioning

No aceptar automáticamente cualquier dispositivo que publique por primera vez.

Estados:

```text
DISCOVERED
PENDING
COMMISSIONED
ACTIVE
BLOCKED
REVOKED
```

Flujo:

```text
ESP32
 ↓
Discovery
 ↓
Server
 ↓
PENDING
 ↓
Commissioning
 ↓
ACTIVE
```

El auto-commissioning debe ser opcional y explícito.

---

# 12. Device Identity

Separar:

```text
device_id
device_uid
hardware_id
mac_address
```

El `device_id` debe ser estable.

La MAC es información de hardware, no la identidad lógica completa.

---

# 13. Device Shadow

El Shadow tendrá:

```text
desired
reported
actual
```

### Desired

Estado solicitado.

### Reported

Estado/configuración informado por el dispositivo.

### Actual

Estado físico efectivo.

Ejemplo:

```json
{
  "desired": {
    "pump": true
  },
  "reported": {
    "pump": true
  },
  "actual": {
    "pump": false
  }
}
```

Esto permite detectar discrepancias físicas.

---

# 14. Autenticación del servidor

Mantener:

```text
password_hash()
```

No almacenar contraseñas reversibles.

JWT continúa siendo el mecanismo inicial de autenticación de la API.

---

# 15. MFA

MFA/TOTP queda aprobado para una fase posterior del servidor.

Preparar:

```text
user_mfa
sessions
refresh_tokens
```

Las acciones críticas podrán requerir MFA/reautenticación.

---

# 16. PCNT

Crear:

```text
PulseCounterManager
```

Utilizarlo para:

```text
Flow
Rain
Wind
Pulse sensors
```

Arquitectura:

```text
PulseCounterManager
├── FlowMeter
├── RainGauge
└── Anemometer
```

Utilizar el driver PCNT moderno de ESP-IDF cuando corresponda al target.

---

# 17. Modbus

No existe autodiscovery universal.

Utilizar:

```text
Scan
 ↓
Profile detection
 ↓
Vendor/Product ID
 ↓
Commissioning
 ↓
SensorProfile
 ↓
SensorInstance
```

Arquitectura:

```text
SensorDriver
 ↓
SensorProfile
 ↓
SensorInstance
 ↓
Measurement
```

---

# 18. CAN/TWAI

Preparar:

```text
BusManager
```

para:

```text
I2C
SPI
UART
RS485
CAN/TWAI
```

CAN/TWAI no será requisito de V8.

---

# 19. 74HC595

El 74HC595 se considera:

```text
shift register
```

No debe documentarse como generador PWM hardware.

Para PWM utilizar:

```text
LEDC
MCPWM
PWM driver
```

según necesidad.

---

# 20. Configuration Engine

V8 debe implementar:

```text
ConfigurationManager
HardwareManager
BusManager
ModuleRegistry
CapabilityRegistry
SensorRegistry
ActuatorRegistry
```

---

# 21. Module System

El sistema debe ser modular.

Cada módulo debe declarar:

```text
id
version
dependencies
capabilities
permissions
configuration
```

Estados:

```text
NOT_INSTALLED
INSTALLED
ENABLED
DISABLED
ERROR
```

Debe ser posible:

```text
install
configure
enable
disable
update
uninstall
```

---

# 22. Page / Block System

Las páginas no deben ser monolíticas.

Deben componerse mediante:

```text
Page
 ↓
Slots
 ↓
Blocks
 ↓
Modules
```

Ejemplo:

```text
Dashboard
├── header
├── main
│   ├── Temperature
│   ├── Humidity
│   ├── Soil
│   ├── Irrigation
│   └── Alarm
└── sidebar
```

Los bloques deben poder instalarse posteriormente.

---

# 23. Capability System

El frontend, servidor y firmware deben utilizar capacidades.

Ejemplo:

```text
wifi
ethernet
sd
modbus
can
temperature
humidity
co2
```

La interfaz no debe mostrar funcionalidades que el dispositivo no soporta.

---

# 24. V8 — orden de implementación

```text
1. Configuration Engine
2. HardwareManager
3. BusManager
4. ModuleRegistry
5. CapabilityRegistry
6. SensorRegistry
7. ActuatorRegistry
8. StorageManager
9. SPI Manager
10. I2C Manager
11. 74HC165
12. MCP23017
13. MCP23S17
14. ADC Manager
15. W5500
16. TLS abstraction
17. RuleEngine
18. SafetyEngine
19. CalculatedVariableEngine
20. LittleFS
21. SD Manager
22. Import/export
23. schema_version
24. migrations
25. OTA multi-target
26. health check
27. rollback
```

---

# 25. V9 — industrial

```text
Modbus
Industrial Sensors
WeatherManager
PCNT
Gateway
CAN/TWAI
Commissioning
Provisioning
Device Shadow
```

---

# 26. Servidor central

Pendientes:

```text
Historical retention
Device Shadow actual
greenhouses.elevation
MFA/TOTP
Audit UI
Backups
Notifications
Central OTA
Discovery/provisioning
```

---

# 27. Regla de seguridad

Nunca convertir el servidor en dependencia de:

```text
irrigation
lighting
climate
safety
emergency
```

El servidor puede solicitar.

El ESP32 decide y ejecuta según sus reglas locales de seguridad.

---

# 28. Regla final

> **El sistema debe poder crecer agregando módulos, bloques, sensores, actuadores, buses y funcionalidades sin convertir el Core en un monolito.**

> **Una nueva funcionalidad debe poder instalarse, registrarse, configurarse, habilitarse, deshabilitarse y actualizarse con el menor impacto posible sobre el resto del sistema.**
