<?php
/**
 * Conexión PDO a PostgreSQL (singleton).
 */
declare(strict_types=1);

namespace App;

use PDO;

final class Database
{
    private static ?PDO $pdo = null;

    public static function get(): PDO
    {
        if (self::$pdo === null) {
            $cfg  = require __DIR__ . '/../config/config.php';
            $dsn  = sprintf(
                'pgsql:host=%s;port=%s;dbname=%s',
                $cfg['db']['host'],
                $cfg['db']['port'],
                $cfg['db']['name']
            );
            self::$pdo = new PDO($dsn, $cfg['db']['user'], $cfg['db']['pass'], [
                PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
                PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
                PDO::ATTR_EMULATE_PREPARES   => false,
            ]);
        }
        return self::$pdo;
    }
}
