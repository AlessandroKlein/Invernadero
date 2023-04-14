#include "ThingSpeak.h"
#include "WiFi.h"
#include <WebServer.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <MQUnifiedsensor.h>
//#include <Adafruit_Sensor.h>
//#include <Adafruit_BMP280.h>
//################################# Datos para modificar #####################################
//___________________________ Definicion de los pines ________________________________________
// 22- 21
#define humedad_pin 15     // Pin del sensor de humedad del suelo
#define humedad_dos_pin 2  // Pin del sensor de humedad del suelo 2
#define Lm35pin 0          // Pin del sensor de temperatura LM35DZ
#define dht_22 4           // Pin del sensor de temperatura y humedad DHT22
#define sensormq9 16       // Define el pin analógico donde está conectado el sensor MQ-9
#define buttonPin 13       // Define el pin del boton para cambiar el menu de la pantalla
// Rele pines
#define bombapin 36        // Pin del relé 1 (Bomba de agua)
#define bombadospin 39     // Pin del relé 2 (Bomba de agua 2)
#define luzpin 34          // Pin del relé 3 (Iluminación)
#define ventilacionpin 35  // Pin del relé 4 (Ventilación)

//________________________Definicion de los tiempo de actualizacion__________________________
#define sensoresactua 500           // Tiempo de actualizacion de los sensores
#define acHumedad 500               // Tiempo de actualizacion de la Humedad
#define acTemperatura 500           // Tiempo de actualizacion de la Temperatura interna
#define acLuz 500                   // Tiempo de actualizacion de la Luz
#define acLCD 1000                  // Tiempo de actualizacion del LCD
#define button_unlag 100            // Para evitar errores de pulsados
#define acThingSpeak 2000           // Tiempo de actualizacion de ThingSpeak
#define actualizacion_h 2000        // Tiempo de actualizacion de la hora en el serial
#define actualizacion_web_tiempo 5  // Tiempo que pasa para la actualizacion de la pagina web
#define menor_porcentaje 60         // Establece la el % de humedad del suelo minimo para que empiece la bomba
#define mayor_porcentaje 70         // Establece la el % de humedad del suelo maximo para que pare la bomba
#define horaamanecer 8              // Establece la hora del amanecer para la iluminacion (Sin poner minutos)
#define horaanochecer 18            // Establece la hora del anochecer para la iluminacion (Sin poner minutos)

//___________________________________________________________________________________________
const char* ssid = "ssid";                // SSID del router.
const char* password = "password";        // Contraseña del router.
const char* timeServer = "pool.ntp.org";  // Servidor NTP para el módulo de tiempo real.
const long gmtOffset_sec = -10800;        // Zona horaria (en segundos).
const int daylightOffset_sec = 0;         // Desfase horario de horario de verano (en segundos).
//___________________________________________________________________________________________
unsigned long channelID = 999999;         // ID del canal.
const char* writeAPIKey = "WriteAPIKey";  // Write API Key del canal.

//############################################################################################

//___________________________________________________________________________________________
unsigned long previousMillis = 0;
unsigned long actualizacion_web = 0;   // variable para almacenar el tiempo de la última vez que se actualizó la pagina web
unsigned long lcdMillis = 0;           // variable para almacenar el tiempo de la última vez que se actualizó el lcd
unsigned long ThingSpeakMillis = 0;    // variable para almacenar el tiempo de la última vez que se actualizó el ThingSpeak
unsigned long luzMillis = 0;           // variable para almacenar el tiempo de la última vez que se actualizó la luz
unsigned long humedadMillis = 0;       // variable para almacenar el tiempo de la última vez que se actualizó la humedad
unsigned long temperaturaMillis = 0;   // variable para almacenar el tiempo de la última vez que se actualizó la temperatura para el ventilador
unsigned long actualsensores_ = 0;     // variable para almacenar el tiempo de la última vez que se actualizaron los sensores
unsigned long actualizacion_hora = 0;  // variable para almacenar el tiempo de la última vez que se actualizón la hora en el serial
unsigned long lcdMillis_buton_lag = 0;
unsigned long lcdMillis_buton_while = 0;
//___________________________________________________________________________________________
int buttonState = 0;        // variable para almacenar el estado actual del botón
int menuOption = 1;         // variable para almacenar la opción de menú actual
int hora = 0;               // Guardar los datos de hora
char strftime_buf[64];      // Guardar los datos de hora
float humedadSuelo = 0;     // Guardar datos de la humedad del suelo
float humedadSuelodos = 0;  // Guardar datos de la humedad del suelo
float temperatura = 0;      // Guardar datos de la temperatura
float temperaturaD = 0;     // Guardar datos de la temperatura del DHT 22
float humedadD = 0;         // Guardar datos de la humedad del DHT 22
float ppm = 0;              // Guardar datos del valor de CO

//______________________________________MQUnifiedsensor______________________________________
/************************Macros relacionadas con el hardware**************************/
#define Board ("ESP-32")
#define Pin (sensormq9)  //Entrada analógica 4 de tu arduino
/***********************Macros relacionadas con el software**************************/
#define Type ("MQ-9")  //MQ9
#define Voltage_Resolution (3.3)
#define ADC_Bit_Resolution (10)  // For arduino UNO/MEGA/NANO
#define RatioMQ9CleanAir (9.83)  //RS / R0 = 9.83 ppm
/*****************************Globales***********************************************/
//Declarar sensor
MQUnifiedsensor MQ9(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin, Type);
//___________________________________________________________________________________________

WiFiClient client;

DHT dht(dht_22, DHT22);  // Crear objeto DHT

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Dirección del display en el bus I2C y tamaño del display 16x2
//LiquidCrystal_I2C lcd(0x27, 20, 4);  // Dirección de la pantalla en el bus I2C y tamaño 20x4

// Configuración del servidor web
WebServer server(80);

//Adafruit_BMP280 bmp;  // Defino BMP280

//___________________________________________________________________________________________
void setup() {
  Serial.begin(115200);
  Serial.println("Test de sensores:");

  // Conexión a la red Wi-Fi
  Serial.println("Conectando a la red Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conexión a la red Wi-Fi fallida. Intentando de nuevo...");
    WiFi.begin(ssid, password);
  }
  Serial.println("Conexión a la red Wi-Fi establecida.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Inicializar la comunicación con ThingSpeak
  ThingSpeak.begin(client);

  // Inicializar el sensor de humedad del suelo
  pinMode(humedad_pin, INPUT);
  pinMode(humedad_dos_pin, INPUT);

  // Inicializar la entrada del boton
  pinMode(buttonPin, INPUT_PULLUP);  // configurar el pin del botón como entrada con resistencia pull-up

  // Inicializar el sensor de temperatura LM35DZ
  pinMode(Lm35pin, INPUT);

  // Inicializar los pines de relés
  pinMode(bombapin, OUTPUT);
  pinMode(bombadospin, OUTPUT);
  pinMode(luzpin, OUTPUT);
  pinMode(ventilacionpin, OUTPUT);
  digitalWrite(bombapin, LOW);        // Inicializar los reles en modo apagado
  digitalWrite(bombadospin, LOW);     // Inicializar los reles en modo apagado
  digitalWrite(luzpin, LOW);          // Inicializar los reles en modo apagado
  digitalWrite(ventilacionpin, LOW);  // Inicializar los reles en modo apagado

  // Inicializar el sensor de temperatura y humedad DHT22
  dht.begin();

  //Inicializo el BMP280
  //BMP280_inicio();

  // Inicializar el módulo de tiempo real
  configTime(gmtOffset_sec, daylightOffset_sec, timeServer);
  Serial.println("Configuración de hora completa.");

  // MQ9
  //Establezca un modelo matemático para calcular la concentración de PPM y el valor de las constantes
  MQ9.setRegressionMethod(1);  //_PPM =  a*ratio^b
  MQ9.setA(36974);
  MQ9.setB(-3.109);  // Configure la ecuación para calcular la concentración de H2
  /*
    Exponential regression:
    Gas    | a      | b
    H2     | 987.99 | -2.162
    LPG    | 574.25 | -2.222
    CO     | 36974  | -3.109
    Alcohol| 3616.1 | -2.675
    Propane| 658.71 | -2.168
  */
  MQ9.init();
  mqsensor();

  // Inicializar el display LCD
  lcd.init();       // Inicializar el display
  lcd.backlight();  // Encender la luz de fondo
}
//___________________________________________________________________________________________

void loop() {
  unsigned long currentMillis = millis();  // obtener el tiempo actual
  // Manejo de solicitudes al servidor web
  if (currentMillis - actualizacion_web >= actualizacion_web_tiempo) {  // si ha pasado el tiempo del intervalo
    actualizacion_web = currentMillis;                                  // guardar el tiempo actual como la última vez que se actualizó
    server.handleClient();
  }
  // Obtener la hora actual
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Error al obtener la hora");
    return;
  }

  if (currentMillis - actualizacion_hora >= actualizacion_h) {  // si ha pasado el tiempo del intervalo
    actualizacion_hora = currentMillis;                         // guardar el tiempo actual como la última vez que se actualizó
    strftime(strftime_buf, sizeof(strftime_buf), "%H:%M:%S", &timeinfo);
    Serial.println(strftime_buf);
  }

  // Leer la humedad del suelo
  if (currentMillis - actualsensores_ >= sensoresactua) {  // si ha pasado el tiempo del intervalo
    actualsensores_ = currentMillis;                       // guardar el tiempo actual como la última vez que se actualizó

    humedad_del_suelo();  // Llamo a la funcion donde estan la lectura de la humedad del suelo
    temperatura_lm35();   // Llamo a la funcion donde estan la lectura de la temperatura interna
    temperatura_dht22();  // Llamo a la funcion donde estan la lectura la lectura de temperatura y humedad del dht22
    MQ9_carbono();        // Llamo a la funcion donde estan la lectura la lectura de nivel del monoxio de carbono del MQ9
    //Adafruit_BMP280_();        // Llamo a la funcion donde estan la lectura la lectura de nivel de precion y altitud
  }

  if (currentMillis - humedadMillis >= acHumedad) {  // si ha pasado el tiempo del intervalo
    humedadMillis = currentMillis;                   // guardar el tiempo actual como la última vez que se actualizó

    humedad_suelo();  // Llamo a la funcion encargada del riego automatico
  }

  if (currentMillis - luzMillis >= acLuz) {  // si ha pasado el tiempo del intervalo
    luzMillis = currentMillis;               // guardar el tiempo actual como la última vez que se actualizó

    hora = timeinfo.tm_hour;
    iluminacion();  // Llamo a la funcion encargada de la iluminacion
  }

  // Compruebo si la temperatura es óptima o no para encender o pagar el ventilador
  if (currentMillis - temperaturaMillis >= acTemperatura) {  // si ha pasado el tiempo del intervalo
    temperaturaMillis = currentMillis;                       // guardar el tiempo actual como la última vez que se actualizó

    ventilacion();  // Llamo a la funcion encargada de la ventilacion
  }

  // Mostrar la información en el display LCD
  //*****************************************
  switch (menuOption) {
    case 1:
      // mostrar el menú 1
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Menu 1");
      // Menu de Encendido o apagado de la luces y bomba
      if (currentMillis - lcdMillis >= acLCD) {  // si ha pasado el tiempo del intervalo
        lcdMillis = currentMillis;               // guardar el tiempo actual como la última vez que se actualizó
        lcd_menu1();
      }
      break;

    case 2:
      // mostrar el menú 2
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Menu 1");
      // Menu de  Humedad del suelo y temperatura
      if (currentMillis - lcdMillis >= acLCD) {  // si ha pasado el tiempo del intervalo
        lcdMillis = currentMillis;               // guardar el tiempo actual como la última vez que se actualizó
        lcd_menu2();
      }
      break;

    case 3:
      // mostrar el menú 3
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Menu 3");
      // Menu de DHT22 De temperatura y humedad
      if (currentMillis - lcdMillis >= acLCD) {  // si ha pasado el tiempo del intervalo
        lcdMillis = currentMillis;               // guardar el tiempo actual como la última vez que se actualizó
        lcd_menu3();
      }
      break;

    case 4:
      // mostrar el menú 4
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Menu 4");
      // Menu de MQ9 de CO2
      if (currentMillis - lcdMillis >= acLCD) {  // si ha pasado el tiempo del intervalo
        lcdMillis = currentMillis;               // guardar el tiempo actual como la última vez que se actualizó
        lcd_menu4();
      }
      break;
    /*case 5:
      // mostrar el menú 5
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Menu 5");
      // Menu de BMP280 de Presión, Altitud, Temperatura
      if (currentMillis - lcdMillis >= acLCD) {  // si ha pasado el tiempo del intervalo
        lcdMillis = currentMillis;               // guardar el tiempo actual como la última vez que se actualizó
        lcd_menu5();
      }
      break;*/
  }

  // leer el estado del botón
  buttonState = digitalRead(buttonPin);

  // esperar a que se suelte el botón
  while (buttonState == LOW) {
    if (currentMillis - lcdMillis_buton_while >= 50) {  // si ha pasado el tiempo del intervalo
      lcdMillis_buton_while = currentMillis; 
      buttonState = digitalRead(buttonPin);
    delay(50);
    }
  }

  // esperar un breve momento para evitar lecturas múltiples del botón
  if (currentMillis - lcdMillis_buton_lag >= button_unlag) {  // si ha pasado el tiempo del intervalo
    lcdMillis_buton_lag = currentMillis;                      // guardar el tiempo actual como la última vez que se actualizó

    // si se presiona el botón, avanzar al siguiente menú
    if (buttonState == HIGH) {
      menuOption++;
      if (menuOption > 4) {
        menuOption = 1;
      }
    }
  }
  //*****************************************

  // Llamo a la variable para cargar los datos a ThingSpeak
  if (currentMillis - ThingSpeakMillis >= acThingSpeak) {  // si ha pasado el tiempo del intervalo
    ThingSpeakMillis = currentMillis;                      // guardar el tiempo actual como la última vez que se actualizó

    // Enviar los datos a ThingSpeak
    paginaweb();  // Llamo a la funcion de ThingSpeak
  }
  //delay(2000);
}

//_________________________________Inicializacion de sensores_________________________________

void mqsensor() {
  Serial.print("Calibrando por favor espere.");
  float calcR0 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ9.update();  // Actualice los datos, el arduino leerá el voltaje del pin analógico
    calcR0 += MQ9.calibrate(RatioMQ9CleanAir);
    Serial.print(".");
  }
  MQ9.setR0(calcR0 / 10);
  Serial.println("  hecho!.");

  if (isinf(calcR0)) {
    Serial.println("Advertencia: Problema de conexión, R0 es infinito (se detectó un circuito abierto), verifique su cableado y suministro");
    while (1)
      ;
  }
  if (calcR0 == 0) {
    Serial.println("Advertencia: se encontró un problema de conexión, R0 es cero (el pin analógico se corta a tierra), verifique su cableado y suministro");
    while (1)
      ;
  }
}

void pagina_web() {
  // Configuración del servidor web
  server.on("/", HTTP_GET, []() {
    String html = "<html><head><title>Invernadero</title></head>";  // Establece el nombre de la pagina
    html += "<p>Hora: ";
    html += String(strftime_buf);
    html += "<body><h1>Humedad del suelo </h1>";
    html += "<p>Humedad del suelo Sensor 1: ";
    //html += String(ThingSpeak.readFloatField(channelID, 1));
    html += String(analogRead(humedad_pin));
    html += "<p>Humedad del suelo Sensor 2: ";
    html += String(analogRead(humedad_dos_pin));
    html += "<body><h1>Temperatura LM35</h1>";
    html += " C</p><p>Temperatura: ";
    //html += String(ThingSpeak.readFloatField(channelID, 2));
    html += String((analogRead(Lm35pin) * 3.3) / 4095.0);
    html += "<body><h1>Temperatura y humedad DHT22</h1>";
    html += " C</p><p>Temperatura DHT: ";
    //html += String(ThingSpeak.readFloatField(channelID, 3));
    html += String(dht.readTemperature());
    html += " C</p><p>Humedad DHT: ";
    //html += String(ThingSpeak.readFloatField(channelID, 4));
    html += String(dht.readHumidity());
    html += "<body><h1>Monóxido de carbono MQ9</h1>";
    html += " C</p><p>Monóxido de carbono: ";
    //html += String(ThingSpeak.readFloatField(channelID, 5));
    html += String(MQ9.readSensor());
    html += " %</p></body></html>";
    server.send(200, "text/html", html);
  });
  // Servidor no funciona
  server.onNotFound(handleNotFound);
  // Inicio del servidor web
  server.begin();
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

/*void BMP280_inicio(){
  if (!bmp.begin(0x76)) {
    Serial.println("No se pudo encontrar el sensor BMP280.");
    while (1);
  }  
}*/

//__________________________________________Sensores_________________________________________
void humedad_del_suelo() {
  // Leer la humedad del sensor LM393
  float humedadSuelo_ = analogRead(humedad_pin);
  humedadSuelo = humedadSuelo_;
  float humedadSuelo_dos = analogRead(humedad_dos_pin);
  humedadSuelodos = humedadSuelo_dos;

  Serial.print("Humedad del suelo Sensor 1: ");
  Serial.println(humedadSuelo);
  Serial.print("Humedad del suelo Sensor 2: ");
  Serial.println(humedadSuelodos);
}
void temperatura_lm35() {
  // Leer la temperatura del sensor LM35DZ
  float temperatura_ = (analogRead(Lm35pin) * 3.3) / 4095.0;
  temperatura_ = temperatura_ * 100;
  temperatura = temperatura_;

  Serial.print("Temperatura LM35DZ: ");
  Serial.println(temperatura_);
}
void temperatura_dht22() {
  // Leer la temperatura y humedad del sensor DHT22
  float temperaturaD_ = dht.readTemperature();
  temperaturaD = temperaturaD_;
  float humedadD_ = dht.readHumidity();
  humedadD = humedadD_;

  Serial.print("Temperatura DHT22: ");
  Serial.println(temperaturaD_);
  Serial.print("Humedad DHT22: ");
  Serial.println(humedadD_);
}
void MQ9_carbono() {
  // Lee el valor de CO en ppm
  MQ9.update();  // Actualice los datos, el arduino leerá el voltaje del pin analógico
  //MQ9.serialDebug(); // Imprimirá la tabla en el puerto serie
  float ppm_ = MQ9.readSensor();  // El sensor leerá la concentración de PPM usando el modelo, los valores a y b establecidos previamente o desde la configuración
  ppm = ppm_;
  Serial.print("Valor de CO: ");
  Serial.print(ppm_);
  Serial.println(" ppm");
}
/*void Adafruit_BMP280_() {
  Serial.print("Temperatura = ");
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");

  Serial.print("Presión = ");
  Serial.print(bmp.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Altitud = ");
  Serial.print(bmp.readAltitude(1013.25)); // altitud estándar al nivel del mar
  Serial.println(" m");
}*/
//___________________________________________________________________________________________

void humedad_suelo() {
  // Controlar la bomba de agua en base a la humedad del suelo
  if (humedadSuelo < menor_porcentaje) {
    digitalWrite(bombapin, LOW);
  } else if (humedadSuelo > mayor_porcentaje) {
    digitalWrite(bombapin, HIGH);
  }
  if (humedadSuelodos < menor_porcentaje) {
    digitalWrite(bombadospin, LOW);
  } else if (humedadSuelodos > mayor_porcentaje) {
    digitalWrite(bombadospin, HIGH);
  }
}
//___________________________________________________________________________________________

void iluminacion() {
  // Controlar la iluminación en base a la hora del día
  if (hora >= horaamanecer && hora < horaanochecer) {
    digitalWrite(luzpin, HIGH);
  } else {
    digitalWrite(luzpin, LOW);
  }
}
//___________________________________________________________________________________________

void ventilacion() {
  // Controlar la ventilación en base a la temperatura
  if (temperatura > 25) {
    digitalWrite(ventilacionpin, HIGH);
  } else if (temperatura < 20) {
    digitalWrite(ventilacionpin, LOW);
  }
}
//___________________________________________________________________________________________
// Menu de Encendido o apagado de la luces y bomba
void lcd_menu1() {
  //Menu 1
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Bomba: ");
  if (digitalRead(bombapin) == 1) {
    lcd.print("On");
  } else {
    lcd.print("Off");
  }

  lcd.setCursor(0, 0);
  lcd.print("Luz: ");
  if (digitalRead(luzpin) == 1) {
    lcd.print("On");
  } else {
    lcd.print("Off");
  }
}
// Menu de  Humedad del suelo y temperatura
void lcd_menu2() {
  //Menu 2
  lcd.clear();

  lcd.setCursor(0, 1);
  lcd.print("Hum Suelo: ");
  lcd.print(humedadSuelo);
  lcd.print("%");

  lcd.setCursor(0, 0);
  lcd.print("Temp.: ");
  lcd.print(temperatura);
  lcd.print(" C");
}
// Menu de DHT22 De temperatura y humedad
void lcd_menu3() {
  //Menu 3
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Temp.: ");
  lcd.print(temperaturaD);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Hum Anviente: ");
  lcd.print(humedadD);
  lcd.print("%");
}
// Menu de MQ9 de CO2
void lcd_menu4() {
  //Menu 4
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("CO2.: ");
  lcd.print(ppm);
  lcd.print(" PPM");

  //lcd.setCursor(0,1);
  //lcd.print("Hum Anviente: ");
  //lcd.print(humedadD);
  //lcd.print("%");
}
// Menu de BMP280 de Presión, Altitud, Temperatura
/*void lcd_menu5(){
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Alt.: ");
  lcd.print(bmp.readAltitude(1013.25));
  lcd.print(" M");

  lcd.setCursor(7, 0);
  lcd.print("Tem.: ");
  lcd.print(bmp.readTemperature());
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Presion.: ");
  lcd.print(bmp.readPressure() / 100.0F);
  lcd.print(" hPa");
}*/
//___________________________________________________________________________________________

void paginaweb() {
  ThingSpeak.setField(1, humedadSuelo);
  ThingSpeak.setField(2, humedadSuelodos);
  ThingSpeak.setField(3, temperatura);   //LM35
  ThingSpeak.setField(4, temperaturaD);  //DHT22
  ThingSpeak.setField(5, humedadD);      //DHT22
  ThingSpeak.setField(6, ppm);           //CO2
  ThingSpeak.writeFields(channelID, writeAPIKey);

  Serial.println("Datos enviados a ThingSpeak.");
}
//___________________________________________________________________________________________