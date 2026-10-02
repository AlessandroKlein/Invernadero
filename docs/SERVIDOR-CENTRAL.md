# Servidor Central — Plataforma multi-proyecto

> **Tipo:** Backend/Web | **Estado:** Especificación | **Fecha:** 2026-10-02
>
> Documento de diseño del **servidor central unificado** que administrará **todos
> los proyectos** de AlessandroKlein (Invernadero, SEMA y futuros). El objetivo es
> que cada nuevo proyecto se incorpore **sin modificar ni afectar** a los
> existentes.

---

## 1. Objetivo

Un único servidor central que:

- registre, autentique y supervise **dispositivos de múltiples proyectos**;
- ingiera telemetría (MQTT/HTTP) y la guarde de forma **aislada por proyecto**;
- exponga una API + dashboard unificados;
- permita OTA, configuración, alarmas, usuarios y auditoría;
- sea **extensible por proyecto** mediante módulos/perfiles sin tocar el núcleo.

Principio rector (heredado de Invernadero):

> **El servidor administra y coordina. El dispositivo (ESP32) controla y protege.**
> El servidor nunca es necesario para una función automática crítica.

---

## 2. Principios de diseño

1. **Multi-tenant por proyecto**: cada proyecto es un espacio aislado (`project`).
2. **Modular**: el núcleo (auth, dispositivos, telemetría, OTA) es común; lo
   específico de cada proyecto vive en un módulo/perfil.
3. **API versionada**: `/api/v1/...` con versionado semántico y sin ruptura.
4. **Comunicación por MQTT + HTTP**: los dispositivos publican por MQTT; el
   control/consulta por REST.
5. **Docs-as-code**: la documentación de cada proyecto vive en su propio módulo.

---

## 3. Arquitectura

```text
                    ┌───────────────────────────────────────┐
                    │           SERVIDOR CENTRAL            │
                    │                                       │
                    │  API Gateway  (REST, versionada)      │
                    │  Auth (JWT) + RBAC + tokens           │
                    │  Device Registry / Provisioning       │
                    │  Telemetry Ingest (MQTT→DB)           │
                    │  Shadow (desired/reported/actual)     │
                    │  Config Manager (versionada)          │
                    │  OTA Manager (grupos/canales)         │
                    │  Alert/Event Engine                   │
                    │  Audit + Notifications                │
                    └───────────────┬───────────────────────┘
                                    │
              ┌─────────────────────┼─────────────────────┐
              ▼                     ▼                     ▼
        Time-Series DB        Relational DB         File Storage
        (readings/telemetría) (definición/config)   (firmware .bin)
```

Stack recomendado (hoy: PHP 8.2 + PostgreSQL 16 + Mosquitto + Docker):

| Capa | Tecnología | Notas |
|------|-----------|-------|
| API | PHP 8.x (front controller) o Node/Go | elegir según equipo |
| DB relacional | PostgreSQL 16 | definición, config, shadow, usuarios |
| Series temporales | PostgreSQL (particionado) o TimescaleDB | telemetría |
| Mensajería | Mosquitto (MQTT 1883 + WS 9001) | broker |
| Cache/límites | Redis (opcional) | rate limiting distribuido |
| Orquestación | Docker Compose | despliegue portable |

---

## 4. Modelo de datos multi-proyecto

El aislamiento por proyecto se logra con una columna `project` (o `project_id`)
en **todas** las tablas de dominio, y un prefijo por proyecto en MQTT.

```text
projects
  └── devices           (pertenece a un project)
        └── sensors / actuators / zones
        └── sensor_readings / actuator_states / events / alarms
        └── device_shadow / configurations
```

Ejemplo de tablas clave (resumen):

```text
projects(id, slug, name, schema_version, enabled)
devices(id, project_id, device_id, device_uid, hardware, firmware, ...)
sensors(id, device_id, sensor_id, magnitude, unit, ...)
sensor_readings(id, sensor_id, value, unit, quality, sequence, ts)
device_shadow(device_id, desired, reported, actual, updated_at)
configurations(device_id, project_id, config_version, source, config, applied)
firmware_versions(project_id, channel, version, hardware_profile, url, sha256)
ota_jobs(firmware_id, device_id, status, result, created_at)
users / roles / permissions / user_roles / audit_logs
```

> `sequence` (BIGINT) en la telemetría habilita **Store & Forward** y
> sincronización incremental (`after_sequence`), ya implementado en la rama `server`.

---

## 5. Aislamiento por proyecto (clave)

- **MQTT**: tópicos con prefijo `{project}/{device_id}/...` (hoy
  `greenhouse/{id}/...`; el nuevo usa `{project}/{id}/...`).
- **REST**: rutas con `project` (`/api/v1/{project}/devices/...`) o scope JWT.
- **RBAC con scope**: cada usuario tiene un `scope` que limita los proyectos que
  ve (`*` = todos, o lista de `project_id`).
- **Configuración por proyecto**: cada proyecto tiene su propio `schema_version` y
  sus propias migraciones, sin interferir con los demás.

---

## 6. API (versionada y extensible)

```text
POST /api/v1/auth/login                 → JWT
GET  /api/v1/projects                   → lista (según scope)
GET  /api/v1/{project}/devices
GET  /api/v1/{project}/devices/{id}
GET  /api/v1/{project}/devices/{id}/readings?after_sequence=N
POST /api/v1/{project}/devices/{id}/readings      (ingesta HTTP)
GET  /api/v1/{project}/devices/{id}/shadow
PUT  /api/v1/{project}/devices/{id}/shadow
GET/POST /api/v1/{project}/devices/{id}/config
POST /api/v1/{project}/devices/{id}/ota
POST /api/v1/firmware/upload
```

Extensiones por proyecto se registran como sub-recursos del módulo del proyecto.

---

## 7. Seguridad

- **JWT** (Bearer) con `sub`, `roles`, `perms` y `scope`.
- **Contraseñas** con `password_hash()` (bcrypt/Argon2).
- **RBAC granular** (permisos por recurso: `device.read`, `device.control`, ...).
- **Token de API** por dispositivo (generado en la web local del ESP32) para
  autorizar control desde el servidor.
- **Rate limiting** por IP (ya implementado, `src/RateLimiter.php`).
- **CORS configurable** (`CORS_ORIGIN`), MQTT con usuario/clave + TLS en producción.
- **MFA/TOTP** en una fase posterior (tablas preparadas).

---

## 8. Provisioning y ciclo de vida de dispositivos

```text
DISCOVERED → PENDING → COMMISSIONED → ACTIVE
                            (y BLOCKED / REVOKED)
```

Un dispositivo nuevo publica `discovery`; el servidor lo deja en `PENDING` hasta
que un administrador lo aprueba (nunca auto-registro por defecto; el
`AUTO-COMMISSION` es opcional y explícito).

---

## 9. Guía de extensión (agregar un proyecto nuevo sin afectar otros)

1. Crear un **módulo de proyecto** (carpeta propia) con su:
   - `schema` (migraciones SQL aisladas por `project`);
   - `profile` (catálogo de sensores/actuadores/reglas);
   - `config` (schema_version y defaults);
   - `docs` (documentación en `Docs/`).
2. Registrar el `slug` del proyecto en la tabla `projects`.
3. Los dispositivos publican en `{slug}/{device_id}/...`; el worker MQTT los rutea
   por `project_id`.
4. El dashboard muestra el proyecto si el usuario tiene `scope` sobre él.

**Regla de oro**: no modificar el núcleo (auth, devices, telemetry, ota) para
agregar un proyecto. Todo lo específico va en el módulo del proyecto.

---

## 10. Roadmap

1. Migrar el servidor actual (rama `server`, específico de Invernadero) a este
   modelo multi-proyecto (`projects` + `project_id`).
2. Generalizar el prefijo MQTT a `{project}/{device_id}/...` manteniendo
   compatibilidad con `greenhouse/...`.
3. Agregar TimescaleDB (opcional) para telemetría a gran escala.
4. MFA/TOTP, refresh tokens y gestión de sesiones.
5. Notificaciones (email/webhook) y auditoría por UI.

---

Ver también: [`docs/DUDAS-Y-DECISIONES.md`](DUDAS-Y-DECISIONES.md) (decisiones),
[`docs/ESTANDAR-DOCUMENTACION.md`](ESTANDAR-DOCUMENTACION.md) (estándar de wiki) y
la rama `server` del repositorio (implementación actual).
