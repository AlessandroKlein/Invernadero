// Código completo para el controlador de cultivo automatizado

// Incluye las bibliotecas necesarias
#include <WiFi.h>
#include <ThingSpeak.h>
#include "Wire.h"
#include <LiquidCrystal_I2C.h>

// Configuración de WiFi
const char *ssid = "tu_ssid";
const char *password = "tu_password";

// Configuración de ThingSpeak
unsigned long myChannelNumber = 2105955;         // Reemplaza con tu número de canal de ThingSpeak
const char *myWriteAPIKey = "WI8RVSQ0Y9SSRCJL"; // Reemplaza con tu clave de escritura de API de ThingSpeak

// Configuración del módulo de tiempo real
RTC_DS1307 rtc;

// Configuración del sensor de humedad del suelo
int soilMoisturePin = 34;        // Pin del sensor de humedad del suelo
int soilMoistureValue = 0;       // Valor de lectura del sensor de humedad del suelo
int soilMoistureThreshold = 600; // Umbral de humedad del suelo (ajústalo según sea necesario)
int pumpRelayPin = 26;           // Pin del relé de la bomba de agua

// Configuración del sensor de temperatura
int tempSensorPin = 35;      // Pin del sensor de temperatura
float tempSensorValue = 0.0; // Valor de lectura del sensor de temperatura
int tempThreshold = 25;      // Umbral de temperatura (ajústalo según sea necesario)
int fanRelayPin = 25;        // Pin del relé del ventilador

// Configuración del relé de iluminación
int lightRelayPin = 27; // Pin del relé de iluminación
int lightOnHour = 6;    // Hora de inicio de la iluminación
int lightOffHour = 18;  // Hora de finalización de la iluminación

// Configuración de la pantalla LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // Dirección I2C de la pantalla LCD

unsigned long lastUpdateTime = 0;

void setup()
{
    // Inicializa el puerto serie
    Serial.begin(9600);

    // Inicializa el módulo WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Conectando a WiFi...");
    }
    Serial.println("Conectado a WiFi.");

    // Inicializa ThingSpeak
    ThingSpeak.begin(client);

    // Inicializa el módulo de tiempo real
    Wire.begin();
    rtc.begin();
    rtc.adjust(DateTime(F(DATE), F(TIME))); // Ajusta el reloj a la hora actual

    // Configura los pines del relé
    pinMode(pumpRelayPin, OUTPUT);
    pinMode(fanRelayPin, OUTPUT);
    pinMode(lightRelayPin, OUTPUT);

    // Inicializa la pantalla LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Controlador de");
    lcd.setCursor(0, 1);
    lcd.print("Cultivo Automa.");
    delay(2000);
}

void loop()
{
    // Obtiene la fecha y hora actual
    DateTime now = rtc.now();

    // Lee el valor delsensor de humedad del suelo
    soilMoistureValue = analogRead(soilMoisturePin);
    // Enciende o apaga la bomba de agua según el umbral de humedad
    if (soilMoistureValue < soilMoistureThreshold)
    {
        digitalWrite(pumpRelayPin, HIGH); // Enciende la bomba de agua
    }
    else
    {
        digitalWrite(pumpRelayPin, LOW); // Apaga la bomba de agua
    }

    // Lee el valor del sensor de temperatura
    tempSensorValue = analogRead(tempSensorPin);
    tempSensorValue = (5.0 * tempSensorValue * 100.0) / 1024.0; // Convierte el valor a grados Celsius

    // Enciende o apaga el ventilador según el umbral de temperatura
    if (tempSensorValue > tempThreshold)
    {
        digitalWrite(fanRelayPin, HIGH); // Enciende el ventilador
    }
    else
    {
        digitalWrite(fanRelayPin, LOW); // Apaga el ventilador
    }

    // Enciende o apaga la iluminación según la hora del día
    if (now.hour() >= lightOnHour && now.hour() < lightOffHour)
    {
        digitalWrite(lightRelayPin, HIGH); // Enciende la iluminación
    }
    else
    {
        digitalWrite(lightRelayPin, LOW); // Apaga la iluminación
    }

    // Actualiza los valores en la pantalla LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Humedad: ");
    lcd.print(soilMoistureValue);
    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    lcd.print(tempSensorValue);
    lcd.print(" C");

    // Actualiza ThingSpeak cada 5 minutos
    if (millis() - lastUpdateTime > 300000)
    {
        lastUpdateTime = millis();

        // Actualiza los campos en ThingSpeak
        ThingSpeak.setField(1, soilMoistureValue);
        ThingSpeak.setField(2, tempSensorValue);
        ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    }

    delay(1000); // Espera 1 segundo antes de volver a leer los
}