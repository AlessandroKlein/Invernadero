# Servidor central de Invernadero

Aplicación web (PHP + PostgreSQL + MQTT) que centraliza la supervisión,
configuración, históricos, alarmas y administración de múltiples ESP32.

## Stack

| Capa | Tecnología |
|------|-----------|
| Frontend | HTML + CSS + JS (Chart.js, mqtt.js vía CDN) |
| Backend / API | PHP 8.2 + Apache (front controller) |
| Base de datos | PostgreSQL 16 |
| Mensajería | Mosquitto (MQTT + WebSocket) |
| Orquestación | Docker Compose |

## Puesta en marcha

```bash
cd server
cp .env.example .env        # ajustar claves/puertos
docker compose up -d --build
```

Servicios:

- Dashboard: http://localhost:8080
- API: http://localhost:8080/api/v1/...
- Adminer (DB): http://localhost:8081
- MQTT: localhost:1883 (nativo) y localhost:9001 (WebSocket)

Credenciales por defecto: **admin / password** (cambiar en producción, ver
`db/init/004_seed.sql`).

## Worker MQTT

El bridge que persiste la telemetría se ejecuta dentro del contenedor PHP:

```bash
docker compose exec php php mqtt/worker.php
```

Se suscribe a `greenhouse/+/#` y escribe en PostgreSQL (estado, sensores,
actuadores, eventos, alarmas).

## Estructura

```
server/
├── docker-compose.yml
├── Dockerfile
├── apache.conf
├── composer.json
├── .env.example
├── config/config.php          # configuración (env)
├── db/init/                   # esquema (001-003) y seed (004)
├── src/                       # Database, Auth, Response, Api (REST)
├── public/                    # index.php (router) + dashboard (index.html, css, js)
├── mqtt/                      # mosquitto.conf + worker.php
└── docs/ARQUITECTURA.md
```

## API REST (resumen)

| Método | Ruta | Descripción |
|--------|------|-------------|
| POST | `/api/v1/auth/login` | Login → JWT |
| GET | `/api/v1/status` | Health check |
| GET/POST | `/api/v1/greenhouses` | Invernaderos |
| GET | `/api/v1/devices` | Dispositivos |
| GET | `/api/v1/devices/{id}` | Detalle |
| GET | `/api/v1/devices/{id}/sensors` · `/actuators` · `/alarms` | Recursos |
| GET/POST | `/api/v1/devices/{id}/readings` | Telemetría |
| GET/PUT | `/api/v1/devices/{id}/shadow` | Device Shadow |
| GET/POST | `/api/v1/devices/{id}/config` | Configuración versionada |
| GET | `/api/v1/firmware` | Versiones de firmware |

El resto de endpoints requieren header `Authorization: Bearer <token>`.

## Dudas / decisiones

Ver `../docs/DUDAS-Y-DECISIONES.md` y `docs/ARQUITECTURA.md`.
