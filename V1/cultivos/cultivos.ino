#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include <ThingSpeak.h>

// WiFi credentials
const char* ssid = "TU_SSID";
const char* password = "TU_CLAVE_WIFI";

// ThingSpeak API credentials
String apiKey = "TU_API_KEY";
const char* server = "api.thingspeak.com";

// Sensor pins
const int soilMoisturePin = 34; // YL-69 sensor
const int waterPumpPin = 14; // Relay 1
const int lightPin = 27; // Relay 2
const int fanPin = 26; // Relay 3
const int tempPin = 32; // LM35 sensor

// LCD module pins
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Time variables
unsigned long previousMillis = 0;
const long interval = 300000; // 5 minutes

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Starting...");
  
  // Initialize relays
  pinMode(waterPumpPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
  pinMode(fanPin, OUTPUT);

  // Set initial state of relays
  digitalWrite(waterPumpPin, HIGH);
  digitalWrite(lightPin, HIGH);
  digitalWrite(fanPin, HIGH);
}

void loop() {
  // Check if it's time to update ThingSpeak
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    updateThingSpeak();
  }

  // Read soil moisture sensor
  int soilMoisture = analogRead(soilMoisturePin);

  // Control water pump based on soil moisture level
  if (soilMoisture < 60) {
    digitalWrite(waterPumpPin, LOW);
  } else if (soilMoisture > 70) {
    digitalWrite(waterPumpPin, HIGH);
  }

  // Read temperature sensor
  float temp = (analogRead(tempPin) * 3.3) / 4095;
  temp = (temp - 0.5) * 100;

  // Control fan based on temperature level
  if (temp > 25) {
    digitalWrite(fanPin, LOW);
  } else if (temp < 20) {
    digitalWrite(fanPin, HIGH);
  }

  // Control light based on time of day
  int hour = getHour();
  if (hour >= 6 && hour <= 18) {
    digitalWrite(lightPin, HIGH);
  } else {
    digitalWrite(lightPin, LOW);
  }

  // Update LCD display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(soilMoisture);
  lcd.print("%");
  delay(1000);
}

void updateThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Build the URL for the API request
    String url = "http://" + String(server) + "/update?api_key=" + apiKey + "&field1=" + String(analogRead(soilMoisturePin));
    // Send the API request and get the response
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      Serial.println("Data sent to ThingSpeak successfully.");
    } else {
      Serial.print("Error sending data to ThingSpeak. HTTP error code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Not connected to WiFi. Cannot update ThingSpeak.");
  }
}

int getHour() {
  time_t now = time(nullptr);
  struct tm* timeInfo = localtime(&now);
  return timeInfo->tm_hour;
}
