#include <WiFiManager.h>
#include <strings_en.h>
#include <wm_consts_en.h>
#include <wm_strings_en.h>

#include <WiFi.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include "ThingSpeak.h"

// WiFi credentials
/*const char* ssid = "Klein_WiFi_EXT";
const char* password = "ChanchitoKaroll2021";*/

// ThingSpeak credentials
//const char* APIKey = "WI8RVSQ0Y9SSRCJL";
//const long channel = 2105955;
// Definir las credenciales de ThingSpeak
unsigned long channelID = 2105955; // your ThingSpeak channel number
const char* writeAPIKey = "WI8RVSQ0Y9SSRCJL";
char myWriteAPIKey[32];

// Web server
WebServer server(80);

// Soil moisture sensors
const int sensor1Pin = 32; // GPIO 36 corresponde a ADC1_CH0
const int sensor2Pin = 35; // GPIO 39 corresponde a ADC1_CH3
const int sensor3Pin = 34; // GPIO 34 corresponde a ADC1_CH6
const int sensor4Pin = 39; // GPIO 35 corresponde a ADC1_CH7

// Temperature sensor
const int tempSensorPin = A4;

// Relay module pins
const int pump1Pin = 12;
const int pump2Pin = 13;
const int pump3Pin = 14;
const int pump4Pin = 27;
const int light1Pin = 26;
const int light2Pin = 25;
const int fanPin = 33;

// LCD module
LiquidCrystal_I2C lcd(0x27, 16, 2);

// DHT22 temperature and humidity sensor
#define DHTPIN 23
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Variables to store sensor readings
float sensor1Value = 0;
float sensor2Value = 0;
float sensor3Value = 0;
float sensor4Value = 0;
float tempValue = 0;
float humidityValue = 0;
float co2 = 0;
float ch4 = 0;
float lpg = 0;
float bmp_temp = 0;
float bmp_presion = 0;
float bmp_altitud = 0;

// Variables to store pump activation thresholds
int pump1Min = 80;
int pump1Max = 90;
int pump2Min = 50;
int pump2Max = 60;
int pump3Min = 60;
int pump3Max = 70;
int pump4Min = 40;
int pump4Max = 50;

// Variables to store light activation times
int light1Start = 6;
int light1End = 18;
int light2Start = 12;
int light2End = 24;

// Variable to store fan activation temperature
int fanTempMax = 26;
int fanTempMin = 18;

// Variables to store CO2 control thresholds
int co2Max = 1100;
int co2Min = 400;

// Variables for NTPClient
WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "pool.ntp.org");
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000);

// Variables for ThingSpeak
WiFiClient client;

#define LED_BUILTIN 0
WiFiManager wm;  // global wm instance

unsigned long lastUpdateTime = 0;
unsigned long lastUpdateTimeTime = 0;
unsigned long lastUpdateTimeSensor = 0;
const unsigned long updateInterval = 5000; //300000// update every 5 minutes
const unsigned long updateIntervalSensor = 5000;
const unsigned long updateIntervalTime = 15000;

// Function to read soil moisture sensor values
void readSoilMoisture() {
  sensor1Value = analogRead(sensor1Pin);
  sensor2Value = analogRead(sensor2Pin);
  sensor3Value = analogRead(sensor3Pin);
  sensor4Value = analogRead(sensor4Pin);
}

// Function to read temperature sensor value
void readTemperature() {
  tempValue = analogRead(tempSensorPin);
  tempValue = (tempValue * 5.0) / 1024.0;
  tempValue = (tempValue - 0.5) * 100.0;
}

// Function to read DHT22 temperature and humidity values
void readDHT22() {
  humidityValue = dht.readHumidity();
  tempValue = dht.readTemperature();
}

// Function to activate/deactivate pumps based on soil moisture readings
void controlPumps() {
  if (sensor1Value >= pump1Min && sensor1Value <= pump1Max) {
    digitalWrite(pump1Pin, HIGH);
  } else {
    digitalWrite(pump1Pin, LOW);
  }
  if (sensor2Value >= pump2Min && sensor2Value <= pump2Max) {
    digitalWrite(pump2Pin, HIGH);
  } else {
    digitalWrite(pump2Pin, LOW);
  }
  if (sensor3Value >= pump3Min && sensor3Value <= pump3Max) {
    digitalWrite(pump3Pin, HIGH);
  } else {
    digitalWrite(pump3Pin, LOW);
  }
  if (sensor4Value >= pump4Min && sensor4Value <= pump4Max) {
    digitalWrite(pump4Pin, HIGH);
  } else {
    digitalWrite(pump4Pin, LOW);
  }
}

// Function to activate/deactivate lights based on time of day
void controlLights() {
  int hour = timeClient.getHours();
  if (hour >= light1Start && hour < light1End) {
    digitalWrite(light1Pin, HIGH);
  } else {
    digitalWrite(light1Pin, LOW);
  }
  if (hour >= light2Start && hour < light2End) {
    digitalWrite(light2Pin, HIGH);
  } else {
    digitalWrite(light2Pin, LOW);
  }
}

// Function to activate/deactivate fan based on temperature
void controlFan() {
  if ( tempValue >= fanTempMax) {
    digitalWrite(fanPin, HIGH);
  } else {
    digitalWrite(fanPin, LOW);
  }
  if (tempValue <= fanTempMin) {
    digitalWrite(fanPin, LOW);
  }
}

// Function to activate/deactivate ventilation based on CO2 levels
void controlVentilation() {
  int co2Value = analogRead(A0);
  if (co2Value >= co2Max) {
    digitalWrite(fanPin, HIGH);
  } else if (co2Value <= co2Min) {
    digitalWrite(fanPin, LOW);
  }
}

// Function to update ThingSpeak channel with sensor readings
void updateThingSpeak() {
  ThingSpeak.setField(1, sensor1Value);
  ThingSpeak.setField(2, sensor2Value);
  ThingSpeak.setField(3, sensor3Value);
  ThingSpeak.setField(4, sensor4Value);
  ThingSpeak.setField(5, tempValue);
  ThingSpeak.setField(6, humidityValue);
  int status = ThingSpeak.writeFields(channelID, writeAPIKey);

  if (status == 200) {
    Serial.println("Datos enviados a ThingSpeak correctamente.");
  } else {
    Serial.println("Error al enviar datos a ThingSpeak. Código de estado HTTP: " + String(status));
  }
}

//____________________________________________________________________________________

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  EEPROM.begin(512);

  // Initialize LCD module
  lcd.init();
  lcd.backlight();

  // Initialize DHT22 sensor
  dht.begin();

  // Initialize relay module pins
  pinMode(pump1Pin, OUTPUT);
  pinMode(pump2Pin, OUTPUT);
  pinMode(pump3Pin, OUTPUT);
  pinMode(pump4Pin, OUTPUT);
  pinMode(light1Pin, OUTPUT);
  pinMode(light2Pin, OUTPUT);
  pinMode(fanPin, OUTPUT);

  // Connect to WiFi network
  wifi_conector();

  // Initialize NTPClient
  timeClient.begin();

  // Set time from NTP server
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  Serial.println("Hora actual: " + timeClient.getFormattedTime());

  // Initialize ThingSpeak client
  ThingSpeak.begin(client);

  // Read configuration values from EEPROM
  readConfigFromEEPROM();

  // Initialize web server
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/save", handleSave);
  server.on("/configapi", handleApi);
  server.on("/saveapi", handleSaveApi);
  server.begin();
}

void loop() {
  // Handle web server requests
  server.handleClient();

  /*if (millis() - lastUpdateTimeTime >= 50000) {
    // Update time from NTP server
    timeClient.update();
    // Print current time
    Serial.println(timeClient.getFormattedTime());
    Serial.println(timeClient.getHours());
    lastUpdateTimeTime = millis();
  }*/
  
  if (millis() - lastUpdateTimeSensor >= updateIntervalSensor) {
    // Read sensor values
    readSoilMoisture();
    readTemperature();
    readDHT22();
    // Control pumps, lights, fan, and ventilation
    controlPumps();
    controlLights();
    controlFan();
    controlVentilation();
    lastUpdateTimeSensor = millis();

    // Display sensor values on LCD module
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(tempValue);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    lcd.print(humidityValue);
    lcd.print(" %");
  }

  // Update ThingSpeak channel every 5 minutes
  if (millis() - lastUpdateTime >= updateInterval) {
    updateThingSpeak();
    lastUpdateTime = millis();
  }
}
//____________________________________________________________________________________

void wifi_conector() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);

  WiFi.mode(WIFI_STA);
  
  //wm.resetSettings();

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("Invernadero_ESP32", "123456789");  // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();

  } else {
    Serial.println("Connected :)");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

//____________________________________________________________________________________
// EEPROM addresses for configuration values
const int pump1MinAddr = 0;
const int pump1MaxAddr = sizeof(int);
const int pump2MinAddr = sizeof(int) * 2;
const int pump2MaxAddr = sizeof(int) * 3;
const int pump3MinAddr = sizeof(int) * 4;
const int pump3MaxAddr = sizeof(int) * 5;
const int pump4MinAddr = sizeof(int) * 6;
const int pump4MaxAddr = sizeof(int) * 7;
const int light1StartAddr = sizeof(int) * 8;
const int light1EndAddr = sizeof(int) * 9;
const int light2StartAddr = sizeof(int) * 10;
const int light2EndAddr = sizeof(int) * 11;
const int fanTempMaxAddr = sizeof(int) * 12;
const int fanTempMinAddr = sizeof(int) * 13;
const int co2MaxAddr = sizeof(int) * 14;
const int co2MinAddr = sizeof(int) * 15;
const int channelIDAddr = sizeof(int) * 16;
const int writeAPIKeyAddr = sizeof(char) * 17;

// Function to read configuration values from EEPROM
void readConfigFromEEPROM() {
  /*EEPROM.get(pump1MinAddr, pump1Min);
  EEPROM.get(pump1MaxAddr, pump1Max);
  EEPROM.get(pump2MinAddr, pump2Min);
  EEPROM.get(pump2MaxAddr, pump2Max);
  EEPROM.get(pump3MinAddr, pump3Min);
  EEPROM.get(pump3MaxAddr, pump3Max);
  EEPROM.get(pump4MinAddr, pump4Min);
  EEPROM.get(pump4MaxAddr, pump4Max);
  EEPROM.get(light1StartAddr, light1Start);
  EEPROM.get(light1EndAddr, light1End);
  EEPROM.get(light2StartAddr, light2Start);
  EEPROM.get(light2EndAddr, light2End);
  EEPROM.get(fanTempMaxAddr, fanTempMax);
  EEPROM.get(fanTempMinAddr, fanTempMin);
  EEPROM.get(co2MaxAddr, co2Max);
  EEPROM.get(co2MinAddr, co2Min);
  EEPROM.get(channelIDAddr, channelID);
  EEPROM.get(writeAPIKeyAddr, writeAPIKey);
  //EEPROM.end();*/
  //___________________________________
  pump1Min = EEPROM.read(105);
  pump1Max = EEPROM.read(108);
  pump2Min = EEPROM.read(111);
  pump2Max = EEPROM.read(114);
  pump3Min = EEPROM.read(117);
  pump3Max = EEPROM.read(120);
  pump4Min = EEPROM.read(123);
  pump4Max = EEPROM.read(126);
  light1Start = EEPROM.read(129);
  light1End = EEPROM.read(132);
  light2Start = EEPROM.read(135);
  light2End = EEPROM.read(139);
  fanTempMax = EEPROM.read(143);
  fanTempMin = EEPROM.read(147);
  co2Max = EEPROM.read(149);
  co2Min = EEPROM.read(151);
  //___________________________________
  EEPROM.get(65, channelID);
  EEPROM.get(70, writeAPIKey);
  //___________________________________
  Serial.println("Se leyo la informacion de readConfigFromEEPROM");

  Serial.println("Valores");
  Serial.print("Humedad: ");
  Serial.print(pump1MinAddr);
  Serial.print(" - ");
  Serial.print(pump1MaxAddr);
  Serial.print(" / ");
  Serial.print(pump2MinAddr);
  Serial.print(" - ");
  Serial.print(pump2MaxAddr);
  Serial.print(" / ");
  Serial.print(pump3MinAddr);
  Serial.print(" - ");
  Serial.print(pump3MaxAddr);
  Serial.print(" / ");
  Serial.print(pump4MinAddr);
  Serial.print(" - ");
  Serial.println(pump4MaxAddr);
  Serial.print("Iluminacion: ");
  Serial.print(light1StartAddr);
  Serial.print(" - ");
  Serial.print(light1EndAddr);
  Serial.print(" / ");
  Serial.print(light2StartAddr);
  Serial.print(" - ");
  Serial.println(light2EndAddr);
  Serial.print("Ventilacion: ");
  Serial.print(fanTempMaxAddr);
  Serial.print(" - ");
  Serial.println(fanTempMinAddr);
  Serial.println("ThingSpeak");
  Serial.print("Numero de cana: ");
  Serial.println(channelID);
  Serial.print("Api: ");
  Serial.println(writeAPIKey);

}

// Function to write configuration values to EEPROM
void writeConfigToEEPROM() {
  //EEPROM.begin(512);
  EEPROM.put(105, pump1Min);
  EEPROM.put(108, pump1Max);
  EEPROM.put(111, pump2Min);
  EEPROM.put(114, pump2Max);
  EEPROM.put(117, pump3Min);
  EEPROM.put(120, pump3Max);
  EEPROM.put(123, pump4Min);
  EEPROM.put(126, pump4Max);
  EEPROM.put(129, light1Start);
  EEPROM.put(132, light1End);
  EEPROM.put(135, light2Start);
  EEPROM.put(139, light2End);
  EEPROM.put(143, fanTempMax);
  EEPROM.put(147, fanTempMin);
  EEPROM.put(149, co2Max);
  EEPROM.put(151, co2Min);
  EEPROM.commit();
  EEPROM.end();
  /*EEPROM.put(pump1MinAddr, pump1Min);
  EEPROM.put(pump1MaxAddr, pump1Max);
  EEPROM.put(pump2MinAddr, pump2Min);
  EEPROM.put(pump2MaxAddr, pump2Max);
  EEPROM.put(pump3MinAddr, pump3Min);
  EEPROM.put(pump3MaxAddr, pump3Max);
  EEPROM.put(pump4MinAddr, pump4Min);
  EEPROM.put(pump4MaxAddr, pump4Max);
  EEPROM.put(light1StartAddr, light1Start);
  EEPROM.put(light1EndAddr, light1End);
  EEPROM.put(light2StartAddr, light2Start);
  EEPROM.put(light2EndAddr, light2End);
  EEPROM.put(fanTempMaxAddr, fanTempMax);
  EEPROM.put(fanTempMinAddr, fanTempMin);
  EEPROM.put(co2MaxAddr, co2Max);
  EEPROM.put(co2MinAddr, co2Min);
  EEPROM.commit();*/
  //EEPROM.end();
  Serial.println(" Configuracion guardada de writeConfigToEEPROM");
}

// Function to handle Config page
void handleConfig() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Configuración</title>";
  html += "<style>body {background-color: #303030;font-family: Arial, sans-serif;font-size: 16px;line-height: 1.6;margin: 0;padding: 0;text-align: center;}nav {background-color: #333;overflow: hidden;text-align: center;}nav ul {margin: 0;padding: 0;list-style: none;display: flex;justify-content: space-between;text-align: center;}nav a {display: block;color: #fff;padding: 14px 16px;text-decoration: none;text-align: center;}nav a:hover {background-color: #ddd;color: #000;text-align: center;}nav a.active {background-color: #04aa6d;color: #fff;text-align: center;}h1 {color: #fff;font-size: 20px;margin-top: 30px;margin-bottom: 10px;text-align: center;}h2 {color: #fff;font-size: 16px;margin-top: 20px;margin-bottom: 7px;text-align: center;}p {color: #fff;font-size: 12px;margin-top: 8px;margin-bottom: 5px;text-align: center;}label {color: #fff;display: block;margin-top: 15px;margin-bottom: 5px;font-weight: 700;text-align: center;}input[type=number],input[type=text] {padding: 5px;border-radius: 5px;border: 1px solid #ccc;margin-bottom: 10px;font-size: 16px;width: 200px;text-align: center;display: block;margin: auto;}table {margin: 0 auto;border-collapse: collapse;width: 80%;}input[type=submit] {background-color: #4caf50;color: #fff;padding: 10px 20px;border: none;border-radius: 5px;cursor: pointer;font-size: 16px;text-align: center;}input[type=submit]:hover {background-color: #45a049;text-align: center;}.subrayado {text-decoration: underline;color: #fff;text-align: center;}.resaltado {color: #4caf50;text-align: center;}</style>";
  html += "</head><body>";
  html += "<nav><ul>";
  html += "<li><a href='/'>Inicio</a></li>";
  html += "<li><a class='active' href='/configapi'>Configuraci&oacute;n Apis</a></li>";
  html += "<li><a class='active' href='/config'>Configuraci&oacute;n</a></li>";
  html += "</ul></nav>";
  html += "<h1>Configuración</h1>";
  html += "<form method='POST' action='/save'>";
  html += "<h2>Bombas</h2>";
  html += "<label for='pump1Min'>Mínimo de humedad sonda 1:</label>";
  html += "<input type='number' id='pump1Min' name='pump1Min' value='" + String(pump1Min) + "'><br>";
  html += "<label for='pump1Max'>M&aacute;ximo de humedad sonda 1:</label>";
  html += "<input type='number' id='pump1Max' name='pump1Max' value='" + String(pump1Max) + "'><br>";
  html += "<label for='pump2Min'>Mínimo de humedad sonda :</label>";
  html += "<input type='number' id='pump2Min' name='pump2Min' value='" + String(pump2Min) + "'><br>";
  html += "<label for='pump2Max'>M&aacute;ximo de humedad sonda 2:</label>";
  html += "<input type='number' id='pump2Max' name='pump2Max' value='" + String(pump2Max) + "'><br>";
  html += "<label for='pump3Min'>Mínimo de humedad sonda 3:</label>";
  html += "<input type='number' id='pump3Min' name='pump3Min' value='" + String(pump3Min) + "'><br>";
  html += "<label for='pump3Max'>M&aacute;ximo de humedad sonda 3:</label>";
  html += "<input type='number' id='pump3Max' name='pump3Max' value='" + String(pump3Max) + "'><br>";
  html += "<label for='pump4Min'>Mínimo de humedad sonda 4:</label>";
  html += "<input type='number' id='pump4Min' name='pump4Min' value='" + String(pump4Min) + "'><br>";
  html += "<label for='pump4Max'>M&aacute;ximo de humedad sonda 4:</label>";
  html += "<input type='number' id='pump4Max' name='pump4Max' value='" + String(pump4Max) + "'><br>";
  html += "<h2>Luces</h2>";
  html += "<label for='light1Start'>Hora de inicio de luz Zona 1:</label>";
  html += "<input type='text' id='light1Start' name='light1Start' value='" + String(light1Start) + "'><br>";
  html += "<label for='light1End'>Hora de fin de luz Zona 1:</label>";
  html += "<input type='text' id='light1End' name='light1End' value='" + String(light1End) + "'><br>";
  html += "<label for='light2Start'>Hora de inicio de luz Zona 2:</label>";
  html += "<input type='text' id='light2Start' name='light2Start' value='" + String(light2Start) + "'><br>";
  html += "<label for='light2End'>Hora de fin de luz Zona 2:</label>";
  html += "<input type='text' id='light2End' name='light2End' value='" + String(light2End) + "'><br>";
  html += "<h2>Ventilador</h2>";
  html += "<label for='fanTempMax'>Temperatura m&aacute;xima para encender el ventilador:</label>";
  html += "<input type='number' id='fanTempMax' name='fanTempMax' value='" + String(fanTempMax) + "'><br>";
  html += "<label for='fanTempMin'>Temperatura mínima para apagar el ventilador:</label>";
  html += "<input type='number' id='fanTempMin' name='fanTempMin' value='" + String(fanTempMin) + "'><br>";
  html += "<h2>CO2</h2>";
  html += "<label for='co2Max'>M&aacute;ximo de CO2:</label>";
  html += "<input type='number' id='co2Max' name='co2Max' value='" + String(co2Max) + "'><br>";
  html += "<label for='co2Min'>Mínimo de CO2:</label>";
  html += "<input type='number' id='co2Min' name='co2Min' value='" + String(co2Min) + "'><br>";
  html += "<input type='submit' value='Guardar'>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Function to handle save page
void handleSave() {
  pump1Min = server.arg("pump1Min").toInt();
  pump1Max = server.arg("pump1Max").toInt();
  pump2Min = server.arg("pump2Min").toInt();
  pump2Max = server.arg("pump2Max").toInt();
  pump3Min = server.arg("pump3Min").toInt();
  pump3Max = server.arg("pump3Max").toInt();
  pump4Min = server.arg("pump4Min").toInt();
  pump4Max = server.arg("pump4Max").toInt();
  light1Start = server.arg("light1Start").toInt();
  light1End = server.arg("light1End").toInt();
  light2Start = server.arg("light2Start").toInt();
  light2End = server.arg("light2End").toInt();
  fanTempMax = server.arg("fanTempMax").toInt();
  fanTempMin = server.arg("fanTempMin").toInt();
  co2Max = server.arg("co2Max").toInt();
  co2Min = server.arg("co2Min").toInt();
  writeConfigToEEPROM();
  server.send(200, "text/plain", "Configuración guardada");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Inicio</title>";
  //html += "<style>body{background-color:#303030;font-family:Arial,sans-serif;font-size:16px;line-height:1.6;margin:0;padding:0}nav{background-color:#333;overflow:hidden}nav ul{margin:0;padding:0;list-style:none;display:flex;justify-content:space-between}nav a{display:block;color:#fff;text-align:center;padding:14px 16px;text-decoration:none}nav a:hover{background-color:#ddd;color:#000}nav a.active{background-color:#04aa6d;color:#fff}h1{color:#fff;margin-bottom:30px;text-align:center}h2{color:#fff;margin-bottom:25px;text-align:center}ul{color:#fff;display:flex;justify-content:center;list-style:none;margin:0;padding:0}li{color:#fff;margin:0 10px}a{color:#333;text-decoration:none}a:hover{color:#007bff}table{margin:0 auto;border-collapse:collapse;width:80%}td,th{color:#fff;text-align:center;padding:10px}th{background-color:#4caf50;color:#fff}</style>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "</head><body>";
  html += "<nav><ul>";
  html += "<li><a class='active' href='/'>Inicio</a></li>";
  html += "<li><a class='active' href='/configapi'>Configuraci&oacute;n Apis</a></li>";
  html += "<li><a href='/config'>Configuraci&oacute;n</a></li>";
  html += "</ul></nav>";
  html += "<h2>Hora actual " + String(timeClient.getFormattedTime()) + "</h2>";
  html += "<h1>Datos de sensores</h1>";
  html += "<table>";
  html += "<tr><td>Sensor 1:</td><td>" + String(sensor1Value) + "</td></tr>";
  html += "<tr><td>Sensor 2:</td><td>" + String(sensor2Value) + "</td></tr>";
  html += "<tr><td>Sensor 3:</td><td>" + String(sensor3Value) + "</td></tr>";
  html += "<tr><td>Sensor 4:</td><td>" + String(sensor4Value) + "</td></tr>";
  html += "<tr><td>Temperatura:</td><td>" + String(tempValue) + "</td></tr>";
  html += "<tr><td>Humedad:</td><td>" + String(humidityValue) + "</td></tr>";
  html += "<tr><td>CO2:</td><td>" + String(co2) + "</td></tr>";
  html += "<tr><td>CH4:</td><td>" + String(ch4) + "</td></tr>";
  html += "<tr><td>LPG:</td><td>" + String(lpg) + "</td></tr>";
  html += "<tr><td>Temperatura BMP:</td><td>" + String(bmp_temp) + "</td></tr>";
  html += "<tr><td>Presi&oacute;n BMP:</td><td>" + String(bmp_presion) + "</td></tr>";
  html += "<tr><td>Altitud BMP:</td><td>" + String(bmp_altitud) + "</td></tr>";
  html += "</table>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}
//____________________________________________________________________________________

void handleApi() {
  String html = "<html>";
  html += "<style>body {background-color: #303030;font-family: Arial, sans-serif;font-size: 16px;line-height: 1.6;margin: 0;padding: 0;text-align: center;}nav {background-color: #333;overflow: hidden;text-align: center;}nav ul {margin: 0;padding: 0;list-style: none;display: flex;justify-content: space-between;text-align: center;}nav a {display: block;color: #fff;padding: 14px 16px;text-decoration: none;text-align: center;}nav a:hover {background-color: #ddd;color: #000;text-align: center;}nav a.active {background-color: #04aa6d;color: #fff;text-align: center;}h1 {color: #fff;font-size: 20px;margin-top: 30px;margin-bottom: 10px;text-align: center;}h2 {color: #fff;font-size: 16px;margin-top: 20px;margin-bottom: 7px;text-align: center;}p {color: #fff;font-size: 12px;margin-top: 8px;margin-bottom: 5px;text-align: center;}label {color: #fff;display: block;margin-top: 15px;margin-bottom: 5px;font-weight: 700;text-align: center;}input[type=number],input[type=text] {padding: 5px;border-radius: 5px;border: 1px solid #ccc;margin-bottom: 10px;font-size: 16px;width: 200px;text-align: center;display: block;margin: auto;}table {margin: 0 auto;border-collapse: collapse;width: 80%;}input[type=submit] {background-color: #4caf50;color: #fff;padding: 10px 20px;border: none;border-radius: 5px;cursor: pointer;font-size: 16px;text-align: center;}input[type=submit]:hover {background-color: #45a049;text-align: center;}.subrayado {text-decoration: underline;color: #fff;text-align: center;}.resaltado {color: #4caf50;text-align: center;}</style>";
  html += "<body>";
  html += "<h1>Configuraci&oacute;n</h1>";
  html += "<form action='/saveapi' method='POST'>";
  html += "<label for='channel_id'>ID del canal:</label><br>";
  html += "<input type='text' id='channel_id' name='channel_id' value='" + String(channelID) + "'><br>";
  html += "<label for='write_api_key'>Clave de API de escritura:</label><br>";
  html += "<input type='text' id='write_api_key' name='write_api_key' value='" + String(writeAPIKey) + "'><br>";
  html += "<input type='submit' value='Guardar'>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSaveApi() {
  String channel_id = server.arg("channel_id");
  String write_api_key = server.arg("write_api_key");

  channelID = channel_id.toInt();
  writeAPIKey = write_api_key.c_str();
  Serial.println(channelID);
  Serial.println(writeAPIKey);

  /*EEPROM.put(channelIDAddr, channelID);
  EEPROM.put(writeAPIKeyAddr, writeAPIKey);*/
  EEPROM.put(65, channelID);
  EEPROM.put(70, writeAPIKey);

  EEPROM.commit();
  //EEPROM.end();
  
  String html = "<html>";
  html += "<style>body {background-color: #303030;font-family: Arial, sans-serif;font-size: 16px;line-height: 1.6;margin: 0;padding: 0;text-align: center;}nav {background-color: #333;overflow: hidden;text-align: center;}nav ul {margin: 0;padding: 0;list-style: none;display: flex;justify-content: space-between;text-align: center;}nav a {display: block;color: #fff;padding: 14px 16px;text-decoration: none;text-align: center;}nav a:hover {background-color: #ddd;color: #000;text-align: center;}nav a.active {background-color: #04aa6d;color: #fff;text-align: center;}h1 {color: #fff;font-size: 20px;margin-top: 30px;margin-bottom: 10px;text-align: center;}h2 {color: #fff;font-size: 16px;margin-top: 20px;margin-bottom: 7px;text-align: center;}p {color: #fff;font-size: 12px;margin-top: 8px;margin-bottom: 5px;text-align: center;}label {color: #fff;display: block;margin-top: 15px;margin-bottom: 5px;font-weight: 700;text-align: center;}input[type=number],input[type=text] {padding: 5px;border-radius: 5px;border: 1px solid #ccc;margin-bottom: 10px;font-size: 16px;width: 200px;text-align: center;display: block;margin: auto;}table {margin: 0 auto;border-collapse: collapse;width: 80%;}input[type=submit] {background-color: #4caf50;color: #fff;padding: 10px 20px;border: none;border-radius: 5px;cursor: pointer;font-size: 16px;text-align: center;}input[type=submit]:hover {background-color: #45a049;text-align: center;}.subrayado {text-decoration: underline;color: #fff;text-align: center;}.resaltado {color: #4caf50;text-align: center;}</style>";
  html += "<body>";
  html += "<h1>Configuraci&oacute;n guardada</h1>";
  html += "<a href='/'>Volver</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}
//____________________________________________________________________________________
