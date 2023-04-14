#include "ThingSpeak.h"
#include "WiFi.h"
#include <Wire.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>


#define pin1 32//A0   // Pin del sensor de humedad del suelo
#define pin2 33//A1   // Pin del sensor de temperatura LM35DZ
#define pin6 25   // Pin del sensor de temperatura y humedad DHT22
#define pin3 14   // Pin del relé 1 (Bomba de agua)
#define pin4 27   // Pin del relé 2 (Iluminación)
#define pin5 26   // Pin del relé 3 (Ventilación)

const char* ssid = "ssid";                        // SSID del router.
const char* password = "password";                // Contraseña del router.
const char* timeServer = "pool.ntp.org";          // Servidor NTP para el módulo de tiempo real.
const long  gmtOffset_sec = -10800;               // Zona horaria (en segundos).
const int   daylightOffset_sec = 0;               // Desfase horario de horario de verano (en segundos).

unsigned long channelID = 999999;                 // ID del canal.
const char* writeAPIKey = "WriteAPIKey";          // Write API Key del canal.

WiFiClient client;

DHT dht(pin6, DHT22);  // Crear objeto DHT

LiquidCrystal_I2C lcd(0x3F, 16, 2); // Dirección del display en el bus I2C y tamaño del display

void setup() {
  Serial.begin(115200);
  Serial.println("Test de sensores:");

  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wifi conectado!");

  // Inicializar la comunicación con ThingSpeak
  ThingSpeak.begin(client);

  // Inicializar el sensor de humedad del suelo
  pinMode(pin1, INPUT);

  // Inicializar el sensor de temperatura LM35DZ
  pinMode(pin2, INPUT);

  // Inicializar los relés
  pinMode(pin3, OUTPUT);
  pinMode(pin4, OUTPUT);
  pinMode(pin5, OUTPUT);

  // Inicializar el sensor de temperatura y humedad DHT22
  dht.begin();

  // Inicializar el módulo de tiempo real
  configTime(gmtOffset_sec, daylightOffset_sec, timeServer);
  Serial.println("Configuración de hora completa.");

  // Inicializar el display LCD
  lcd.init();                      // Inicializar el display
  lcd.backlight();                 // Encender la luz de fondo
}

void loop() {
  // Obtener la hora actual
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Error al obtener la hora");
    return;
  }

  // Leer la humedad del suelo
  float humedadSuelo = analogRead(pin1);

  Serial.print("Humedad del suelo: ");
  Serial.println(humedadSuelo);

  // Leer la temperatura del sensor LM35DZ
  float temperatura = (analogRead(pin2) * 3.3) / 4095.0;
  temperatura = temperatura * 100;

  Serial.print("Temperatura LM35DZ: ");
  Serial.println(temperatura);
// Leer la temperatura y humedad del sensor DHT22
  float temperaturaD= dht.readTemperature();
  float humedadD = dht.readHumidity();

  Serial.print("Temperatura DHT22: ");
  Serial.println(temperaturaD);
  Serial.print("Humedad DHT22: ");
  Serial.println(humedadD);  

  // Controlar la bomba de agua en base a la humedad del suelo
  if (humedadSuelo < 60) {
    digitalWrite(pin3, HIGH);
  } else if (humedadSuelo > 70) {
    digitalWrite(pin3, LOW);
  }

  // Controlar la iluminación en base a la hora del día
  int hora = timeinfo.tm_hour;
  if (hora >= 6 && hora < 18) {
    digitalWrite(pin4, HIGH);
  } else {
    digitalWrite(pin4, LOW);
  }

  // Controlar la ventilación en base a la temperatura
  if (temperatura > 25) {
    digitalWrite(pin5, HIGH);
  } else if (temperatura < 20) {
    digitalWrite(pin5, LOW);
  }

  // Mostrar la información en el display LCD
  lcd.setCursor(0,1);
  lcd.print("H S: ");
  lcd.print(humedadSuelo);
  lcd.print("%");

  lcd.setCursor(0,0);
  lcd.print("Temp: ");
  lcd.print(temperatura);
  lcd.print(" C");

  //delay(5000);

  //lcd.clear();

  lcd.setCursor(8,0);
  lcd.print("DHT: ");
  lcd.print(temperaturaD);
  lcd.print(" C");

  lcd.setCursor(8,1);
  lcd.print("H A: ");
  lcd.print(humedadD);
  lcd.print("%");

  // Enviar los datos a ThingSpeak
  ThingSpeak.setField(1, humedadSuelo);
  ThingSpeak.setField(2, temperatura);
  ThingSpeak.setField(3, temperaturaD);
  ThingSpeak.setField(4, humedadD);
  ThingSpeak.writeFields(channelID, writeAPIKey);

  Serial.println("Datos enviados a ThingSpeak.");

  delay(2000);
}