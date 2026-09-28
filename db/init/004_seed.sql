-- =============================================================
-- Datos iniciales (seed): roles, usuario admin, invernadero de ejemplo
-- =============================================================

INSERT INTO roles (name, description) VALUES
    ('admin',       'Acceso total'),
    ('operator',    'Opera y modifica parámetros'),
    ('viewer',      'Solo lectura'),
    ('maintenance', 'Mantenimiento y diagnóstico');

-- Usuario admin por defecto: usuario "admin", contraseña "password".
-- El hash bcrypt corresponde a "password". Para generar el propio:
--   php -r "echo password_hash('TU_CLAVE', PASSWORD_BCRYPT), PHP_EOL;"
INSERT INTO users (role_id, username, email, password_hash) VALUES
    (1, 'admin', 'admin@example.com',
     '$2y$10$92IXUNpkjO0rOQ5byMi.Ye4oKoEa3Ro9llC/.og/at2.uheWG/igi');

-- Invernadero de ejemplo
INSERT INTO greenhouses (name, type, timezone) VALUES
    ('Invernadero Principal', 'outdoor', 'America/Argentina/Buenos_Aires');

-- Firmware de referencia (manifest)
INSERT INTO firmware_versions
    (project, channel, version, hardware_profile, release_date, min_hardware, config_schema, protocol)
VALUES
    ('Invernadero', 'stable', '3.1.0', 'ESP32-GH-V1', '2026-09-28', 'rev0', 1, 1);
