-- Store & Forward (SEMA §241-243): secuencia monotónica para sincronización
-- incremental y detección de huecos.
ALTER TABLE sensor_readings ADD COLUMN IF NOT EXISTS sequence BIGINT;
CREATE INDEX IF NOT EXISTS idx_readings_sequence ON sensor_readings (sequence);
