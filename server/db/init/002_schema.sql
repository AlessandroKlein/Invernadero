-- =============================================================
-- Esquema (parte 2): sensores, actuadores, alarmas, configuración, firmware
-- =============================================================

-- ------------------------- Sensores -------------------------
CREATE TABLE sensors (
    id        UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id UUID        NOT NULL REFERENCES devices(id) ON DELETE CASCADE,
    zone_id   UUID        REFERENCES zones(id) ON DELETE SET NULL,
    sensor_id VARCHAR(40) NOT NULL,       -- TEMP-01, PH-01...
    name      VARCHAR(120),
    type      VARCHAR(40),                -- temperature/humidity/soil/co2/ph/ec...
    interface VARCHAR(20),                -- I2C/ONEWIRE/ADC/GPIO/RS485/MODBUS
    unit      VARCHAR(20),
    enabled   BOOLEAN     NOT NULL DEFAULT TRUE,
    config    JSONB       DEFAULT '{}',
    UNIQUE (device_id, sensor_id)
);

-- Telemetría (índice de consulta; retención configurable vía job).
CREATE TABLE sensor_readings (
    id        BIGSERIAL PRIMARY KEY,
    sensor_id UUID        NOT NULL REFERENCES sensors(id) ON DELETE CASCADE,
    value     DOUBLE PRECISION NOT NULL,
    unit      VARCHAR(20),
    quality   VARCHAR(20) DEFAULT 'GOOD',
    ts        TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX idx_readings_sensor_ts ON sensor_readings (sensor_id, ts DESC);

-- ------------------------- Actuadores -------------------------
CREATE TABLE actuators (
    id          UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id   UUID        NOT NULL REFERENCES devices(id) ON DELETE CASCADE,
    zone_id     UUID        REFERENCES zones(id) ON DELETE SET NULL,
    actuator_id VARCHAR(40) NOT NULL,     -- pump, valve1, fan1...
    name        VARCHAR(120),
    role        VARCHAR(20),              -- pump/valve/fan/extractor/light/heater...
    type        VARCHAR(20),              -- DIGITAL/PWM
    enabled     BOOLEAN     NOT NULL DEFAULT TRUE,
    config      JSONB       DEFAULT '{}',
    UNIQUE (device_id, actuator_id)
);

CREATE TABLE actuator_states (
    id          BIGSERIAL PRIMARY KEY,
    actuator_id UUID        NOT NULL REFERENCES actuators(id) ON DELETE CASCADE,
    state       BOOLEAN     NOT NULL,
    output      INTEGER     DEFAULT 0,     -- 0..100 (PWM) o 0/100
    ts          TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- ------------------------- Alarmas y eventos -------------------------
CREATE TABLE alarms (
    id           UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    device_id    UUID REFERENCES devices(id) ON DELETE CASCADE,
    zone_id      UUID REFERENCES zones(id) ON DELETE SET NULL,
    type         VARCHAR(40) NOT NULL,     -- SENSOR_ERROR, LOW_TANK, NO_FLOW...
    severity     VARCHAR(20) NOT NULL DEFAULT 'WARNING',
    status       VARCHAR(20) NOT NULL DEFAULT 'ACTIVE',
    message      TEXT,
    source       VARCHAR(20) DEFAULT 'DEVICE',
    acknowledged BOOLEAN NOT NULL DEFAULT FALSE,
    resolved_at  TIMESTAMPTZ,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE events (
    id         BIGSERIAL PRIMARY KEY,
    device_id  UUID REFERENCES devices(id) ON DELETE CASCADE,
    type       VARCHAR(40) NOT NULL,
    message    TEXT,
    payload    JSONB,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
