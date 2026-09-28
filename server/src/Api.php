<?php
/**
 * Enrutador y controladores de la API REST del servidor central.
 */
declare(strict_types=1);

namespace App;

use PDO;

final class Api
{
    public static function run(string $path, string $method): never
    {
        // /api/v1/... -> segmentos
        $seg = array_values(array_filter(explode('/', substr($path, strlen('/api/v1')))));
        $route = $seg[0] ?? '';

        try {
            if ($route === 'auth' && $method === 'POST') {
                self::login();
            }
            if ($route === 'status' && $method === 'GET') {
                Response::json(['status' => 'ok', 'time' => gmdate('c')]);
            }

            // El resto requiere JWT válido
            $user = Auth::verify(Auth::bearer());
            if ($user === null) {
                Response::error('No autorizado', 401);
            }

            switch ($route) {
                case 'greenhouses':
                    self::greenhouses($seg[1] ?? null, $method);
                case 'devices':
                    self::devices($seg, $method);
                case 'firmware':
                    self::firmware();
                case 'events':
                    self::events($seg[1] ?? null);
                default:
                    Response::error('Ruta no encontrada', 404);
            }
        } catch (\Throwable $e) {
            Response::error('Error interno: ' . $e->getMessage(), 500);
        }
    }

    // ---------- Autenticación ----------
    private static function login(): never
    {
        $in = json_decode(file_get_contents('php://input') ?: '[]', true) ?: [];
        $username = $in['username'] ?? '';
        $password = $in['password'] ?? '';

        $db = Database::get();
        $st = $db->prepare('SELECT id, username, password_hash, role_id FROM users WHERE username = ? AND active = TRUE');
        $st->execute([$username]);
        $u = $st->fetch();

        if (!$u || !password_verify($password, $u['password_hash'])) {
            Response::error('Credenciales inválidas', 401);
        }

        $token = Auth::issue([
            'sub' => $u['id'],
            'username' => $u['username'],
            'role_id' => (int) $u['role_id'],
        ]);
        Response::json(['token' => $token, 'username' => $u['username'], 'role_id' => (int) $u['role_id']]);
    }

    // ---------- Invernaderos ----------
    private static function greenhouses(?string $id, string $method): never
    {
        $db = Database::get();
        if ($method === 'GET' && $id === null) {
            $rows = $db->query('SELECT * FROM greenhouses ORDER BY name')->fetchAll();
            Response::json($rows);
        }
        if ($method === 'POST' && $id === null) {
            $in = json_decode(file_get_contents('php://input') ?: '[]', true) ?: [];
            $st = $db->prepare('INSERT INTO greenhouses (name, type, timezone) VALUES (?, ?, ?) RETURNING *');
            $st->execute([
                $in['name'] ?? 'Sin nombre',
                $in['type'] ?? 'outdoor',
                $in['timezone'] ?? 'America/Argentina/Buenos_Aires',
            ]);
            Response::json($st->fetch(), 201);
        }
        if ($method === 'GET' && $id !== null) {
            $st = $db->prepare('SELECT * FROM greenhouses WHERE id = ?');
            $st->execute([$id]);
            $row = $st->fetch();
            if (!$row) Response::error('No encontrado', 404);
            Response::json($row);
        }
        Response::error('Operación no soportada', 405);
    }

    // ---------- Dispositivos ----------
    private static function devices(array $seg, string $method): never
    {
        $db = Database::get();
        $id = $seg[1] ?? null;

        if ($method === 'GET' && $id === null) {
            $rows = $db->query('SELECT * FROM devices ORDER BY device_id')->fetchAll();
            Response::json($rows);
        }

        if ($id === null) {
            Response::error('Falta id de dispositivo', 400);
        }

        $dev = self::findDevice($id);
        if (!$dev) Response::error('Dispositivo no encontrado', 404);

        $sub = $seg[2] ?? null;
        if ($method === 'GET' && $sub === null) {
            Response::json($dev);
        }

        self::deviceSub($dev, $sub, $seg, $method);
    }

    private static function findDevice(string $idOrDeviceId): ?array
    {
        $db = Database::get();
        $st = $db->prepare('SELECT * FROM devices WHERE id = ? OR device_id = ? LIMIT 1');
        $st->execute([$idOrDeviceId, $idOrDeviceId]);
        $row = $st->fetch();
        return $row ?: null;
    }

    private static function deviceSub(array $dev, string $sub, array $seg, string $method): never
    {
        $db = Database::get();
        $did = $dev['id'];

        switch ($sub) {
            case 'sensors':
                $st = $db->prepare('SELECT * FROM sensors WHERE device_id = ? ORDER BY sensor_id');
                $st->execute([$did]);
                Response::json($st->fetchAll());

            case 'actuators':
                $st = $db->prepare('SELECT * FROM actuators WHERE device_id = ? ORDER BY actuator_id');
                $st->execute([$did]);
                Response::json($st->fetchAll());

            case 'readings':
                if ($method === 'POST') {
                    self::ingestReadings($did);
                }
                self::readings($did, $seg[3] ?? null);

            case 'alarms':
                $st = $db->prepare('SELECT * FROM alarms WHERE device_id = ? ORDER BY created_at DESC LIMIT 200');
                $st->execute([$did]);
                Response::json($st->fetchAll());

            case 'shadow':
                if ($method === 'PUT') {
                    $in = json_decode(file_get_contents('php://input') ?: '[]', true) ?: [];
                    $st = $db->prepare(
                        'INSERT INTO device_shadow (device_id, desired, reported) VALUES (?, ?, ?)
                         ON CONFLICT (device_id) DO UPDATE SET desired = EXCLUDED.desired, reported = EXCLUDED.reported, updated_at = now()
                         RETURNING *'
                    );
                    $st->execute([$did, json_encode($in['desired'] ?? new \stdClass()), json_encode($in['reported'] ?? new \stdClass())]);
                    Response::json($st->fetch());
                }
                $st = $db->prepare('SELECT * FROM device_shadow WHERE device_id = ?');
                $st->execute([$did]);
                $row = $st->fetch();
                Response::json($row ?: ['desired' => new \stdClass(), 'reported' => new \stdClass()]);

            case 'config':
                if ($method === 'POST') {
                    $in = json_decode(file_get_contents('php://input') ?: '[]', true) ?: [];
                    $version = (int) ($in['config_version'] ?? 1);
                    $st = $db->prepare(
                        'INSERT INTO configurations (device_id, config_version, source, config, applied)
                         VALUES (?, ?, ?, ?, TRUE)
                         ON CONFLICT (device_id, config_version) DO UPDATE SET config = EXCLUDED.config, applied = TRUE
                         RETURNING *'
                    );
                    $st->execute([$did, $version, 'CENTRAL', json_encode($in['config'] ?? new \stdClass())]);
                    Response::json($st->fetch(), 201);
                }
                $st = $db->prepare('SELECT * FROM configurations WHERE device_id = ? ORDER BY config_version DESC LIMIT 50');
                $st->execute([$did]);
                Response::json($st->fetchAll());

            default:
                Response::error('Sub-recurso no encontrado', 404);
        }
    }

    private static function ingestReadings(string $did): never
    {
        $db = Database::get();
        $in = json_decode(file_get_contents('php://input') ?: '[]', true) ?: [];
        $items = $in['readings'] ?? (isset($in['sensor_id']) ? [$in] : []);
        $count = 0;

        foreach ($items as $r) {
            $sid = $r['sensor_id'] ?? null;
            if ($sid === null) continue;
            $st = $db->prepare('SELECT id FROM sensors WHERE device_id = ? AND sensor_id = ?');
            $st->execute([$did, $sid]);
            $sensor = $st->fetch();
            if (!$sensor) continue;
            $ins = $db->prepare('INSERT INTO sensor_readings (sensor_id, value, unit, quality) VALUES (?, ?, ?, ?)');
            $ins->execute([$sensor['id'], (float) ($r['value'] ?? 0), $r['unit'] ?? null, $r['quality'] ?? 'GOOD']);
            $count++;
        }
        Response::json(['ingested' => $count]);
    }

    private static function readings(string $did, ?string $sensorId): never
    {
        $db = Database::get();
        $limit = min(1000, (int) ($_GET['limit'] ?? 500));
        $from  = $_GET['from'] ?? null;
        $to    = $_GET['to'] ?? null;

        $sql = 'SELECT r.ts, r.value, r.unit, r.quality, s.sensor_id, s.name
                FROM sensor_readings r JOIN sensors s ON s.id = r.sensor_id
                WHERE s.device_id = :did';
        $params = ['did' => $did];

        if ($sensorId) {
            $sql .= ' AND s.sensor_id = :sid';
            $params['sid'] = $sensorId;
        }
        if ($from) {
            $sql .= ' AND r.ts >= :from';
            $params['from'] = $from;
        }
        if ($to) {
            $sql .= ' AND r.ts <= :to';
            $params['to'] = $to;
        }
        $sql .= ' ORDER BY r.ts DESC LIMIT ' . $limit;

        $st = $db->prepare($sql);
        $st->execute($params);
        Response::json(array_reverse($st->fetchAll()));
    }

    private static function firmware(): never
    {
        $db = Database::get();
        $rows = $db->query('SELECT * FROM firmware_versions ORDER BY release_date DESC, version DESC')->fetchAll();
        Response::json($rows);
    }

    private static function events(?string $deviceId): never
    {
        $db = Database::get();
        if ($deviceId) {
            $st = $db->prepare(
                'SELECT e.* FROM events e JOIN devices d ON d.id = e.device_id
                 WHERE d.device_id = ? ORDER BY e.created_at DESC LIMIT 200'
            );
            $st->execute([$deviceId]);
            Response::json($st->fetchAll());
        }
        $rows = $db->query('SELECT * FROM events ORDER BY created_at DESC LIMIT 200')->fetchAll();
        Response::json($rows);
    }
}
