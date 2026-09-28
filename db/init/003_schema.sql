-- =============================================================
-- Esquema (parte 3): configuración, auditoría, firmware/OTA, Modbus, automatización
-- =============================================================

-- ------------------------- Configuración versionada -------------------------
CREATE TABLE configurations (
    id             UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id      UUID     NOT NULL REFERENCES devices(id) ON DELETE CASCADE,
    config_version INTEGER  NOT NULL,
    source         VARCHAR(20) DEFAULT 'CENTRAL',   -- LOCAL/CENTRAL
    config         JSONB    NOT NULL DEFAULT '{}',
    applied        BOOLEAN  NOT NULL DEFAULT FALSE,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (device_id, config_version)
);

-- ------------------------- Auditoría -------------------------
CREATE TABLE audit_logs (
    id         BIGSERIAL PRIMARY KEY,
    user_id    UUID REFERENCES users(id) ON DELETE SET NULL,
    device_id  UUID REFERENCES devices(id) ON DELETE SET NULL,
    action     VARCHAR(40) NOT NULL,
    entity     VARCHAR(80),
    old_value  JSONB,
    new_value  JSONB,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- ------------------------- Firmware y OTA -------------------------
CREATE TABLE firmware_versions (
    id               UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    project          VARCHAR(40) NOT NULL DEFAULT 'Invernadero',
    channel          VARCHAR(20) NOT NULL DEFAULT 'stable',
    version          VARCHAR(20) NOT NULL,
    hardware_profile VARCHAR(40),
    url              TEXT,
    sha256           VARCHAR(64),
    release_date     DATE,
    min_bootloader   VARCHAR(20),
    min_hardware     VARCHAR(20),
    config_schema    INTEGER,
    protocol         INTEGER,
    UNIQUE (channel, version, hardware_profile)
);

CREATE TABLE ota_jobs (
    id           UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    firmware_id  UUID NOT NULL REFERENCES firmware_versions(id),
    device_id    UUID NOT NULL REFERENCES devices(id) ON DELETE CASCADE,
    status       VARCHAR(20) NOT NULL DEFAULT 'PENDING',
    result       TEXT,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT now(),
    completed_at TIMESTAMPTZ
);

-- ------------------------- Modbus / industriales -------------------------
CREATE TABLE modbus_profiles (
    id           UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name         VARCHAR(120) NOT NULL,
    manufacturer VARCHAR(80),
    model        VARCHAR(80),
    baudrate     INTEGER DEFAULT 9600,
    parity       VARCHAR(10) DEFAULT 'NONE',
    stop_bits    INTEGER DEFAULT 1,
    register_map JSONB DEFAULT '{}',
    unit         VARCHAR(20),
    scale        DOUBLE PRECISION DEFAULT 1.0,
    offset       DOUBLE PRECISION DEFAULT 0.0
);

CREATE TABLE modbus_devices (
    id         UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id  UUID NOT NULL REFERENCES devices(id) ON DELETE CASCADE,
    profile_id UUID REFERENCES modbus_profiles(id) ON DELETE SET NULL,
    uid        VARCHAR(40) UNIQUE,        -- identidad permanente
    slave_id   INTEGER NOT NULL,
    name       VARCHAR(120),
    zone_id    UUID REFERENCES zones(id) ON DELETE SET NULL
);

-- ------------------------- Automatización -------------------------
CREATE TABLE automation_rules (
    id         UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id  UUID REFERENCES devices(id) ON DELETE CASCADE,
    name       VARCHAR(120) NOT NULL,
    conditions JSONB NOT NULL DEFAULT '[]',
    actions    JSONB NOT NULL DEFAULT '[]',
    enabled    BOOLEAN NOT NULL DEFAULT TRUE,
    priority   INTEGER NOT NULL DEFAULT 100
);

CREATE TABLE schedules (
    id        UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id UUID REFERENCES devices(id) ON DELETE CASCADE,
    name      VARCHAR(120) NOT NULL,
    cron      VARCHAR(80),
    action    JSONB NOT NULL DEFAULT '{}',
    enabled   BOOLEAN NOT NULL DEFAULT TRUE
);

-- ------------------------- Notificaciones -------------------------
CREATE TABLE notifications (
    id         UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id    UUID REFERENCES users(id) ON DELETE CASCADE,
    type       VARCHAR(20) DEFAULT 'INFO',
    target     VARCHAR(160),
    message    TEXT,
    read       BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
