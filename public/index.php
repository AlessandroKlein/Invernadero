<?php
/**
 * Front controller del servidor central.
 * - Rutas /api/v1/*  -> API REST
 * - Cualquier otra   -> SPA (public/index.html)
 */
declare(strict_types=1);

require_once __DIR__ . '/../vendor/autoload.php';

use App\Api;
use App\RateLimiter;

$config = require __DIR__ . '/../config/config.php';

$path = parse_url($_SERVER['REQUEST_URI'] ?? '/', PHP_URL_PATH) ?? '/';

// CORS configurable (CORS_ORIGIN en .env; '*' por defecto).
$origin = $config['cors']['origin'] ?? '*';
header('Access-Control-Allow-Origin: ' . $origin);
header('Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, Authorization');
header('Access-Control-Max-Age: 86400');
header('Vary: Origin');
if ($origin !== '*') {
    header('Access-Control-Allow-Credentials: true');
}
if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'OPTIONS') {
    http_response_code(204);
    exit;
}

// Rate limiting solo para la API.
if (str_starts_with($path, '/api/')) {
    RateLimiter::check(
        (int) ($config['rate_limit']['max_requests'] ?? 120),
        (int) ($config['rate_limit']['window_seconds'] ?? 60)
    );
}

if (str_starts_with($path, '/api/')) {
    Api::run($path, $_SERVER['REQUEST_METHOD'] ?? 'GET');
}

// Servir la SPA
if ($path === '/' || $path === '/index.html') {
    header('Content-Type: text/html; charset=utf-8');
    readfile(__DIR__ . '/index.html');
    exit;
}

http_response_code(404);
echo 'Not Found';
