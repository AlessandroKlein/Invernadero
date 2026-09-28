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

Credenciales por defecto: **admin / password** (superadmin; cambiar en producción,
ver `db/init/004_seed.sql`).

## Roles y permisos (RBAC)

El servidor implementa **RBAC** con 8 roles (`superadmin`, `admin`, `integrator`,
`maintenance`, `supervisor`, `operator`, `auditor`, `viewer`), **permisos granulares**
(`device.read`, `device.configure`, `user.create`, `hardware.configure`, …) y
**alcance (scope)** por invernadero. Los permisos viven en la base de datos y el
backend valida **cada petición** (el frontend solo oculta opciones; no es la
protección).

El dashboard de administración permite al superadmin/administrador **crear, editar y
eliminar usuarios**, asignarles roles y definir su scope (`*` = todos los
invernaderos). Solo un `superadmin` puede asignar el rol `superadmin`.

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
| POST | `/api/v1/auth/login` | Login → JWT (roles + permisos + scope) |
| GET | `/api/v1/auth/me` | Usuario autenticado |
| GET | `/api/v1/roles` | Listar roles (`user.read`) |
| GET | `/api/v1/permissions` | Permisos y su asignación por rol |
| GET/POST | `/api/v1/users` | Listar / crear usuarios (`user.read` / `user.create`) |
| PUT/DELETE | `/api/v1/users/{id}` | Editar / eliminar usuario |
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

## Exposición pública (acceso desde cualquier IP)

El contenedor PHP ya escucha en **todas las interfaces** (Apache `<VirtualHost *:80>`
y `ports: "${HTTP_PORT:-8080}:80"`), por lo que es accesible desde cualquier IP de la
red usando `http://<ip-del-host>:8080`.

Para exponerlo a Internet, dos opciones:

1. **Túnel de Cloudflare** (sin abrir puertos):
   ```bash
   # en .env:  TUNNEL_TOKEN=<token>
   docker compose --profile tunnel up -d cloudflared
   ```
   El servicio `cloudflared` (ya incluido en `docker-compose.yml`) publica el
   dashboard y la API a través de un dominio de Cloudflare con TLS.

2. **Nginx Proxy Manager** (o cualquier reverse proxy): apuntar un subdominio al
   contenedor `php` en el puerto `8080`, con `Websockets Support` activado (para
   `/ws` y MQTT-over-WS en el 9001) y certificado Let's Encrypt.

> Recomendación de producción: exponer solo por HTTPS y cambiar el `JWT_SECRET`
> y la contraseña del superadmin.

## Decisiones / pendientes

Ver `docs/ARQUITECTURA.md` y, en la rama `main` del proyecto, `docs/DUDAS-Y-DECISIONES.md`.
