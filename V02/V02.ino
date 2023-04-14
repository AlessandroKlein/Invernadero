// Incluye las bibliotecas necesarias
#include <WiFi.h>
#include <ThingSpeak.h>
#include <Wire.h>
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

    // Lee el valor del sensor de humedad del suelo
    soilMoistureValue = analogRead(soilMoisturePin);

    // Activa la bomba de agua si el suelo está seco
    if (soilMoistureValue < soilMoistureThreshold)
    {
        digitalWrite(pumpRelayPin, HIGH);
    }
    else
    {
        digitalWrite(pumpRelayPin, LOW);
    }

    // Lee el valor del sensor de temperatura
    tempSensorValue = (analogRead(tempSensorPin) * 0.48875);

    // Activa el ventilador si la temperatura es alta
    if (tempSensorValue > tempThreshold)
    {
        digitalWrite(fanRelayPin, HIGH);
    }
    else
    {
        digitalWrite(fanRelayPin, LOW);
    }

    // Activa la iluminación durante las horas especificadas
    if (now.hour() >= lightOnHour && now.hour() < lightOffHour)
    {
        digitalWrite(lightRelayPin, HIGH);
    }
    else
    {
        digitalWrite(lightRelayPin, LOW);
    }

    // Muestra la información en la pantalla LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Humedad: ");
    lcd.print(soilMoistureValue);
    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    lcd.print(tempSensorValue);
    lcd.print(" C");

    // Envía los datos a ThingSpeak cada 5 minutos
    if (millis() - lastUpdateTime > 300000)
    {
        lastUpdateTime = millis();
        ThingSpeak.writeField(myChannelNumber, 1, soilMoistureValue, myWriteAPIKey);
        ThingSpeak.writeField(myChannelNumber, 2, tempSensorValue, myWriteAPIKey);
    }
}