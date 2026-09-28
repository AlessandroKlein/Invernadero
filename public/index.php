<?php
/**
 * Front controller del servidor central.
 * - Rutas /api/v1/*  -> API REST
 * - Cualquier otra   -> SPA (public/index.html)
 */
declare(strict_types=1);

require_once __DIR__ . '/../vendor/autoload.php';

use App\Api;

$path = parse_url($_SERVER['REQUEST_URI'] ?? '/', PHP_URL_PATH) ?? '/';

// CORS básico (ajustar en producción)
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, Authorization');
if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'OPTIONS') {
    http_response_code(204);
    exit;
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
