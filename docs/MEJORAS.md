# Mejoras recomendadas

> **Tipo:** Roadmap técnico | **Estado:** En desarrollo | **Fecha:** 2026-10-02
>
> Documento que consolida las mejoras identificadas al contrastar el proyecto
> Invernadero con el README de [SEMA](https://github.com/AlessandroKlein/SEMA)
> y con las decisiones de `docs/DUDAS-Y-DECISIONES.md`. Marca qué ya está
> implementado y qué queda pendiente, con prioridad sugerida.

---

## 1. Resumen (v3.9.0 → v3.15.0)

| Mejora | Estado |
|--------|--------|
| Partición de 8 MB (margen de flash para V8/V9) | ✅ implementado |
| Exposición de registros V8 por REST | ✅ implementado |
| Token de API generable desde la web (control desde servidor central) | ✅ implementado |
| Health monitor por tareas (heartbeat + stack high-water-mark + heap) | ✅ implementado |
| Contadores de reinicio en NVS (boot/watchdog/brownout/panic/...) | ✅ implementado |
| Watchdog de tareas (suscripción de las tareas) | ✅ implementado |
| División SensorTask/ControlTask con semáforo + accesores protegidos | ✅ v3.10.0 |
| V8.1: `SpiManager`, `ShiftRegister165` (74HC165), `Mcp23s17`, `AdcManager` | ✅ v3.10.0 |
| V8.4: `StorageManager` (LittleFS/SPIFFS) | ✅ v3.10.0 |
| V9: `ModbusProfileRegistry` (perfiles + instancias + provisioning) | ✅ v3.10.0 |
| Abstracción CAN/TWAI + capa de aplicación (nodos) | ✅ v3.10.0 |
| `Logger` estructurado + `EventBus` + `Scheduler` por capacidades | ✅ v3.11.0 |
| SD (backend de `StorageManager`) | ✅ v3.11.0 |
| Tipos de configuración por capas (`ConfigLayer`) | ✅ v3.11.0 |
| Multi-board + página `/pins` | ✅ v3.12.0 |
| Interfaz WiFi/Ethernet intercambiable (W5500): conexión + MQTT | ✅ v3.13.0 |
| Configuración por capas: `schema_version` + migraciones + merge | ✅ v3.14.0 |
| Servidor: rate limiting + CORS + Store & Forward (`sequence_id`) | ✅ rama `server` |
| Autodetección guiada (escaneo I²C → `/api/v1/detect`) | ✅ v3.15.0 |
| Autodetección guiada (escaneo I²C → `/api/v1/detect`) | ✅ v3.15.0 |
| OTA HTTP sobre Ethernet (W5500) | ✅ v3.16.0 |
| Gateway RS485 (polling multi-esclavo por perfiles) | ✅ v3.17.0 |
| HTTPS sobre W5500 (requiere W5500lwIP/ETH nativo) | ❌ pendiente |

---

## 2. División de tareas FreeRTOS (SEMA §132-134, §202-208)

### Estado actual (v3.10.0)

```text
Core 0 — SensorTask (prio 3) → leer sensores (escribe slots protegidos)
         ControlTask (prio 2) → seguridad + controladores + salidas
Core 1 — loop()               → red, MQTT, API, WebSocket, OTA (comunicación)
```

La adquisición y el control ya están separados con un semáforo sensor→control;
los accesores de `SensorManager` se protegieron con mutex (`valueAt`), por lo que
no hay carrera de datos. La comunicación sigue en el núcleo 1 y nunca bloquea la
adquisición (principio de no bloqueo).

### División recomendada (refactor opcional futuro)

Para granularidad total al estilo SEMA se puede llegar a:

```text
SensorTask        (prioridad alta)   → leer sensores, publicar snapshot por cola
MeasurementTask   (prioridad media)  → validar, calcular variables (VPD, ...)
ControlTask       (prioridad alta)   → seguridad + controladores + salidas
StorageTask       (prioridad media)  → historial local (LittleFS/SD)
NetworkTask       (prioridad baja)   → WiFi/Ethernet/mDNS/NTP
CommunicationTask (prioridad baja)   → MQTT publish/subscribe
WebTask           (prioridad baja)   → API REST + WebSocket
```

La evolución natural es sustituir el semáforo por una cola (`xQueue`) de
`SensorValue` snapshots para desacoplar aún más las tareas (menos contención).

---

## 3. Health Monitor y observabilidad (SEMA §158-159, §216-219)

Ya implementado:

- `HealthMonitor` (`system/HealthMonitor`) con heartbeat por tarea,
  `stack high-water-mark` y `free_heap`/`min_free_heap`, expuesto en
  `GET /api/v1/health`.
- `BootCounters` (`system/BootCounters`) con contadores por causa de reinicio,
  expuesto en `GET /api/v1/boot`.

Pendiente recomendado:

- **Watchdog de tareas completo**: hoy solo `loop` y `automation` se suscriben;
  cuando se dividan las tareas, cada una debe `subscribe()` + `feed()`.
- **Contadores por componente** (RS485 TX/RX/CRC/timeouts, errores I²C/CAN) como
  métricas en `/api/v1/health`.
- **Log estructurado** con nivel configurable (`TRACE..CRITICAL`) y publicación
  remota (MQTT/HTTP), manteniendo almacenamiento local.
- **Health Monitor de hardware**: SDA/SCL stuck LOW, recuperación de bus I²C.

---

## 4. Token de API para el servidor central

### Implementado

- `ConfigManager::rotateApiToken()` genera un token aleatorio de 128 bits
  persistido en NVS (key separada, nunca se exporta en el JSON).
- `RestApi::requireAuth()` acepta `X-Auth-Token` o `Authorization: Bearer`, con
  prioridad: sesión de admin **o** token de API.
- Endpoints: `POST /api/v1/token/rotate` (devuelve el token una vez),
  `POST /api/v1/token/revoke`, `GET /api/v1/token/status` (sin revelar el token).
- La página de configuración permite generar/revocar el token.

### Recomendado

- En el servidor central, guardar el token **cifrado** (no en texto plano) y
  usarlo como `Bearer` para `PUT/POST/DELETE` contra el dispositivo.
- Añadir **scopes** mínimos (p. ej. `control`, `config`) cuando el servidor
  necesite separar permisos por token.
- Rotación automática programada y **rate limiting** sobre los endpoints
  autenticados (SEMA §167).

---

## 5. CAN/TWAI (SEMA §25)

### Implementado

`CanManager` (`hardware/CanManager`) abstrae el periférico TWAI de ESP-IDF:
`begin(tx, rx, bitrate)`, `send()`, `receive()`, métricas y JSON. Bitrates
soportados: 25k/50k/100k/125k/250k/500k/800k/1M.

### Pendiente

- Asignar pines según instalación (transceptor **SN65HVD230/231/232** externo).
- Filtros por CAN ID y prioridad (la capa de aplicación con nodos ya está:
  `registerNode`/`touchNode`/`nodeAlive`).
- No habilitar `CAN` como capability hasta que exista el transceptor.

---

## 6. RS485 / Modbus (SEMA §23-24)

Estado: RS485 + Modbus RTU básicos implementados (escaneo + lectura de
registros). `ModbusProfileRegistry` (v3.10.0) aporta los perfiles declarativos
(`ModbusProfile`: slave ID, registro, tipo de dato, escala, offset, unidad) y las
instancias (`SensorInstance`) con estado de provisioning. Pendiente:

- **Gateway RS485** multi-esclavo con polling por intervalos configurables.
- Integrar el registro de perfiles con el driver `ModbusRtu` en tiempo de ejecución.

---

## 7. Configuración por capas (SEMA §46, README §205)

Pendiente: evolucionar el `ConfigManager` plano hacia capas:

```text
FACTORY → HARDWARE → DRIVERS → INSTALLATION → AUTOMATION → USER
```

Esto permite separar "qué hardware hay" de "qué hace", y que un sensor cambie de
función sin tocar el driver. Implica `schema_version` + migraciones.

---

## 8. Otras mejoras de SEMA (estado)

- **Event Bus / Event Manager** (SEMA §204-205) → ✅ v3.11.0 (`core/EventBus`).
- **Scheduler por capacidades** (SEMA §206-208) → ✅ v3.11.0 (`core/Scheduler`).
- **Log estructurado** (SEMA §218-220) → ✅ v3.11.0 (`system/Logger`).
- **SD** → ✅ v3.11.0 (`StorageManager::beginSD()`).
- **API Keys + revocación** (SEMA §164-165) → ✅ (token de API v3.9.0).
- **Autodetección guiada** (SEMA §210) → ❌ pendiente.
- **Store & Forward** (SEMA §241-243) → ❌ pendiente (sincronización incremental
  con `sequence_id`, requiere servidor).
- **Rate limiting y CORS** (SEMA §166-167) → ❌ pendiente (lado servidor).

---

## 9. Prioridad sugerida

1. Gateway RS485 (polling multi-esclavo por perfiles).
2. HTTP/OTA sobre Ethernet (desbloquear §10).
3. Integración CAN de aplicación (tras definir hardware).

## 10. HTTP/OTA sobre Ethernet — resuelto (HTTP) / pendiente (HTTPS)

El `HTTPClient` del ESP32 solo acepta `WiFiClient`, pero `Update.h` es **agnóstico
al origen del stream**: recibe bytes por `Update.write()` desde cualquier `Client`
(WiFi o `EthernetClient` del W5500).

### Resuelto (v3.16.0)

- **OTA por HTTP sobre Ethernet (W5500)**: `OtaManager::applyFromUrl` hace un GET
  manual sobre el `Client*` activo y escribe el `.bin` en la partición OTA, con
  soporte de `Content-Length` y `Transfer-Encoding: chunked`.

### Pendiente

- **HTTPS sobre W5500**: la librería clásica `Ethernet` no tiene TLS. Requiere
  `W5500lwIP` (pila lwIP + mbedTLS) o ETH nativo (LAN8720) para OTA seguro.
- **Estación meteorológica por Ethernet**: aplicar el mismo patrón (GET manual
  sobre `Client*`) a `WeatherStation`. **Diferido para el futuro**: no se inicia
  hasta terminar la modularidad completa (v3.18.0+).

Se deja registrado el resto para cuando se defina el hardware de red definitivo.

## 11. Gateway RS485 (polling multi-esclavo por perfiles) — implementado

`ModbusGateway` (`sensors/ModbusGateway`) sondea múltiples esclavos RS485/Modbus
según las instancias de `ModbusProfileRegistry`, usando `ModbusRtu` como driver
físico. Convierte registros (UINT16/INT16/UINT32/INT32/FLOAT32) con escala/offset
y expone estado por esclavo (`OK / TIMEOUT / CRC_ERROR / DISCONNECTED`) en
`GET /api/v1/modbus/gateway`.

### Diseño

```text
ModbusGateway
    │
    ├── poll(slave_id, register, data_type, scale, offset)
    │        └── ModbusRtu (driver físico)
    │
    ├── tabla de esclavos (de ModbusProfileRegistry.instances)
    │        ├── slave 1 → pH (perfil ph-generic)
    │        ├── slave 2 → EC (perfil ec-generic)
    │        └── slave N → ...
    │
    └── valores + estado (GET /api/v1/modbus/gateway)
```

### Pendiente (mejoras)

- Integrar los valores del gateway en `SensorManager` (magnitudes) para que los
  controladores los usen directamente.
- Publicar por MQTT los valores/estado del gateway.
- `poll_interval_ms` configurable por instancia (hoy fijo en 5000 ms).

## 12. Estado de MEJORAS (resumen)

| Ítem | Estado |
|------|--------|
| Gateway RS485 (polling multi-esclavo por perfiles) | ✅ v3.17.0 |
| OTA HTTP sobre Ethernet | ✅ v3.16.0 |
| HTTPS sobre W5500 (W5500lwIP / ETH nativo) | ❌ pendiente |
| Estación meteorológica por Ethernet (GET sobre `Client*`) | ❌ pendiente |
| Resto de MEJORAS | ✅ implementado |

## 13. Configurabilidad completa de hardware — pendiente (evaluación)

### ¿Es modular el proyecto?

**Parcialmente.** La configuración **funcional** sí es 100 % editable desde la web
sin tocar código; la configuración **de hardware** todavía es compile-time.

### Configurable desde la web/API (sin recompilar)

- **Sensores**: habilitar/deshabilitar (`sht31`, `ds18b20`, `soil`, `light`, `co2`,
  `rain`, `wind`, `tank`, `flow`, `ph`, `ec`, `exterior`).
- **Actuadores**: habilitar + cantidad (`pump`, `valves`, `fans`, `extractors`,
  `lights`, `heater`, `humidifier`, `roof`, `window`, `shade`).
- **Umbrales, calibración, zonas, horarios**, reglas, variables calculadas.
- **Red**: WiFi/Ethernet (`net_interface`), MQTT, NTP, DNS, servidor central.
- **Token de API, canal de actualización, simulación**.
- **Perfiles Modbus** (declarativos, `ModbusProfileRegistry`).

### NO configurable (compile-time)

- **Selección del modelo de sensor** (drivers compilados; no se puede agregar un
  modelo nuevo desde la web).
- **Expansores** (74HC165 / MCP23S17 / ADC) y sus pines/CS/canales (el catálogo
  es estático en `HardwareManager::begin()`).
- **Buses adicionales** (registrados en `BusManager::begin()`).

### Camino a modularidad completa (Tasmota-like)

1. ✅ **Pines en tiempo de ejecución (v3.18.0–v3.19.0)**: `PinConfig` +
   `PinConfigManager` en NVS (`ghpins`); sensores/actuadores/buses leen pines de
   NVS. Expuesto en `GET/PUT /api/v1/pins` (PUT protegido) y con **formulario web
   editable en `/pins`** (v3.19.0).
2. **Catálogo de sensores instanciable** (en curso):
   - ✅ v3.20.0 — `SensorRegistry` editable y persistido (`PUT /api/v1/sensors/catalog`).
   - ✅ v3.21.0 — `SensorManager` lee las **direcciones I²C** (SHT31/AHT20/ADS1115/BH1750/SCD41) del catálogo.
   - ✅ v3.22.0 — el **`enabled` del catálogo** controla el reporte de los I²C.
   - ❌ Falta: extender `enabled`/dirección por catálogo al resto (1-Wire, pulsos,
     tanque, pH/EC) y la **configuración de expansores** (paso 3).

> **Bloqueo de pines (v3.22.0):** `GH_PINS_LOCKED` (0 público / 1 PCB fija). Con
> PCB fija, los pines no se editan (web bloqueada, `PUT` → 403), pero el catálogo
> de sensores/actuadores sigue abierto al público para configurar su instalación.
3. **Configuración de expansores**: tipo (74HC165/MCP23S17/ADC) + bus + CS/dir +
   canales, editable desde la web (protegido, README §204).
4. **Persistencia + migraciones** (ya hay `schema_version` + `migrate`).

### Impacto

- Requiere refactor de `SensorManager`/`ActuatorManager`/`HardwareManager` para
  leer pines/direcciones desde configuración en lugar de `PinMap`.
- Es el hito que convierte el firmware en "plataforma" completa (README §282-286).

> La UI web actual permite habilitar/deshabilitar y ajustar parámetros, pero **no**
> elegir pines ni modelos de sensor; eso está documentado como "Hardware avanzado
> protegido" (README §203-204) y sigue siendo compile-time.
