#pragma once
// Tipos y estructuras compartidas por todo el firmware.
// Se centralizan aquí para mantener consistencia entre módulos.

#include <Arduino.h>

namespace gh {

// Estado de un sensor/lectura. Nunca se asume que un valor es válido (sección 56).
enum class SensorStatus : uint8_t {
  UNKNOWN = 0,      // Aún no leído
  OK = 1,           // Lectura válida (calidad GOOD)
  WARNING = 2,      // Lectura dudosa
  ERROR = 3,        // Lectura inválida (hardware presente pero falla)
  DISCONNECTED = 4, // Hardware no detectado
  OUT_OF_RANGE = 5, // Valor fuera de los límites configurados
  INVALID = 6,      // Valor imposible/no válido
  TIMEOUT = 7,      // Sin respuesta en el tiempo esperado
  CALIBRATION = 8   // Calibración pendiente/incompleta
};

// Tipos de sensor soportados por el firmware (hardware disponible).
enum class SensorType : uint8_t {
  NONE = 0,
  TEMP_SHT31,        // Temperatura SHT31
  HUM_SHT31,         // Humedad SHT31
  TEMP_AHT20,        // Temperatura AHT20
  HUM_AHT20,         // Humedad AHT20
  TEMP_DS18B20,      // Temperatura DS18B20 (bus 1-Wire)
  SOIL_MOISTURE,     // Humedad de suelo capacitivo (ADC)
  LIGHT_LUX,         // Iluminación BH1750
  CO2,               // Dióxido de carbono SCD4x
  FLOW_RATE,         // Caudal instantáneo
  TANK_LEVEL,        // Nivel del depósito (ultrasónico)
  RAIN_ACCUM,        // Lluvia acumulada (pluviómetro)
  WIND_SPEED,        // Velocidad del viento (anemómetro)
  PH,                // pH (analógico o Modbus)
  EC,                // Conductividad eléctrica
  TEMP_EXTERIOR,     // Temperatura exterior (segundo sensor)
  HUM_EXTERIOR      // Humedad exterior
};

// Rol/función de un actuador. Determina cómo lo gestionan los controladores.
enum class ActuatorRole : uint8_t {
  GENERIC = 0,
  PUMP,          // Bomba
  VALVE,         // Electroválvula de riego
  FAN,           // Ventilador
  EXTRACTOR,     // Extractor
  HEATER,        // Calefacción
  HUMIDIFIER,    // Humidificador
  LIGHT,         // Iluminación
  WINDOW_OPEN,   // Apertura de ventana
  WINDOW_CLOSE,  // Cierre de ventana
  ROOF_OPEN,     // Apertura de techo
  ROOF_CLOSE,    // Cierre de techo
  SHADE_OPEN,    // Apertura de sombreado
  SHADE_CLOSE,   // Cierre de sombreado
  ALARM          // Salida de alarma
};

// Tipo eléctrico de salida: digital (ON/OFF) o PWM.
enum class OutputKind : uint8_t {
  DIGITAL = 0,
  PWM = 1
};

// Modo de operación del sistema (sección 5).
enum class OpMode : uint8_t {
  AUTO = 0,       // Control automático
  MANUAL = 1,     // Control manual desde la web
  PROGRAMMED = 2, // Horarios/programas
  SAFETY = 3      // Estado de seguridad (prevalece sobre el resto)
};

// Tipo de instalación (sección 63).
enum class GreenhouseType : uint8_t {
  INDOOR = 0,
  OUTDOOR = 1,
  MIXED = 2
};

// Roles de usuario del servidor central (sección 76).
enum class UserRole : uint8_t {
  ADMIN = 0,
  OPERATOR = 1,
  VIEWER = 2
};

// Estado de funcionamiento del dispositivo (sección 175).
enum class DeviceState : uint8_t {
  BOOTING = 0, INITIALIZING = 1, SELF_TEST = 2, NETWORK = 3, SYNC = 4,
  RUN = 5, DEGRADED = 6, ERROR = 7, MAINTENANCE = 8, UPDATING = 9, RECOVERY = 10
};

// Causa del último reinicio (sección 150).
enum class ResetCause : uint8_t {
  POWER_ON = 0, SOFTWARE_RESET = 1, WATCHDOG = 2, BROWNOUT = 3,
  PANIC = 4, OTA = 5, FACTORY_RESET = 6, UNKNOWN = 7
};

// Fuente propietaria de la configuración (sección 103).
enum class ConfigSource : uint8_t { LOCAL = 0, CENTRAL = 1 };

// Canal de actualización de firmware (sección 148).
enum class UpdateChannel : uint8_t { STABLE = 0, BETA = 1, DEVELOPMENT = 2 };

// Identidad y capacidades del dispositivo (secciones 118/129/166/179).
struct DeviceInfo {
  char deviceUid[24] = "";                    // Identidad permanente (derivada de MAC)
  char hardwareProfile[24] = "ESP32-GH-V1";   // Perfil de hardware
  char firmwareVersion[12] = "3.0.0";         // Versión de firmware
  char hardwareVersion[12] = "rev0";          // Revisión de hardware
  uint16_t configSchemaVersion = 1;           // Esquema de configuración
  uint16_t protocolVersion = 1;               // Versión de protocolo
  UpdateChannel channel = UpdateChannel::STABLE;
  char capabilities[10][16] = {};             // Lista de capacidades (sección 166)
  uint8_t capabilityCount = 0;
};

// Estadísticas del bus RS485/Modbus (sección 177).
struct ModbusStats {
  uint32_t txCount = 0;
  uint32_t rxCount = 0;
  uint32_t crcErrors = 0;
  uint32_t timeouts = 0;
  uint32_t retries = 0;
  uint8_t lastError = 0;
  uint8_t devicesFound = 0;
};

// Estructura de configuración de un sensor (sección 38).
struct SensorConfig {
  bool enabled = false;              // Habilitado
  uint8_t zone = 0;                  // Zona a la que pertenece
  uint32_t readIntervalMs = 5000;    // Intervalo de lectura
  float minValue = 0.0f;             // Valor mínimo para detección de rango
  float maxValue = 0.0f;             // Valor máximo para detección de rango
  float soilDryRaw = 2850.0f;        // Calibración suelo: lectura ADC seco (RAW)
  float soilWetRaw = 1450.0f;        // Calibración suelo: lectura ADC húmedo (RAW)
  float ph4Voltage = 3.02f;          // Calibración pH: tensión patrón 4.00
  float ph7Voltage = 2.51f;          // Calibración pH: tensión patrón 7.00
  float ph10Voltage = 2.01f;         // Calibración pH: tensión patrón 10.00
  uint8_t slaveId = 1;               // Dirección Modbus (sección 18.4)
  uint16_t modbusRegister = 0;       // Registro Modbus a leer
  bool temperatureCompensation = false; // Compensación de temperatura
  uint32_t pollIntervalMs = 5000;    // Intervalo de polling Modbus
};

// Estructura de configuración de un actuador (sección 39).
struct ActuatorConfig {
  bool enabled = false;                  // Habilitado
  uint8_t zone = 0;                      // Zona
  OutputKind kind = OutputKind::DIGITAL; // Tipo de salida
  uint8_t channel = 0;                   // Canal lógico (índice en el expansor)
  OpMode mode = OpMode::AUTO;            // Modo de operación
  uint32_t pwmFrequency = 500;           // Frecuencia PWM (Hz)
  uint8_t pwmResolution = 8;             // Resolución PWM (bits)
  float minValue = 0.0f;                 // Mínimo % (PWM)
  float maxValue = 100.0f;               // Máximo % (PWM)
  uint32_t minOnMs = 0;                  // Tiempo mínimo encendido
  uint32_t minOffMs = 0;                 // Tiempo mínimo apagado
  bool safeState = false;                // Estado seguro al arrancar/reiniciar
};

// Estructura completa de configuración del sistema (sección 82).
struct SystemConfig {
  char deviceId[16] = "GH001";
  char deviceName[32] = "Invernadero";
  GreenhouseType type = GreenhouseType::OUTDOOR;
  char greenhouseId[24] = "GREENHOUSE-001";          // Identificación del invernadero (sección 102)
  uint32_t configVersion = 1;                        // Versión de configuración (sección 104)
  ConfigSource configSource = ConfigSource::LOCAL;   // Propietario de la config (sección 103)
  bool simulation = false;                           // Modo simulación sin hardware (sección 191)
  UpdateChannel updateChannel = UpdateChannel::STABLE; // Canal OTA (sección 148)

  // Funciones habilitadas (sección 62).
  bool featureClimate = true;
  bool featureIrrigation = true;
  bool featureLighting = false;
  bool featureCo2 = false;
  bool featureHeating = false;
  bool featureHumidification = false;
  bool featureRoof = false;
  bool featureWindows = false;
  bool featureShade = false;

  // Clima (secciones 64/65).
  float tempMin = 18.0f;
  float tempTarget = 24.0f;
  float tempMax = 28.0f;
  float tempEmergency = 35.0f;
  float tempHysteresis = 2.0f;
  float humMin = 55.0f;
  float humTarget = 70.0f;
  float humMax = 85.0f;
  float humHysteresis = 5.0f;

  // Riego (sección 66).
  float soilMin = 35.0f;
  float soilTarget = 55.0f;
  uint32_t irrigationMaxTimeMs = 15UL * 60UL * 1000UL; // 15 min
  float flowMin = 1.0f;              // Caudal mínimo (L/min)
  uint32_t flowCheckDelayMs = 5000;  // Espera tras arrancar la bomba

  // Ventilación (sección 67).
  float ventOnTemp = 28.0f;
  float ventOffTemp = 25.0f;
  float ventHumMax = 85.0f;

  // Techo (sección 68).
  float roofOpenTemp = 28.0f;
  float roofCloseTemp = 24.0f;
  bool roofCloseOnRain = true;
  bool roofCloseOnWind = true;
  float windMaxSpeed = 30.0f;        // km/h

  // Iluminación (sección 30).
  float lightMinLux = 5000.0f;
  uint8_t lightStartHour = 6;
  uint8_t lightEndHour = 22;
  float lightIntensity = 80.0f;      // %

  // Calibración de sensores (sección 54). Valores ilustrativos; se ajustan
  // desde la interfaz web durante la calibración real.
  float soilDryRaw[4] = {2850, 2850, 2850, 2850}; // ADC en seco por zona
  float soilWetRaw[4] = {1450, 1450, 1450, 1450}; // ADC en húmedo por zona
  float ph4Voltage = 3.02f;                        // Tensión patrón pH 4.00
  float ph7Voltage = 2.51f;                        // Tensión patrón pH 7.00
  float ph10Voltage = 2.01f;                       // Tensión patrón pH 10.00
  float flowLitersPerPulse = 1.0f / 450.0f;        // caud. ~450 pulsos/L
  float rainMmPerPulse = 0.2794f;                  // pluviómetro mm/pulso
  float windKmhPerPulse = 2.4f;                    // anemómetro km/h por pulso
  float tankDepthCm = 100.0f;                      // profundidad del tanque

  // Red (secciones 37/86).
  char wifiSsid[32] = "";
  char wifiPass[64] = "";
  char hostname[32] = "invernadero";
  char apSsid[32] = "Invernadero-AP";
  char apPass[64] = "invernadero";
  char mqttHost[64] = "";
  uint16_t mqttPort = 1883;
  char mqttUser[32] = "";
  char mqttPass[64] = "";
  char ntpServer[48] = "pool.ntp.org";
  int8_t timezoneOffset = -3;        // ARG
  char timezone[48] = "America/Argentina/Buenos_Aires"; // IANA (sección 125)
  char dnsPrimary[16] = "8.8.8.8";   // DNS (sección 126)
  char dnsSecondary[16] = "1.1.1.1";

  // Servidor central (sección 128).
  bool managedByCentral = false;
  char centralUrl[64] = "";
  uint16_t centralPort = 443;
  char centralToken[64] = "";

  // RS485 / Modbus (sección 113).
  uint32_t rs485Baud = 9600;
  uint8_t rs485Parity = 0;     // 0=NONE, 1=EVEN, 2=ODD
  uint8_t rs485StopBits = 1;

  // Sensores habilitados.
  bool sensorSht31 = true;
  bool sensorDs18b20 = true;
  bool sensorSoil = true;
  bool sensorLight = true;
  bool sensorCo2 = false;
  bool sensorRain = false;
  bool sensorWind = false;
  bool sensorTank = true;
  bool sensorFlow = true;
  bool sensorPh = false;
  bool sensorEc = false;
  bool sensorExterior = false;

  // Actuadores habilitados.
  bool actPump = true;
  uint8_t actValves = 4;
  uint8_t actFans = 2;
  uint8_t actExtractors = 1;
  uint8_t actLights = 1;
  bool actHeater = false;
  bool actHumidifier = false;
  bool actRoof = false;
  bool actWindow = false;
  bool actShade = false;

  // Zonas (sección 120).
  uint8_t zoneCount = 4;
  char zoneNames[8][16] = {"Zona 1", "Zona 2", "Zona 3", "Zona 4", "", "", "", ""};
};

// Lectura de un sensor en tiempo de ejecución.
struct SensorValue {
  SensorType type = SensorType::NONE;
  String name;
  uint8_t zone = 0;
  bool enabled = false;
  float value = 0.0f;
  float raw = 0.0f;
  SensorStatus status = SensorStatus::UNKNOWN;
  String unit;
  uint32_t lastReadMs = 0;
};

// Estado en tiempo de ejecución de un actuador.
struct ActuatorState {
  ActuatorRole role = ActuatorRole::GENERIC;
  String name;
  uint8_t index = 0;      // Subíndice (nº de válvula, ventilador, etc.)
  uint8_t zone = 0;
  bool enabled = false;
  OutputKind kind = OutputKind::DIGITAL;
  OpMode mode = OpMode::AUTO;
  float output = 0.0f;      // 0..100 (PWM) o 0/100 (digital)
  bool requested = false;   // Orden deseada (antes de aplicar seguridad)
  bool fault = false;       // Fallo detectado
  uint32_t lastChangeMs = 0;
};

// Evento del historial (sección 77).
struct LogEvent {
  uint32_t timestamp = 0;   // epoch seconds
  uint8_t severity = 0;     // 0 info, 1 warning, 2 error, 3 alarma
  char message[96] = "";
};

// Conversiones a cadena para API/diagnóstico (secciones 150/175/176).
const char* sensorStatusString(SensorStatus s);
const char* deviceStateString(DeviceState s);
const char* resetCauseString(ResetCause s);
const char* updateChannelString(UpdateChannel c);

} // namespace gh

