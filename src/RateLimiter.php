<?php
/**
 * Rate limiting por IP (ventana deslizante simple, basado en archivos).
 * Self-contained: no requiere extensiones (APCu/Redis) y funciona en Docker.
 */
declare(strict_types=1);

namespace App;

final class RateLimiter
{
    /**
     * Permite como máximo $maxRequests por $windowSeconds por IP.
     * Envía 429 y termina si se supera el límite.
     */
    public static function check(int $maxRequests, int $windowSeconds): void
    {
        $ip = $_SERVER['REMOTE_ADDR'] ?? 'unknown';
        $key = sys_get_temp_dir() . '/rl_' . md5($ip);

        $now = microtime(true);
        $data = ['window' => 0.0, 'count' => 0];
        if (is_file($key)) {
            $decoded = json_decode((string) file_get_contents($key), true);
            if (is_array($decoded)) {
                $data = $decoded;
            }
        }

        // Ventana deslizante: resetear si pasó más de windowSeconds.
        if ($now - (float) ($data['window'] ?? 0.0) > $windowSeconds) {
            $data = ['window' => $now, 'count' => 1];
        } else {
            $data['count'] = (int) ($data['count'] ?? 0) + 1;
        }

        file_put_contents($key, json_encode($data), LOCK_EX);

        if ($data['count'] > $maxRequests) {
            http_response_code(429);
            header('Content-Type: application/json');
            echo json_encode(['error' => 'Demasiadas solicitudes, intente más tarde']);
            exit;
        }
    }
}
