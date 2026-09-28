-- =============================================================
-- Plataforma Invernadero - Esquema (parte 1): usuarios, invernaderos, dispositivos
-- Separación: DEFINICIÓN (configuración) vs TELEMETRÍA (mediciones).
-- =============================================================

CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- ------------------------- RBAC: roles, permisos, usuarios -------------------------
CREATE TABLE roles (
    id          SERIAL PRIMARY KEY,
    name        VARCHAR(40)  NOT NULL UNIQUE,
    description TEXT
);

CREATE TABLE permissions (
    id          SERIAL PRIMARY KEY,
    code        VARCHAR(60)  NOT NULL UNIQUE,
    description TEXT
);

CREATE TABLE role_permissions (
    role_id       INTEGER NOT NULL REFERENCES roles(id) ON DELETE CASCADE,
    permission_id INTEGER NOT NULL REFERENCES permissions(id) ON DELETE CASCADE,
    PRIMARY KEY (role_id, permission_id)
);

CREATE TABLE users (
    id            UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    username      VARCHAR(80)  NOT NULL UNIQUE,
    email         VARCHAR(160) UNIQUE,
    full_name     VARCHAR(160),
    password_hash VARCHAR(255) NOT NULL,
    active        BOOLEAN      NOT NULL DEFAULT TRUE,
    scope         JSONB        NOT NULL DEFAULT '{"greenhouses": ["*"]}',
    last_login    TIMESTAMPTZ,
    created_at    TIMESTAMPTZ  NOT NULL DEFAULT now()
);

CREATE TABLE user_roles (
    user_id UUID    NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    role_id INTEGER NOT NULL REFERENCES roles(id) ON DELETE CASCADE,
    PRIMARY KEY (user_id, role_id)
);

-- ------------------------- Invernaderos y zonas -------------------------
CREATE TABLE greenhouses (
    id         UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name       VARCHAR(120) NOT NULL,
    type       VARCHAR(20)  NOT NULL DEFAULT 'outdoor',  -- indoor/outdoor/mixed
    latitude   DOUBLE PRECISION,
    longitude  DOUBLE PRECISION,
    timezone   VARCHAR(64)  NOT NULL DEFAULT 'America/Argentina/Buenos_Aires',
    created_at TIMESTAMPTZ  NOT NULL DEFAULT now()
);

CREATE TABLE zones (
    id            UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    greenhouse_id UUID         NOT NULL REFERENCES greenhouses(id) ON DELETE CASCADE,
    name          VARCHAR(80)  NOT NULL,
    sort_order    INTEGER      NOT NULL DEFAULT 0
);

-- ------------------------- Dispositivos -------------------------
CREATE TABLE devices (
    id               UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    greenhouse_id    UUID REFERENCES greenhouses(id) ON DELETE SET NULL,
    device_uid       VARCHAR(40) NOT NULL UNIQUE,      -- identidad permanente (MAC)
    device_id        VARCHAR(40) NOT NULL UNIQUE,      -- GH-001
    name             VARCHAR(120),
    hardware_profile VARCHAR(40) DEFAULT 'ESP32-GH-V1',
    hardware_version VARCHAR(20) DEFAULT 'rev0',
    firmware_version VARCHAR(20) DEFAULT '0.0.0',
    config_schema    INTEGER     DEFAULT 1,
    protocol         INTEGER     DEFAULT 1,
    capabilities     JSONB       DEFAULT '[]',
    status           VARCHAR(20) DEFAULT 'OFFLINE',    -- ONLINE/OFFLINE/DEGRADED/...
    ip               VARCHAR(45),
    mac              VARCHAR(17),
    last_seen        TIMESTAMPTZ,
    created_at       TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE device_shadow (
    id         UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id  UUID NOT NULL UNIQUE REFERENCES devices(id) ON DELETE CASCADE,
    desired    JSONB NOT NULL DEFAULT '{}',
    reported   JSONB NOT NULL DEFAULT '{}',
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
