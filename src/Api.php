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
            // Rutas públicas
            if ($route === 'auth' && ($seg[1] ?? 'login') === 'login' && $method === 'POST') {
                self::login();
            }
            if ($route === 'status' && $method === 'GET') {
                Response::json(['status' => 'ok', 'time' => gmdate('c')]);
            }

            // El resto requiere JWT válido
            $auth = Auth::verify(Auth::bearer());
            if ($auth === null) {
                Response::error('No autorizado', 401);
            }

            switch ($route) {
                case 'auth':
                    if (($seg[1] ?? '') === 'me' && $method === 'GET') self::me($auth);
                    Response::error('Ruta no encontrada', 404);
                case 'roles':
                    Permissions::require($auth, 'user.read');
                    self::roles();
                case 'permissions':
                    Permissions::require($auth, 'user.read');
                    self::permissions();
                case 'users':
                    self::users($seg[1] ?? null, $method, $auth);
                case 'greenhouses':
                    self::greenhouses($seg[1] ?? null, $method, $auth);
                case 'devices':
                    self::devices($seg, $method, $auth);
                case 'firmware':
                    self::firmware($seg[1] ?? null, $method, $auth);
                case 'events':
                    Permissions::require($auth, 'device.read');
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
        $st = $db->prepare('SELECT * FROM users WHERE username = ? AND active = TRUE');
        $st->execute([$username]);
        $u = $st->fetch();

        if (!$u || !password_verify($password, $u['password_hash'])) {
            Response::error('Credenciales inválidas', 401);
        }

        $ident = Auth::identityFor($u['id']);
        $scope = json_decode($u['scope'], true) ?: ['greenhouses' => ['*']];

        $db->prepare('UPDATE users SET last_login = now() WHERE id = ?')->execute([$u['id']]);

        $token = Auth::issue([
            'sub' => $u['id'],
            'username' => $u['username'],
            'full_name' => $u['full_name'],
            'roles' => $ident['roles'],
            'perms' => $ident['perms'],
            'scope' => $scope,
        ]);
        Response::json([
            'token' => $token,
            'user' => [
                'id' => $u['id'],
                'username' => $u['username'],
                'full_name' => $u['full_name'],
                'roles' => $ident['roles'],
                'perms' => $ident['perms'],
                'scope' => $scope,
            ],
        ]);
    }

    private static function me(array $auth): never
    {
        $db = Database::get();
        $st = $db->prepare('SELECT id, username, email, full_name, active, scope, created_at, last_login FROM users WHERE id = ?');
        $st->execute([$auth['sub'] ?? null]);
        $u = $st->fetch();
        if (!$u) Response::error('Usuario no encontrado', 404);
        $u['roles'] = $auth['roles'] ?? [];
        $u['perms'] = $auth['perms'] ?? [];
        Response::json($u);
    }

    // ---------- RBAC: roles y permisos ----------
    private static function roles(): never
    {
        $db = Database::get();
        Response::json($db->query('SELECT * FROM roles ORDER BY name')->fetchAll());
    }

    private static function permissions(): never
    {
        $db = Database::get();
        $perms = $db->query('SELECT * FROM permissions ORDER BY code')->fetchAll();
        $map = [];
        foreach ($db->query(
            'SELECT rp.role_id, p.code FROM role_permissions rp JOIN permissions p ON p.id = rp.permission_id'
        )->fetchAll() as $row) {
            $map[$row['role_id']][] = $row['code'];
        }
        Response::json(['permissions' => $perms, 'role_permissions' => $map]);
    }

    // ---------- Gestión de usuarios (RBAC) ----------
    private static function users(?string $id, string $method, array $auth): never
    {
        if ($id === null) {
            if ($method === 'GET') {
                Permissions::require($auth, 'user.read');
                self::listUsers();
            }
            if ($method === 'POST') {
                Permissions::require($auth, 'user.create');
                self::createUser($auth);
            }
            Response::error('Operación no soportada', 405);
        }
        Permissions::require($auth, $method === 'DELETE' ? 'user.delete' : 'user.edit');
        self::mutateUser($id, $method, $auth);
    }

    private static function listUsers(): never
    {
        $db = Database::get();
        $rows = $db->query('SELECT * FROM users ORDER BY username')->fetchAll();
        foreach ($rows as &$r) {
            $st = $db->prepare('SELECT r.name FROM roles r JOIN user_roles ur ON ur.role_id = r.id WHERE ur.user_id = ? ORDER BY r.name');
            $st->execute([$r['id']]);
            $r['roles'] = array_column($st->fetchAll(), 'name');
        }
        unset($r);
        Response::json($rows);
    }

    private static function createUser(array $auth): never
    {
        $db = Database::get();
        $in = json_decode(file_get_contents('php://input') ?: '[]', true) ?: [];
        $username = trim($in['username'] ?? '');
        $password = $in['password'] ?? '';
        if ($username === '' || strlen($password) < 6) {
            Response::error('Usuario requerido y contraseña de al menos 6 caracteres', 400);
        }
        self::guardSuperadmin($in['roles'] ?? [], $auth);

        $hash = password_hash($password, PASSWORD_BCRYPT);
        $scope = json_encode($in['scope'] ?? ['greenhouses' => ['*']]);
        $st = $db->prepare(
            'INSERT INTO users (username, email, full_name, password_hash, active, scope)
             VALUES (?, ?, ?, ?, ?, ?) RETURNING id'
        );
        try {
            $st->execute([
                $username,
                $in['email'] ?? null,
                $in['full_name'] ?? null,
                $hash,
                (bool) ($in['active'] ?? true),
                $scope,
            ]);
        } catch (\Throwable $e) {
            Response::error('No se pudo crear el usuario (¿ya existe?): ' . $e->getMessage(), 409);
        }
        $id = $st->fetch()['id'];
        self::setRoles($id, $in['roles'] ?? []);
        Response::json(['id' => $id, 'username' => $username], 201);
    }

    private static function mutateUser(string $id, string $method, array $auth): never
    {
        $db = Database::get();
        $st = $db->prepare('SELECT * FROM users WHERE id = ? OR username = ?');
        $st->execute([$id, $id]);
        $u = $st->fetch();
        if (!$u) Response::error('Usuario no encontrado', 404);

        if ($method === 'DELETE') {
            if ($u['id'] === ($auth['sub'] ?? '')) {
                Response::error('No podés eliminar tu propio usuario', 400);
            }
            $db->prepare('DELETE FROM users WHERE id = ?')->execute([$u['id']]);
            Response::json(['deleted' => true]);
        }

        $in = json_decode(file_get_contents('php://input') ?: '[]', true) ?: [];
        if (array_key_exists('roles', $in)) {
            self::guardSuperadmin($in['roles'], $auth);
        }

        $fields = [];
        $params = [];
        foreach (['email', 'full_name', 'username'] as $f) {
            if (array_key_exists($f, $in)) { $fields[] = "$f = ?"; $params[] = $in[$f]; }
        }
        if (array_key_exists('active', $in)) { $fields[] = 'active = ?'; $params[] = (bool) $in['active']; }
        if (array_key_exists('scope', $in)) { $fields[] = 'scope = ?'; $params[] = json_encode($in['scope']); }
        if (!empty($in['password'])) { $fields[] = 'password_hash = ?'; $params[] = password_hash($in['password'], PASSWORD_BCRYPT); }

        if ($fields) {
            $params[] = $u['id'];
            $db->prepare('UPDATE users SET ' . implode(', ', $fields) . ' WHERE id = ?')->execute($params);
        }
        if (array_key_exists('roles', $in)) {
            self::setRoles($u['id'], $in['roles']);
        }
        Response::json(['updated' => true, 'id' => $u['id']]);
    }

    /** Solo quien tiene 'server.configure' (superadmin) puede asignar el rol superadmin. */
    private static function guardSuperadmin(array $roles, array $auth): void
    {
        if (in_array('superadmin', $roles, true) && !Permissions::has($auth, 'server.configure')) {
            Response::error('Solo un superadmin puede asignar el rol superadmin', 403);
        }
    }

    private static function setRoles(string $userId, array $roleNames): void
    {
        $db = Database::get();
        $db->prepare('DELETE FROM user_roles WHERE user_id = ?')->execute([$userId]);
        $st = $db->prepare('SELECT id FROM roles WHERE name = ?');
        $ins = $db->prepare('INSERT INTO user_roles (user_id, role_id) VALUES (?, ?)');
        foreach ($roleNames as $name) {
            $st->execute([$name]);
            $r = $st->fetch();
            if ($r) $ins->execute([$userId, $r['id']]);
        }
    }

    // ---------- Invernaderos ----------
    private static function greenhouses(?string $id, string $method, array $auth): never
    {
        Permissions::require($auth, $method === 'POST' ? 'greenhouse.create' : 'greenhouse.read');
        $db = Database::get();
        if ($method === 'GET' && $id === null) {
            $rows = $db->query('SELECT * FROM greenhouses ORDER BY name')->fetchAll();
            if (!Permissions::globalScope($auth)) {
                $allowed = Permissions::scopedGreenhouses($auth);
                $rows = array_values(array_filter($rows, fn($g) => in_array($g['name'], $allowed, true) || in_array($g['id'], $allowed, true)));
            }
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
    private static function devices(array $seg, string $method, array $auth): never
    {
        Permissions::require($auth, $method === 'GET' ? 'device.read' : 'device.configure');
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

        self::deviceSub($dev, $sub, $seg, $method, $auth);
    }

    private static function findDevice(string $idOrDeviceId): ?array
    {
        $db = Database::get();
        $st = $db->prepare('SELECT * FROM devices WHERE id = ? OR device_id = ? LIMIT 1');
        $st->execute([$idOrDeviceId, $idOrDeviceId]);
        $row = $st->fetch();
        return $row ?: null;
    }

    private static function deviceSub(array $dev, string $sub, array $seg, string $method, array $auth): never
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

            case 'ota':
                if ($method === 'POST') {
                    Permissions::require($auth, 'ota.execute');
                    self::triggerOta($dev);
                }
                $st = $db->prepare('SELECT * FROM ota_jobs WHERE device_id = ? ORDER BY created_at DESC LIMIT 50');
                $st->execute([$did]);
                Response::json($st->fetchAll());

            default:
                Response::error('Sub-recurso no encontrado', 404);
        }
    }

    private static function triggerOta(array $dev): never
    {
        $in = json_decode(file_get_contents('php://input') ?: '[]', true) ?: [];
        $version = trim($in['version'] ?? '');
        if ($version === '') {
            Response::error('Versión requerida', 400);
        }

        $db = Database::get();
        $st = $db->prepare(
            'SELECT * FROM firmware_versions
             WHERE version = ? AND (hardware_profile = ? OR hardware_profile IS NULL)
             ORDER BY release_date DESC LIMIT 1'
        );
        $st->execute([$version, $dev['hardware_profile'] ?? 'ESP32-GH-V1']);
        $fw = $st->fetch();
        if (!$fw) {
            Response::error('Firmware no encontrado', 404);
        }

        $job = $db->prepare('INSERT INTO ota_jobs (firmware_id, device_id, status) VALUES (?, ?, ?) RETURNING id');
        $job->execute([$fw['id'], $dev['id'], 'PENDING']);
        $jobId = $job->fetch()['id'];

        $ok = Mqtt::publish('greenhouse/' . $dev['device_id'] . '/cmd', [
            'type' => 'ota',
            'version' => $fw['version'],
            'url' => $fw['url'],
            'sha256' => $fw['sha256'],
            'job_id' => $jobId,
        ]);

        $db->prepare('UPDATE ota_jobs SET status = ?, result = ? WHERE id = ?')
            ->execute([$ok ? 'SENT' : 'MQTT_ERROR', $ok ? null : 'No se pudo publicar el comando MQTT', $jobId]);

        Response::json(['ok' => $ok, 'job_id' => $jobId, 'status' => $ok ? 'SENT' : 'MQTT_ERROR']);
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
            $seq = isset($r['sequence']) ? (int) $r['sequence'] : null;
            $ins = $db->prepare('INSERT INTO sensor_readings (sensor_id, value, unit, quality, sequence) VALUES (?, ?, ?, ?, ?)');
            $ins->execute([$sensor['id'], (float) ($r['value'] ?? 0), $r['unit'] ?? null, $r['quality'] ?? 'GOOD', $seq]);
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
        $afterSeq = $_GET['after_sequence'] ?? null;

        $sql = 'SELECT r.ts, r.value, r.unit, r.quality, r.sequence, s.sensor_id, s.name
                FROM sensor_readings r JOIN sensors s ON s.id = r.sensor_id
                WHERE s.device_id = :did';
        $params = ['did' => $did];

        if ($sensorId) {
            $sql .= ' AND s.sensor_id = :sid';
            $params['sid'] = $sensorId;
        }
        if ($afterSeq !== null && $afterSeq !== '') {
            $sql .= ' AND r.sequence > :aseq';
            $params['aseq'] = (int) $afterSeq;
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

    private static function firmware(?string $sub, string $method, array $auth): never
    {
        if ($sub === 'upload' && $method === 'POST') {
            Permissions::require($auth, 'firmware.update');
            self::firmwareUpload();
        }
        Permissions::require($auth, 'firmware.read');
        self::firmwareList();
    }

    private static function firmwareList(): never
    {
        $db = Database::get();
        $rows = $db->query('SELECT * FROM firmware_versions ORDER BY release_date DESC, version DESC')->fetchAll();
        Response::json($rows);
    }

    private static function firmwareUpload(): never
    {
        $file = $_FILES['file'] ?? null;
        $version = trim($_POST['version'] ?? '');
        $channel = $_POST['channel'] ?? 'stable';
        $hardwareProfile = $_POST['hardware_profile'] ?? 'ESP32-GH-V1';

        if ($version === '' || $file === null || ($file['error'] ?? UPLOAD_ERR_NO_FILE) !== UPLOAD_ERR_OK) {
            Response::error('Archivo (.bin) y versión requeridos', 400);
        }
        // Sanitizar el nombre de versión: solo [A-Za-z0-9._-].
        if (!preg_match('/^[A-Za-z0-9._-]+$/', $version)) {
            Response::error('Versión inválida (solo letras, números, . _ -)', 400);
        }

        $dir = __DIR__ . '/../public/firmware';
        if (!is_dir($dir)) {
            mkdir($dir, 0775, true);
        }
        $filename = $version . '.bin';
        $dest = $dir . '/' . $filename;

        if (!move_uploaded_file($file['tmp_name'], $dest)) {
            Response::error('No se pudo guardar el firmware', 500);
        }

        $sha = strtoupper(hash_file('sha256', $dest));
        $scheme = $_SERVER['REQUEST_SCHEME'] ?? 'http';
        $host = $_SERVER['HTTP_HOST'] ?? 'localhost';
        $url = $scheme . '://' . $host . '/firmware/' . $filename;

        $db = Database::get();
        $st = $db->prepare(
            'INSERT INTO firmware_versions
                (project, channel, version, hardware_profile, url, sha256, release_date)
             VALUES (?, ?, ?, ?, ?, ?, CURRENT_DATE)
             ON CONFLICT (channel, version, hardware_profile) DO UPDATE
                SET url = EXCLUDED.url, sha256 = EXCLUDED.sha256, release_date = CURRENT_DATE
             RETURNING *'
        );
        $st->execute(['Invernadero', $channel, $version, $hardwareProfile, $url, $sha]);

        Response::json($st->fetch(), 201);
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
