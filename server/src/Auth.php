<?php
/**
 * Autenticación JWT (emisión y verificación de tokens).
 */
declare(strict_types=1);

namespace App;

use Firebase\JWT\JWT;
use Firebase\JWT\Key;

final class Auth
{
    private static function cfg(): array
    {
        return require __DIR__ . '/../config/config.php';
    }

    /** Emite un JWT para un usuario. */
    public static function issue(array $payload): string
    {
        $cfg = self::cfg();
        $now = time();
        $payload = array_merge([
            'iat' => $now,
            'exp' => $now + (int) $cfg['jwt']['ttl'],
        ], $payload);
        return JWT::encode($payload, $cfg['jwt']['secret'], 'HS256');
    }

    /** Verifica el Bearer token y devuelve el payload, o null si es inválido. */
    public static function verify(?string $token): ?array
    {
        if (!$token) {
            return null;
        }
        $cfg = self::cfg();
        try {
            return (array) JWT::decode($token, new Key($cfg['jwt']['secret'], 'HS256'));
        } catch (\Throwable) {
            return null;
        }
    }

    /** Extrae el token del header Authorization. */
    public static function bearer(): ?string
    {
        $h = $_SERVER['HTTP_AUTHORIZATION'] ?? '';
        if (preg_match('/Bearer\s+(\S+)/i', $h, $m)) {
            return $m[1];
        }
        return null;
    }
}
