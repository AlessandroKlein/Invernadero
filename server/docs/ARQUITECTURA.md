# Arquitectura del servidor central

> **Tipo:** Backend/Web | **Estado:** En desarrollo | **Fecha:** 2026-09-28

## 1. Descripción

El servidor central es una aplicación **independiente del firmware** que
supervisa y administra múltiples invernaderos/ESP32. El ESP32 sigue siendo el
**controlador de campo autónomo**; el servidor es la capa de supervisión,
históricos, configuración y OTA.

## 2. Diagrama

```
                        DASHBOARD (HTML/CSS/JS)
                               │  fetch / MQTT-WS
                    ┌──────────┴──────────┐
                    │      PHP/Apache     │
                    │   public/index.php  │
                    │      src/Api.php    │
                    └──────────┬──────────┘
                               │ PDO
                         PostgreSQL 16
                               ▲
                               │  worker.php (CLI)
                    ┌──────────┴──────────┐
                    │   Mosquitto (MQTT)  │
                    └──────────┬──────────┘
                               │ greenhouse/+/#
                        ESP32 (devices)
```

## 3. Flujo de datos

1. El ESP32 publica en `greenhouse/<device_id>/{state,sensors,actuators,...}`.
2. `mqtt/worker.php` (suscriptor de `greenhouse/+/#`) persiste en PostgreSQL
   (auto-registra el dispositivo si no existe).
3. La API REST (`/api/v1/...`) expone los datos con JWT.
4. El dashboard consulta la API (polling) y opcionalmente se suscribe a MQTT over
   WebSocket para tiempo real.

## 4. Componentes

- **Front controller** `public/index.php`: enruta `/api/v1/*` a la API y sirve la SPA.
- **`src/Api.php`**: controladores REST (login, invernaderos, dispositivos, sensores,
  actuadores, lecturas, alarmas, shadow, config, firmware, eventos).
- **`src/Database.php`**: PDO a PostgreSQL.
- **`src/Auth.php`**: JWT (emisión/verificación).
- **`mqtt/worker.php`**: bridge MQTT→DB.

## 5. Modelo de datos (resumen)

Separación **definición** vs **telemetría**:

- Definición: `greenhouses`, `zones`, `devices`, `sensors`, `actuators`,
  `modbus_profiles`, `automation_rules`, `schedules`, `firmware_versions`.
- Telemetría: `sensor_readings`, `actuator_states`, `events`, `alarms`.
- Configuración: `configurations` (versionada), `device_shadow` (desired/reported).
- Seguridad/auditoría: `users`, `roles`, `audit_logs`, `notifications`.

## 6. Seguridad

- JWT para la API; contraseñas con `password_hash()` (bcrypt).
- CORS abierto en desarrollo (restringir en producción).
- MQTT con `allow_anonymous true` en desarrollo; habilitar usuario/clave y TLS
  (`MQTTS`) en producción.

## 7. Pendientes / decisiones

Ver `../docs/DUDAS-Y-DECISIONES.md`: retención de históricos, VPD, estación
meteorológica, sunrise/sunset, TLS, roles por endpoint, etc.
