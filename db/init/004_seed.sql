-- =============================================================
-- Datos iniciales (seed): roles, usuario admin, invernadero de ejemplo
-- =============================================================

INSERT INTO roles (name, description) VALUES
    ('superadmin',  'Administrador absoluto del sistema'),
    ('admin',       'Administra instalaciones y usuarios'),
    ('integrator',  'Configura hardware, buses y automatización'),
    ('maintenance', 'Diagnóstico y mantenimiento de campo'),
    ('supervisor',  'Supervisión de producción y reportes'),
    ('operator',    'Operación diaria del invernadero'),
    ('auditor',     'Solo lectura y auditoría'),
    ('viewer',      'Consulta básica (solo lectura)');

-- ------------------------- Permisos -------------------------
INSERT INTO permissions (code, description) VALUES
    ('greenhouse.read',      'Ver invernaderos'),
    ('greenhouse.create',    'Crear invernaderos'),
    ('greenhouse.edit',      'Editar invernaderos'),
    ('greenhouse.delete',    'Eliminar invernaderos'),
    ('device.read',          'Ver dispositivos'),
    ('device.control',       'Control manual de actuadores'),
    ('device.restart',       'Reiniciar dispositivos'),
    ('device.configure',     'Configurar dispositivos'),
    ('device.delete',        'Eliminar dispositivos'),
    ('sensor.read',          'Ver sensores'),
    ('sensor.configure',     'Configurar sensores'),
    ('sensor.calibrate',     'Calibrar sensores'),
    ('actuator.read',        'Ver actuadores'),
    ('actuator.control',     'Controlar actuadores'),
    ('actuator.configure',   'Configurar actuadores'),
    ('automation.read',      'Ver automatización'),
    ('automation.execute',   'Ejecutar automatización'),
    ('automation.configure', 'Configurar automatización'),
    ('hardware.read',        'Ver configuración de hardware'),
    ('hardware.configure',   'Configurar hardware (GPIO/SPI/I2C/RS485)'),
    ('modbus.read',          'Leer dispositivos Modbus'),
    ('modbus.configure',     'Configurar Modbus'),
    ('network.read',         'Ver configuración de red'),
    ('network.configure',    'Configurar red'),
    ('firmware.read',        'Ver firmware'),
    ('firmware.update',      'Actualizar firmware'),
    ('ota.execute',          'Ejecutar OTA'),
    ('alarm.read',           'Ver alarmas'),
    ('alarm.ack',            'Reconocer alarmas'),
    ('user.read',            'Ver usuarios'),
    ('user.create',          'Crear usuarios'),
    ('user.edit',            'Editar usuarios'),
    ('user.delete',          'Eliminar usuarios'),
    ('audit.read',           'Ver auditoría'),
    ('diagnostics.execute',  'Ejecutar diagnóstico'),
    ('server.configure',     'Configurar el servidor'),
    ('security.configure',   'Configurar seguridad'),
    ('database.manage',      'Gestionar base de datos');
-- ------------------------- Permisos por rol -------------------------
-- superadmin: todos
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p WHERE r.name = 'superadmin';

-- admin: todos excepto configuración crítica del servidor
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p
WHERE r.name = 'admin' AND p.code NOT IN ('server.configure', 'security.configure', 'database.manage');

-- integrator: hardware/automatización, sin administrar usuarios ni servidor
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p
WHERE r.name = 'integrator' AND p.code IN (
 'greenhouse.read','device.read','device.restart','device.configure',
 'sensor.read','sensor.configure','sensor.calibrate',
 'actuator.read','actuator.configure',
 'automation.read','automation.execute','automation.configure',
 'hardware.read','hardware.configure','modbus.read','modbus.configure',
 'network.read','network.configure','firmware.read','firmware.update','ota.execute',
 'diagnostics.execute','alarm.read','alarm.ack');

-- maintenance: diagnóstico y reemplazo, sin cambiar arquitectura
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p
WHERE r.name = 'maintenance' AND p.code IN (
 'greenhouse.read','device.read','device.restart','sensor.read','sensor.calibrate',
 'actuator.read','automation.read','hardware.read','modbus.read','network.read',
 'firmware.read','alarm.read','alarm.ack','diagnostics.execute');

-- supervisor: producción, reportes y consignas/horarios
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p
WHERE r.name = 'supervisor' AND p.code IN (
 'greenhouse.read','device.read','sensor.read','actuator.read','actuator.control',
 'automation.read','automation.execute','automation.configure',
 'alarm.read','alarm.ack','firmware.read','network.read');

-- operator: operación diaria
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p
WHERE r.name = 'operator' AND p.code IN (
 'greenhouse.read','device.read','sensor.read','actuator.read','actuator.control',
 'automation.read','automation.execute','alarm.read','alarm.ack','firmware.read');

-- auditor: solo lectura + logs
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p
WHERE r.name = 'auditor' AND p.code IN (
 'greenhouse.read','device.read','sensor.read','actuator.read','automation.read',
 'alarm.read','firmware.read','audit.read','user.read');

-- viewer: consulta básica
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p
WHERE r.name = 'viewer' AND p.code IN (
 'greenhouse.read','device.read','sensor.read','actuator.read','alarm.read','firmware.read','automation.read');

-- ------------------------- Usuario superadmin por defecto -------------------------
-- usuario "admin", contraseña "password" (cambiar en producción).
INSERT INTO users (username, email, full_name, password_hash, scope) VALUES
    ('admin', 'admin@example.com', 'Administrador',
     '$2y$10$92IXUNpkjO0rOQ5byMi.Ye4oKoEa3Ro9llC/.og/at2.uheWG/igi',
     '{"greenhouses": ["*"]}');

INSERT INTO user_roles (user_id, role_id)
SELECT u.id, r.id FROM users u, roles r WHERE u.username = 'admin' AND r.name = 'superadmin';

-- Invernadero de ejemplo
INSERT INTO greenhouses (name, type, timezone) VALUES
    ('Invernadero Principal', 'outdoor', 'America/Argentina/Buenos_Aires');

-- Firmware de referencia (manifest)
INSERT INTO firmware_versions
    (project, channel, version, hardware_profile, release_date, min_hardware, config_schema, protocol)
VALUES
    ('Invernadero', 'stable', '3.1.0', 'ESP32-GH-V1', '2026-09-28', 'rev0', 1, 1);
