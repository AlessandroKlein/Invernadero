# Mejoras recomendadas

> **Tipo:** Roadmap técnico | **Estado:** En desarrollo | **Fecha:** 2026-10-02
>
> Documento que consolida las mejoras identificadas al contrastar el proyecto
> Invernadero con el README de [SEMA](https://github.com/AlessandroKlein/SEMA)
> y con las decisiones de `docs/DUDAS-Y-DECISIONES.md`. Marca qué ya está
> implementado y qué queda pendiente, con prioridad sugerida.

---

## 1. Resumen (v3.9.0 → v3.10.0)

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
| W5500 Ethernet (librería SPI + interfaz de red intercambiable) | ❌ pendiente |
| SD (backend de `StorageManager`) | ❌ pendiente |
| Configuración por capas + migraciones | ❌ pendiente |
| Event Bus + scheduler por capacidades + Store & Forward | ❌ pendiente |

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

## 8. Otras mejoras de SEMA aplicables (prioridad media/baja)

- **Autodetección guiada** (SEMA §210): escanear buses → detectar dispositivos →
  asociar funciones sin recompilar.
- **Event Bus / Event Manager** (SEMA §204-205): desacoplar módulos ante eventos
  (lluvia, alarma, batería, red).
- **Scheduler por capacidades** (SEMA §206-208): crear tareas solo si el módulo
  está habilitado.
- **Store & Forward** (SEMA §241-243): sincronización incremental con
  `sequence_id` para no perder datos offline.
- **API Keys + revocación** (SEMA §164-165) para integraciones externas.
- **Rate limiting y CORS** (SEMA §166-167).

---

## 9. Prioridad sugerida

1. Proteger `SensorManager` (snapshot por cola) y dividir `SensorTask`/`ControlTask`.
2. Perfiles Modbus declarativos + commissioning (V9).
3. Configuración por capas + `schema_version`/migraciones.
4. Event Bus + scheduler por capacidades.
5. Store & Forward + sincronización incremental con el servidor.
6. Integración CAN de aplicación (tras definir hardware).
