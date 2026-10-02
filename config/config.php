<?php
/**
 * Configuración del servidor central.
 *
 * Carga valores desde variables de entorno (docker-compose) y, en desarrollo
 * local, desde un archivo `.env` mediante vlucas/phpdotenv.
 *
 * @return array Configuración (db, mqtt, jwt).
 */
declare(strict_types=1);

$autoload = __DIR__ . '/../vendor/autoload.php';
if (file_exists($autoload)) {
    require_once $autoload;
    if (class_exists(\Dotenv\Dotenv::class) && file_exists(__DIR__ . '/../.env')) {
        \Dotenv\Dotenv::createImmutable(__DIR__ . '/../')->load();
    }
}

return [
    'db' => [
        'host' => getenv('DB_HOST') ?: 'localhost',
        'port' => getenv('DB_PORT') ?: '5432',
        'name' => getenv('DB_NAME') ?: 'invernadero',
        'user' => getenv('DB_USER') ?: 'invernadero',
        'pass' => getenv('DB_PASS') ?: 'invernadero',
    ],
    'mqtt' => [
        'host'    => getenv('MQTT_HOST') ?: 'localhost',
        'port'    => (int) (getenv('MQTT_PORT') ?: 1883),
        'ws_port' => (int) (getenv('MQTT_WS_PORT') ?: 9001),
        'user'    => getenv('MQTT_USER') ?: '',
        'pass'    => getenv('MQTT_PASS') ?: '',
    ],
    'jwt' => [
        'secret' => getenv('JWT_SECRET') ?: 'cambiar-esta-clave-secreta',
        'ttl'    => (int) (getenv('JWT_TTL') ?: 86400),
    ],
    'cors' => [
        'origin' => getenv('CORS_ORIGIN') ?: '*',
    ],
    'rate_limit' => [
        'max_requests'  => (int) (getenv('RATE_LIMIT_MAX') ?: 120),
        'window_seconds' => (int) (getenv('RATE_LIMIT_WINDOW') ?: 60),
    ],
];
