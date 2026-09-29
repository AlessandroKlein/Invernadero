<?php
/**
 * Publicador MQTT del servidor central.
 * Permite enviar comandos a los ESP32 (p. ej. OTA) desde la API REST.
 */
declare(strict_types=1);

namespace App;

use PhpMqtt\Client\MqttClient;
use PhpMqtt\Client\ConnectionSettings;

final class Mqtt
{
    /** Publica un payload JSON en un tópico. Retorna true si se pudo enviar. */
    public static function publish(string $topic, array $payload): bool
    {
        $cfg = require __DIR__ . '/../config/config.php';
        $mqtt = $cfg['mqtt'];

        $client = new MqttClient($mqtt['host'], $mqtt['port'], 'invernadero-api-' . getmypid());
        $settings = (new ConnectionSettings())->setConnectTimeout(5);
        if (($mqtt['user'] ?? '') !== '') {
            $settings->setUsername($mqtt['user'])->setPassword($mqtt['pass']);
        }

        try {
            $client->connect($settings, true);
            $client->publish($topic, json_encode($payload), 0, true);
            $client->disconnect();
            return true;
        } catch (\Throwable $e) {
            return false;
        }
    }
}
