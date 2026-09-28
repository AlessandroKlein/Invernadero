<?php
/**
 * RBAC: definición y verificación de permisos y alcance (scope).
 */
declare(strict_types=1);

namespace App;

final class Permissions
{
    /** Lista maestra de permisos (coincide con el seed `db/init/004_seed.sql`). */
    public const ALL = [
        'greenhouse.read', 'greenhouse.create', 'greenhouse.edit', 'greenhouse.delete',
        'device.read', 'device.control', 'device.restart', 'device.configure', 'device.delete',
        'sensor.read', 'sensor.configure', 'sensor.calibrate',
        'actuator.read', 'actuator.control', 'actuator.configure',
        'automation.read', 'automation.execute', 'automation.configure',
        'hardware.read', 'hardware.configure',
        'modbus.read', 'modbus.configure',
        'network.read', 'network.configure',
        'firmware.read', 'firmware.update', 'ota.execute',
        'alarm.read', 'alarm.ack',
        'user.read', 'user.create', 'user.edit', 'user.delete',
        'audit.read', 'diagnostics.execute',
        'server.configure', 'security.configure', 'database.manage',
    ];

    /** ¿El payload JWT posee el permiso indicado? */
    public static function has(array $auth, string $perm): bool
    {
        $perms = $auth['perms'] ?? [];
        return in_array($perm, $perms, true) || in_array('*', $perms, true);
    }

    /** Verifica un permiso y responde 403 si falta. */
    public static function require(array $auth, string $perm): void
    {
        if (!self::has($auth, $perm)) {
            Response::error('Permiso denegado: ' . $perm, 403);
        }
    }

    /** ¿El usuario tiene alcance global (\"*\")? */
    public static function globalScope(array $auth): bool
    {
        $scope = $auth['scope'] ?? [];
        $gh = $scope['greenhouses'] ?? [];
        return in_array('*', $gh, true);
    }

    /** Nombres/ids de invernaderos permitidos por el scope (['*'] = todos). */
    public static function scopedGreenhouses(array $auth): array
    {
        return $auth['scope']['greenhouses'] ?? ['*'];
    }
}
