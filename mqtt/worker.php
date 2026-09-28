<?php
/**
 * Worker MQTT -> PostgreSQL.
 *
 * Se suscribe a `greenhouse/+/#` y persiste:
 *   greenhouse/{device_id}/state      -> estado, last_seen, shadow.reported
 *   greenhouse/{device_id}/sensors    -> sensores + lecturas
 *   greenhouse/{device_id}/actuators  -> actuadores + estados
 *   greenhouse/{device_id}/events     -> eventos
 *   greenhouse/{device_id}/alarms     -> alarmas
 *
 * Ejecutar:  php mqtt/worker.php
 */
declare(strict_types=1);

require __DIR__ . '/../vendor/autoload.php';

use App\Database;
use PhpMqtt\Client\MqttClient;
use PhpMqtt\Client\ConnectionSettings;

$cfg = require __DIR__ . '/../config/config.php';
$mqtt = $cfg['mqtt'];
$db = Database::get();

/** Registra o resuelve un dispositivo por device_id; devuelve su UUID. */
function ensureDevice(PDO $db, string $deviceId, ?string $uid): string
{
    $st = $db->prepare('SELECT id FROM devices WHERE device_id = ?');
    $st->execute([$deviceId]);
    if ($row = $st->fetch()) {
        return $row['id'];
    }
    $ins = $db->prepare(
        'INSERT INTO devices (device_id, device_uid, name) VALUES (?, ?, ?)
         ON CONFLICT (device_id) DO NOTHING RETURNING id'
    );
    $ins->execute([$deviceId, $uid ?: ('unknown-' . $deviceId), $deviceId]);
    if ($r = $ins->fetch()) return $r['id'];
    $st->execute([$deviceId]);
    return $st->fetch()['id'];
}

function updateState(PDO $db, string $devUuid, array $p): void
{
    $db->prepare(
        'UPDATE devices SET last_seen = now(),
            status = COALESCE(?, status),
            firmware_version = COALESCE(?, firmware_version)
         WHERE id = ?'
    )->execute([
        $p['state'] ?? $p['status'] ?? null,
        $p['version'] ?? $p['firmware'] ?? null,
        $devUuid,
    ]);
    $st = $db->prepare(
        'INSERT INTO device_shadow (device_id, reported) VALUES (?, ?)
         ON CONFLICT (device_id) DO UPDATE SET reported = EXCLUDED.reported, updated_at = now()'
    );
    $st->execute([$devUuid, json_encode($p)]);
}

function upsertSensor(PDO $db, string $devUuid, array $s): void
{
    $sid = $s['id'] ?? $s['sensor_id'] ?? null;
    if ($sid === null) return;
    $st = $db->prepare(
        'INSERT INTO sensors (device_id, sensor_id, name, type, interface, unit)
         VALUES (?, ?, ?, ?, ?, ?)
         ON CONFLICT (device_id, sensor_id) DO UPDATE SET name = EXCLUDED.name, unit = EXCLUDED.unit'
    );
    $st->execute([
        $devUuid, $sid,
        $s['name'] ?? $sid,
        $s['type'] ?? null,
        $s['interface'] ?? null,
        $s['unit'] ?? null,
    ]);
}

function ingestSensors(PDO $db, string $devUuid, array $p): void
{
    $items = $p['sensors'] ?? $p;
    if (isset($p['value'])) $items = [$p];

    foreach ($items as $s) {
        if (!is_array($s)) continue;
        upsertSensor($db, $devUuid, $s);
        $sid = $s['id'] ?? $s['sensor_id'] ?? null;
        if ($sid === null || !array_key_exists('value', $s)) continue;
        $st = $db->prepare('SELECT id FROM sensors WHERE device_id = ? AND sensor_id = ?');
        $st->execute([$devUuid, $sid]);
        if (!$sensor = $st->fetch()) continue;
        $db->prepare('INSERT INTO sensor_readings (sensor_id, value, unit, quality) VALUES (?, ?, ?, ?)')
            ->execute([$sensor['id'], (float) $s['value'], $s['unit'] ?? null, $s['quality'] ?? 'GOOD']);
    }
}

function ingestActuators(PDO $db, string $devUuid, array $p): void
{
    $items = $p['actuators'] ?? $p;
    if (!is_array($items)) return;

    foreach ($items as $a) {
        if (!is_array($a)) continue;
        $aid = $a['id'] ?? $a['actuator_id'] ?? null;
        if ($aid === null) continue;
        $st = $db->prepare(
            'INSERT INTO actuators (device_id, actuator_id, name, role, type)
             VALUES (?, ?, ?, ?, ?)
             ON CONFLICT (device_id, actuator_id) DO UPDATE SET name = EXCLUDED.name'
        );
        $st->execute([$devUuid, $aid, $a['name'] ?? $aid, $a['role'] ?? null, $a['type'] ?? null]);

        $state = $a['state'] ?? null;
        if ($state === null && !array_key_exists('output', $a)) continue;
        $st = $db->prepare('SELECT id FROM actuators WHERE device_id = ? AND actuator_id = ?');
        $st->execute([$devUuid, $aid]);
        if (!$act = $st->fetch()) continue;
        $db->prepare('INSERT INTO actuator_states (actuator_id, state, output) VALUES (?, ?, ?)')
            ->execute([
                $act['id'],
                (bool) ($state ?? false),
                (int) ($a['output'] ?? ($state ? 100 : 0)),
            ]);
    }
}

function insertEvent(PDO $db, string $devUuid, array $p): void
{
    $db->prepare('INSERT INTO events (device_id, type, message, payload) VALUES (?, ?, ?, ?)')
        ->execute([$devUuid, $p['type'] ?? 'event', $p['message'] ?? null, json_encode($p)]);
}

function insertAlarm(PDO $db, string $devUuid, array $p): void
{
    $db->prepare(
        'INSERT INTO alarms (device_id, type, severity, status, message, source) VALUES (?, ?, ?, ?, ?, ?)'
    )->execute([
        $devUuid,
        $p['type'] ?? 'ALARM',
        $p['severity'] ?? 'WARNING',
        $p['status'] ?? 'ACTIVE',
        $p['message'] ?? null,
        'DEVICE',
    ]);
}

function handleMessage(PDO $db, string $topic, string $message): void
{
    $parts = explode('/', $topic);
    if (count($parts) < 3) return;
    $deviceId = $parts[1];
    $type = $parts[2];
    $payload = json_decode($message, true);
    if (!is_array($payload)) $payload = ['raw' => $message];

    $devUuid = ensureDevice($db, $deviceId, $payload['uid'] ?? $payload['device_uid'] ?? null);

    switch ($type) {
        case 'state':     updateState($db, $devUuid, $payload); break;
        case 'sensors':   ingestSensors($db, $devUuid, $payload); break;
        case 'actuators': ingestActuators($db, $devUuid, $payload); break;
        case 'events':    insertEvent($db, $devUuid, $payload); break;
        case 'alarms':    insertAlarm($db, $devUuid, $payload); break;
    }
}

$client = new MqttClient($mqtt['host'], $mqtt['port'], 'invernadero-server-' . getmypid());
$settings = (new ConnectionSettings())->setConnectTimeout(5);
if ($mqtt['user'] !== '') {
    $settings->setUsername($mqtt['user'])->setPassword($mqtt['pass']);
}

$client->connect($settings, true);
$client->subscribe('greenhouse/+/#', function (string $topic, string $message, bool $retained = false) use ($db): void {
    handleMessage($db, $topic, $message);
}, 0);

echo '[MQTT worker] escuchando greenhouse/+/# en ' . $mqtt['host'] . ':' . $mqtt['port'] . PHP_EOL;
$client->loop(true, 0.1);

