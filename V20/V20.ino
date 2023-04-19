#include "ThingSpeak.h"
#include "WiFi.h"
#include <WebServer.h>
#include "DHT.h"
#include <WebSocketsServer.h>
#include <LiquidCrystal_I2C.h>
#include <MQUnifiedsensor.h>
//#include <Adafruit_Sensor.h>
//#include <Adafruit_BMP280.h>
#include <EEPROM.h>
//################################# Datos para modificar #####################################
//___________________________ Definicion de los pines ________________________________________
// 22- 21
#define humedad_pin 15       // Pin del sensor de humedad del suelo // Pin analógica 
#define humedad_vege_pin 2   // Pin del sensor de humedad de Vegetacion // Pin analógica 
#define humedad_prin_flora_pin 0  // Pin del sensor de humedad de Flora // Pin analógica 
#define humedad_flora_pin 5  // Pin del sensor de humedad de Flora // Pin analógica 
#define Lm35pin 4            // Pin del sensor de temperatura LM35DZ // Pin analógica 
#define dht_22 16            // Pin del sensor de temperatura y humedad DHT22
#define sensormq9 17         // Define el pin analógico donde está conectado el sensor MQ-9 // Pin analógica 
// Boton
#define buttonPin 13  // Define el pin del boton para cambiar el menu de la pantalla
// Rele pines
#define bombapin 36        // Pin del relé 1 (Bomba de agua) // Etapa de Vegetacion
#define bombadospin 39     // Pin del relé 2 (Bomba de agua 2)  
#define bombatrespin 34    // Pin del relé 3 (Bomba de agua 3)
#define bombacuatropin 33  // Pin del relé 4 (Bomba de agua 4)
#define luzpin 35          // Pin del relé 5 (Iluminación)
#define ventilacionpin 32  // Pin del relé 6 (Ventilación)

//________________________Definicion de los tiempo de actualizacion__________________________
#define WEBSOCKETS_actualizacion 3000// El valor 3000 es el indicado por el tipo de precision que se quiere, si no se quiere tal precision paselo a 5000
#define sensoresactua 500           // Tiempo de actualizacion de los sensores
#define acHumedad 500               // Tiempo de actualizacion de la Humedad
#define acTemperatura 500           // Tiempo de actualizacion de la Temperatura interna
#define acLuz 500                   // Tiempo de actualizacion de la Luz
#define acLCD 1000                  // Tiempo de actualizacion del LCD
#define button_unlag 100            // Para evitar errores de pulsados
#define acThingSpeak 2000           // Tiempo de actualizacion de ThingSpeak
#define actualizacion_h 2000        // Tiempo de actualizacion de la hora en el serial
#define actualizacion_web_tiempo 5  // Tiempo que pasa para la actualizacion de la pagina web
//________________________ Datos de % de humedad del suelo __________________________
// Crecimiento
int menor_porcentaje = 80;  // Establece la el % de humedad del suelo minimo para que empiece la bomba
int mayor_porcentaje = 90;  // Establece la el % de humedad del suelo maximo para que pare la bomba
// Vegetacion
int menor_porcentaje_vege = 70;  // Establece la el % de humedad del suelo minimo para que empiece la bomba
int mayor_porcentaje_vege = 80;  // Establece la el % de humedad del suelo maximo para que pare la bomba
// Flora
int menor_porcentaje_Prin_flora = 40;  // Establece la el % de humedad del suelo minimo para que empiece la bomba
int mayor_porcentaje_prin_flora = 50;  // Establece la el % de humedad del suelo maximo para que pare la bomba

int menor_porcentaje_flora = 40;  // Establece la el % de humedad del suelo minimo para que empiece la bomba
int mayor_porcentaje_flora = 40;  // Establece la el % de humedad del suelo maximo para que pare la bomba
//________________________ Datos de temperatura __________________________
int temperatura_mayor = 25;
int temperatura_menor = 20;
//________________________ Dartos para el siclo solar __________________________
int horaamanecer = 8;    // Establece la hora del amanecer para la iluminacion (Sin poner minutos)
int horaanochecer = 18;  // Establece la hora del anochecer para la iluminacion (Sin poner minutos)

//___________________________________________________________________________________________
String ssid = "";                         // SSID de la red WiFi
String password = "";                     // Contraseña de la red WiFi
const char* timeServer = "pool.ntp.org";  // Servidor NTP para el módulo de tiempo real.
const long gmtOffset_sec = -10800;        // Zona horaria (en segundos).
const int daylightOffset_sec = 0;         // Desfase horario de horario de verano (en segundos).
//___________________________________________________________________________________________
unsigned long channelID = 999999;         // ID del canal.
const char* writeAPIKey = "WriteAPIKey";  // Write API Key del canal.

//___________________________________________________________________________________________
#define IO_USERNAME "your_username"
#define IO_KEY "your_key"
//############################################################################################

bool shouldSaveConfig = false; // Bandera para indicar si se deben guardar los datos de la configuración en la memoria EEPROM
int soilHumidity = 0; // Porcentaje de humedad del suelo
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
unsigned long WEBSOCKETS_time = 0;
//___________________________________________________________________________________________
int buttonState = 0;           // variable para almacenar el estado actual del botón
int menuOption = 1;            // variable para almacenar la opción de menú actual
int hora = 0;                  // Guardar los datos de hora
char strftime_buf[64];         // Guardar los datos de hora
float humedad_plantin = 0;        // Guardar datos de la humedad del suelo
float humedad_vege = 0;   // Guardar datos de la humedad del suelo Vegetacion
float humedad_prin_flora_ = 0;  // Guardar datos de la humedad del suelo Flora
float humedad_flora = 0;  // Guardar datos de la humedad del suelo Flora
float temp_exterior = 0;         // Guardar datos de la temperatura
float temp_interior = 0;        // Guardar datos de la temperatura del DHT 22
float humedad_ambiente = 0;            // Guardar datos de la humedad del DHT 22
float ppm = 0;                 // Guardar datos del valor de CO

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

DHT dht(dht_22, DHT22);  // Crear objeto DHT

LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Dirección del display en el bus I2C y tamaño del display 16x2
//LiquidCrystal_I2C lcd(0x27, 20, 4);  // Dirección de la pantalla en el bus I2C y tamaño 20x4

// Configuración del servidor web
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
uint8_t clientID;
WiFiClient client;

//Adafruit_BMP280 bmp;  // Defino BMP280

//___________________________________________________________________________________________
void setup() {
  Serial.begin(115200);
  Serial.println("Test de sensores:");
  WiFi.begin();
  // Conexión a la red Wi-Fi
  if (WiFi.SSID() != "") {
    Serial.println("Conectando a la red WiFi guardada...");
    WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Conectando...");
    }
    Serial.println("Conectado a la red WiFi guardada");
  } else {
    // Si no existen datos de la configuración, crear una red WiFi generada automáticamente
    Serial.println("Creando una red WiFi para la configuración...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-AP", "password123"); // Nombre y contraseña de la red WiFi generada automáticamente
  }

  Serial.println("Conexión a la red Wi-Fi establecida.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  // Inicializar la comunicación con ThingSpeak
  ThingSpeak.begin(client);

  // Inicializar el sensor de humedad del suelo
  pinMode(humedad_pin, INPUT);
  pinMode(humedad_vege_pin, INPUT);
  pinMode(humedad_flora_pin, INPUT);

  // Inicializar la entrada del boton
  pinMode(buttonPin, INPUT_PULLUP);  // configurar el pin del botón como entrada con resistencia pull-up

  // Inicializar el sensor de temperatura LM35DZ
  pinMode(Lm35pin, INPUT);

  // Inicializar los pines de relés
  pinMode(bombapin, OUTPUT);
  pinMode(bombadospin, OUTPUT);
  pinMode(bombatrespin, OUTPUT);
  pinMode(bombacuatropin, OUTPUT);
  pinMode(luzpin, OUTPUT);
  pinMode(ventilacionpin, OUTPUT);
  digitalWrite(bombapin, LOW);        // Inicializar los reles en modo apagado
  digitalWrite(bombadospin, LOW);     // Inicializar los reles en modo apagado
  digitalWrite(bombatrespin, LOW);    // Inicializar los reles en modo apagado
  digitalWrite(bombacuatropin, LOW);    // Inicializar los reles en modo apagado
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

  // Configuración del servidor de WebSockets
  webSocket.onEvent(webSocketEvent);
  webSocket.begin();
  // Configurar las rutas del servidor web
  server.on("/config", handleRoot);
  server.on("/save", handleSave);
  server.begin();
  Serial.println("Servidor de WebSockets iniciado");  
}
//___________________________________________________________________________________________

void loop() {
  unsigned long currentMillis = millis();  // obtener el tiempo actual
  // Manejo de solicitudes al servidor web
  // Manejar los eventos del servidor de WebSockets
  webSocket.loop();
  server.handleClient();

  if (shouldSaveConfig) {
    // Guardar los datos de la configuración en la memoria EEPROM
    Serial.println("Guardando datos de la configuración en la memoria EEPROM...");
    EEPROM.begin(512);
    EEPROM.writeString(0, ssid);
    EEPROM.writeString(32, password);
    EEPROM.writeInt(64, mayor_porcentaje);
    EEPROM.writeInt(128, menor_porcentaje);
    EEPROM.writeInt(192, mayor_porcentaje_vege);
    EEPROM.writeInt(256, menor_porcentaje_vege);
    EEPROM.writeInt(320, mayor_porcentaje_prin_flora);
    EEPROM.writeInt(384, menor_porcentaje_Prin_flora);
    EEPROM.writeInt(448, mayor_porcentaje_flora);
    EEPROM.writeInt(512, menor_porcentaje_flora);
    EEPROM.writeInt(576, temperatura_mayor);
    EEPROM.writeInt(640, temperatura_menor);
    EEPROM.writeInt(704, horaamanecer);
    EEPROM.writeInt(768, horaanochecer);
    EEPROM.write(704, 1); // Indicador de que hay datos de la configuración guardados
    EEPROM.commit();
    EEPROM.end();
    Serial.println("Datos de la configuración guardados");
    shouldSaveConfig = false;
  }
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
  if (currentMillis - WEBSOCKETS_time >= WEBSOCKETS_actualizacion) {
    sendSensorData();
    WEBSOCKETS_time = currentMillis;
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

// Web local v2
void pagina_web() {
  String ptr = "<!DOCTYPE html>";
  ptr +="<html>";
  ptr +="<head>";
  ptr +="<title>ESP32 Weather Station</title>";
  ptr +="<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  ptr +="<link href='https://fonts.googleapis.com/css?family=Open+Sans:300,400,600' rel='stylesheet'>";
  ptr +="<style>";
  ptr +="html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #444444;}";
  ptr +="body{margin: 0px;} ";
  ptr +="h1 {margin: 50px auto 30px;} ";
  ptr +=".side-by-side{display: table-cell;vertical-align: middle;position: relative;}";
  ptr +=".text{font-weight: 600;font-size: 19px;width: 200px;}";
  ptr +=".reading{font-weight: 300;font-size: 50px;padding-right: 25px;}";
  ptr +=".temperature .reading{color: #F29C1F;}";
  ptr +=".temperature_ex .reading{color: #CD7900 ;}";
  ptr +=".humidity .reading{color: #3B97D3;}";
  ptr +=".humidity_suelo .reading{color: #DAF7A6;}";
  ptr +=".pressure .reading{color: #26B99A;}";
  ptr +=".altitude .reading{color: #955BA5;}";
  ptr +=".co2_color .reading{color: ##FFC300;}";
  ptr +=".superscript{font-size: 17px;font-weight: 600;position: absolute;top: 10px;}";
  ptr +=".data{padding: 10px;}";
  ptr +=".container{display: table;margin: 0 auto;}";
  ptr +=".icon{width:65px}";
  ptr +="</style>";
  //________________________________________________________
  ptr +="<script>\n";
  ptr += "var socket = new WebSocket(\"ws://\" + location.hostname + \":81/\");";
  ptr += "socket.onmessage = function(event) {\n";
  ptr += "  var data = JSON.parse(event.data);\n";
  ptr += "  document.getElementById(\"humedad_plantin_web\").innerHTML = data.humedad_plantin_web.toFixed(1);\n";
  ptr += "  document.getElementById(\"humedad_vege_web\").innerHTML = humedad_vege_web.toFixed(1);\n";
  ptr += "  document.getElementById(\"humedad_prin_flora_web\").innerHTML = data.humedad_prin_flora_web.toFixed(1);\n";
  ptr += "  document.getElementById(\"humedad_flora_web\").innerHTML = data.humedad_flora_web.toFixed(1);\n";
  ptr += "  document.getElementById(\"temp_interior_web\").innerHTML = data.temp_interior_web.toFixed(1);\n";
  ptr += "  document.getElementById(\"humedad_ambiente_web\").innerHTML = data.humedad_ambiente_web.toFixed(1);\n";
  ptr += "  document.getElementById(\"temp_interior_bmp280_web\").innerHTML = data.temp_interior_bmp280_web.toFixed(1);\n";
  ptr += "  document.getElementById(\"pres_interior_bmp280_web\").innerHTML = data.pres_interior_bmp280_web.toFixed(1);\n";
  ptr += "  document.getElementById(\"alti_interior_bmp280_web\").innerHTML = data.alti_interior_bmp280_web.toFixed(1);\n";
  ptr += "  document.getElementById(\"temp_exterior_web\").innerHTML = data.temp_exterior_web.toFixed(1);\n";
  ptr += "  document.getElementById(\"ppm_web\").innerHTML = data.co2_web.toFixed(1);\n";
  ptr += "}\n";
  ptr +="</script>\n";
  //________________________________________________________
  ptr +="</head>";
  ptr +="<body>";
  ptr +="<h1>Datos invernadero</h1>";
  ptr +="<div class='container'>";
  ptr +="<h2>Humedad del suelo</h2>";
  // Estado actual de la humedad del suelo durante la etapa de Plantin
  ptr +="<div class='data humidity_suelo'>";
  ptr +="<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 29.235 40.64' height='40.64px' id='Layer_1' version='1.1' viewBox='0 0 29.235 40.64' width='29.235px' x='0px' xml:space='preserve' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' y='0px'>";
  ptr += "<path d='M14.618,0C14.618,0,0,17.95,0,26.022C0,34.096,6.544,40.64,14.618,40.64s14.617-6.544,14.617-14.617";
  ptr += "C29.235,17.95,14.618,0,14.618,0z M13.667,37.135c-5.604,0-10.162-4.56-10.162-10.162c0-0.787,0.638-1.426,1.426-1.426";
  ptr += "c0.787,0,1.425,0.639,1.425,1.426c0,4.031,3.28,7.312,7.311,7.312c0.787,0,1.425,0.638,1.425,1.425";
  ptr += "C15.093,36.497,14.455,37.135,13.667,37.135z' fill='#3B97D3' />";
  ptr += "</svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Estado actual de la humedad del suelo durante la etapa de Plantin</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr += "<p><span id='humedad_plantin_web'>--</span> %</p>";
  if (digitalRead(bombapin) == 1) {
      ptr += "<h3>Bomba activa </h3>";
    } else {
      ptr += "<h3>Bomba apagada </h3>";;
    }  
  ptr +="<span class='superscript'>%</span></div>";
  ptr +="</div>";
  // Estado actual de la humedad del suelo durante la etapa de Vegetación
  ptr +="<div class='data humidity_suelo'>";
  ptr +="<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 29.235 40.64' height='40.64px' id='Layer_1' version='1.1' viewBox='0 0 29.235 40.64' width='29.235px' x='0px' xml:space='preserve' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' y='0px'>";
  ptr += "<path d='M14.618,0C14.618,0,0,17.95,0,26.022C0,34.096,6.544,40.64,14.618,40.64s14.617-6.544,14.617-14.617";
  ptr += "C29.235,17.95,14.618,0,14.618,0z M13.667,37.135c-5.604,0-10.162-4.56-10.162-10.162c0-0.787,0.638-1.426,1.426-1.426";
  ptr += "c0.787,0,1.425,0.639,1.425,1.426c0,4.031,3.28,7.312,7.311,7.312c0.787,0,1.425,0.638,1.425,1.425";
  ptr += "C15.093,36.497,14.455,37.135,13.667,37.135z' fill='#3B97D3' />";
  ptr += "</svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Estado actual de la humedad del suelo durante la etapa de Vegetación</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr += "<p><span id='humedad_vege_web'>--</span> %</p>";
  if (digitalRead(bombadospin) == 1) {
      ptr += "<h3>Bomba activa </h3>";
    } else {
      ptr += "<h3>Bomba apagada </h3>";;
    }  
  ptr +="<span class='superscript'>%</span></div>";
  ptr +="</div>";
  // Estado actual de la humedad del suelo durante la etapa de Vegetación
  ptr +="<div class='data humidity_suelo'>";
  ptr +="<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 29.235 40.64' height='40.64px' id='Layer_1' version='1.1' viewBox='0 0 29.235 40.64' width='29.235px' x='0px' xml:space='preserve' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' y='0px'>";
  ptr += "<path d='M14.618,0C14.618,0,0,17.95,0,26.022C0,34.096,6.544,40.64,14.618,40.64s14.617-6.544,14.617-14.617";
  ptr += "C29.235,17.95,14.618,0,14.618,0z M13.667,37.135c-5.604,0-10.162-4.56-10.162-10.162c0-0.787,0.638-1.426,1.426-1.426";
  ptr += "c0.787,0,1.425,0.639,1.425,1.426c0,4.031,3.28,7.312,7.311,7.312c0.787,0,1.425,0.638,1.425,1.425";
  ptr += "C15.093,36.497,14.455,37.135,13.667,37.135z' fill='#3B97D3' />";
  ptr += "</svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Estado actual de la humedad del suelo durante la etapa de Vegetación</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr += "<p><span id='humedad_prin_flora_web'>--</span> %</p>";
  if (digitalRead(bombatrespin) == 1) {
      ptr += "<h3>Bomba activa </h3>";
    } else {
      ptr += "<h3>Bomba apagada </h3>";;
    }  
  ptr +="<span class='superscript'>%</span></div>";
  ptr +="</div>";
  // Estado actual de la humedad del suelo durante la etapa de Vegetación
  ptr +="<div class='data humidity_suelo'>";
  ptr +="<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 29.235 40.64' height='40.64px' id='Layer_1' version='1.1' viewBox='0 0 29.235 40.64' width='29.235px' x='0px' xml:space='preserve' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' y='0px'>";
  ptr += "<path d='M14.618,0C14.618,0,0,17.95,0,26.022C0,34.096,6.544,40.64,14.618,40.64s14.617-6.544,14.617-14.617";
  ptr += "C29.235,17.95,14.618,0,14.618,0z M13.667,37.135c-5.604,0-10.162-4.56-10.162-10.162c0-0.787,0.638-1.426,1.426-1.426";
  ptr += "c0.787,0,1.425,0.639,1.425,1.426c0,4.031,3.28,7.312,7.311,7.312c0.787,0,1.425,0.638,1.425,1.425";
  ptr += "C15.093,36.497,14.455,37.135,13.667,37.135z' fill='#3B97D3' />";
  ptr += "</svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Estado actual de la humedad del suelo durante la etapa de Vegetación</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr += "<p><span id='humedad_flora_web'>--</span> %</p>";
  if (digitalRead(bombacuatropin) == 1) {
      ptr += "<h3>Bomba activa </h3>";
    } else {
      ptr += "<h3>Bomba apagada </h3>";;
    }  
  ptr +="<span class='superscript'>%</span></div>";
  ptr +="</div>";
  // Temperatura Interior DHT22
  ptr +="<div class='data temperature'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
  ptr +="C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
  ptr +="c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
  ptr +="c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
  ptr +="s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Temperatura Interior DHT22</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr += "<p><span id='temp_interior_web'>--</span> %</p>";
  ptr +="<span class='superscript'>&deg;C</span></div>";
  ptr +="</div>";
  // Temperatura Interior BMP280
  ptr +="<div class='data temperature'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
  ptr +="C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
  ptr +="c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
  ptr +="c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
  ptr +="s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Temperatura Interior BMP280</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr += "<p><span id='temp_interior_bmp280_web'>--</span> %</p>";
  ptr +="<span class='superscript'>&deg;C</span></div>";
  ptr +="</div>";
  // Temperatura Externa LM35
  ptr +="<div class='data temperature_ex'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
  ptr +="C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
  ptr +="c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
  ptr +="c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
  ptr +="s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Temperatura Externa LM35</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr += "<p><span id='temp_exterior_web'>--</span> %</p>";
  ptr +="<span class='superscript'>&deg;C</span></div>";
  ptr +="</div>";
  // humedad del ambiente interior
  ptr +="<div class='data humidity'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 29.235 40.64'height=40.64px id=Layer_1 version=1.1 viewBox='0 0 29.235 40.64'width=29.235px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><path d='M14.618,0C14.618,0,0,17.95,0,26.022C0,34.096,6.544,40.64,14.618,40.64s14.617-6.544,14.617-14.617";
  ptr +="C29.235,17.95,14.618,0,14.618,0z M13.667,37.135c-5.604,0-10.162-4.56-10.162-10.162c0-0.787,0.638-1.426,1.426-1.426";
  ptr +="c0.787,0,1.425,0.639,1.425,1.426c0,4.031,3.28,7.312,7.311,7.312c0.787,0,1.425,0.638,1.425,1.425";
  ptr +="C15.093,36.497,14.455,37.135,13.667,37.135z'fill=#3C97D3 /></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Humedad del ambiente interior</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr += "<p><span id='humedad_ambiente_web'>--</span> %</p>";
  ptr +="<span class='superscript'>%</span></div>";
  ptr +="</div>";
  // Presión atmosférica
  ptr +="<div class='data pressure'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 40.542 40.541'height=40.541px id=Layer_1 version=1.1 viewBox='0 0 40.542 40.541'width=40.542px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M34.313,20.271c0-0.552,0.447-1,1-1h5.178c-0.236-4.841-2.163-9.228-5.214-12.593l-3.425,3.424";
  ptr +="c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293c-0.391-0.391-0.391-1.023,0-1.414l3.425-3.424";
  ptr +="c-3.375-3.059-7.776-4.987-12.634-5.215c0.015,0.067,0.041,0.13,0.041,0.202v4.687c0,0.552-0.447,1-1,1s-1-0.448-1-1V0.25";
  ptr +="c0-0.071,0.026-0.134,0.041-0.202C14.39,0.279,9.936,2.256,6.544,5.385l3.576,3.577c0.391,0.391,0.391,1.024,0,1.414";
  ptr +="c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293L5.142,6.812c-2.98,3.348-4.858,7.682-5.092,12.459h4.804";
  ptr +="c0.552,0,1,0.448,1,1s-0.448,1-1,1H0.05c0.525,10.728,9.362,19.271,20.22,19.271c10.857,0,19.696-8.543,20.22-19.271h-5.178";
  ptr +="C34.76,21.271,34.313,20.823,34.313,20.271z M23.084,22.037c-0.559,1.561-2.274,2.372-3.833,1.814";
  ptr +="c-1.561-0.557-2.373-2.272-1.815-3.833c0.372-1.041,1.263-1.737,2.277-1.928L25.2,7.202L22.497,19.05";
  ptr +="C23.196,19.843,23.464,20.973,23.084,22.037z'fill=#26B999 /></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Presión atmosférica</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr += "<p><span id='pres_interior_bmp280_web'>--</span> %</p>";
  ptr +="<span class='superscript'>hPa</span></div>";
  ptr +="</div>";
  // Altitud
  ptr +="<div class='data altitude'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 58.422 40.639'height=40.639px id=Layer_1 version=1.1 viewBox='0 0 58.422 40.639'width=58.422px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M58.203,37.754l0.007-0.004L42.09,9.935l-0.001,0.001c-0.356-0.543-0.969-0.902-1.667-0.902";
  ptr +="c-0.655,0-1.231,0.32-1.595,0.808l-0.011-0.007l-0.039,0.067c-0.021,0.03-0.035,0.063-0.054,0.094L22.78,37.692l0.008,0.004";
  ptr +="c-0.149,0.28-0.242,0.594-0.242,0.934c0,1.102,0.894,1.995,1.994,1.995v0.015h31.888c1.101,0,1.994-0.893,1.994-1.994";
  ptr +="C58.422,38.323,58.339,38.024,58.203,37.754z'fill=#955BA5 /><path d='M19.704,38.674l-0.013-0.004l13.544-23.522L25.13,1.156l-0.002,0.001C24.671,0.459,23.885,0,22.985,0";
  ptr +="c-0.84,0-1.582,0.41-2.051,1.038l-0.016-0.01L20.87,1.114c-0.025,0.039-0.046,0.082-0.068,0.124L0.299,36.851l0.013,0.004";
  ptr +="C0.117,37.215,0,37.62,0,38.059c0,1.412,1.147,2.565,2.565,2.565v0.015h16.989c-0.091-0.256-0.149-0.526-0.149-0.813";
  ptr +="C19.405,39.407,19.518,39.019,19.704,38.674z'fill=#955BA5 /></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Altitud</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr += "<p><span id='alti_interior_bmp280_web'>--</span> %</p>";
  ptr +="<span class='superscript'>m</span></div>";
  ptr +="</div>";
  // CO2
  ptr +="<div class='data co2_color'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 40.542 40.541'height=40.541px id=Layer_1 version=1.1 viewBox='0 0 40.542 40.541'width=40.542px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M34.313,20.271c0-0.552,0.447-1,1-1h5.178c-0.236-4.841-2.163-9.228-5.214-12.593l-3.425,3.424";
  ptr +="c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293c-0.391-0.391-0.391-1.023,0-1.414l3.425-3.424";
  ptr +="c-3.375-3.059-7.776-4.987-12.634-5.215c0.015,0.067,0.041,0.13,0.041,0.202v4.687c0,0.552-0.447,1-1,1s-1-0.448-1-1V0.25";
  ptr +="c0-0.071,0.026-0.134,0.041-0.202C14.39,0.279,9.936,2.256,6.544,5.385l3.576,3.577c0.391,0.391,0.391,1.024,0,1.414";
  ptr +="c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293L5.142,6.812c-2.98,3.348-4.858,7.682-5.092,12.459h4.804";
  ptr +="c0.552,0,1,0.448,1,1s-0.448,1-1,1H0.05c0.525,10.728,9.362,19.271,20.22,19.271c10.857,0,19.696-8.543,20.22-19.271h-5.178";
  ptr +="C34.76,21.271,34.313,20.823,34.313,20.271z M23.084,22.037c-0.559,1.561-2.274,2.372-3.833,1.814";
  ptr +="c-1.561-0.557-2.373-2.272-1.815-3.833c0.372-1.041,1.263-1.737,2.277-1.928L25.2,7.202L22.497,19.05";
  ptr +="C23.196,19.843,23.464,20.973,23.084,22.037z'fill=#26B999 /></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Dióxido de carbono (CO2)</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr += "<p><span id='ppm_web'>--</span> %</p>";
  ptr +="<span class='superscript'>hPa</span></div>";
  ptr +="</div>";
  // Final
  ptr +="</div>";
  ptr +="</body>";
  ptr +="</html>";
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
  // Crecimiento
  float humedadSuelo_ = analogRead(humedad_pin);
  humedad_plantin = humedadSuelo_;
  // Vegetacion
  float humedadSuelo_dos = analogRead(humedad_vege_pin);
  humedad_vege = humedadSuelo_dos;
  // Flora
  float humedadSuelo_tres = analogRead(humedad_prin_flora_pin);
  humedad_prin_flora_ = humedadSuelo_tres;
  float humedadSuelo_cuatro = analogRead(humedad_flora_pin);
  humedad_flora = humedadSuelo_cuatro;

  Serial.print("H. Inicio: ");
  Serial.println(humedad_plantin);
  Serial.print("H. Vegetacion: ");
  Serial.println(humedad_vege);
  Serial.print("H. Flora: ");
  Serial.println(humedad_prin_flora_);
  Serial.print("H. Flora: ");
  Serial.println(humedad_flora);
}
void temperatura_lm35() {
  // Leer la temperatura del sensor LM35DZ
  float temperatura_ = (analogRead(Lm35pin) * 3.3) / 4095.0;
  temperatura_ = temperatura_ * 100;
  temp_exterior = temperatura_;

  Serial.print("Temperatura LM35DZ: ");
  Serial.println(temperatura_);
}
void temperatura_dht22() {
  // Leer la temperatura y humedad del sensor DHT22
  float temperaturaD_ = dht.readTemperature();
  temp_interior = temperaturaD_;
  float humedadD_ = dht.readHumidity();
  humedad_ambiente = humedadD_;

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
  if (humedad_plantin < menor_porcentaje) {
    digitalWrite(bombapin, LOW);
  } else if (humedad_plantin > mayor_porcentaje) {
    digitalWrite(bombapin, HIGH);
  }
  if (humedad_vege < menor_porcentaje_vege) {
    digitalWrite(bombadospin, LOW);
  } else if (humedad_vege > mayor_porcentaje_vege) {
    digitalWrite(bombadospin, HIGH);
  }
  if (humedad_prin_flora_ < menor_porcentaje_Prin_flora) {
    digitalWrite(bombatrespin, LOW);
  } else if (humedad_prin_flora_ > mayor_porcentaje_prin_flora) {
    digitalWrite(bombatrespin, HIGH);
  }
  if (humedad_flora < menor_porcentaje_flora) {
    digitalWrite(bombacuatropin, LOW);
  } else if (humedad_flora > mayor_porcentaje_flora) {
    digitalWrite(bombacuatropin, HIGH);
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
  if (temp_interior > temperatura_mayor) {
    digitalWrite(ventilacionpin, HIGH);
  } else if (temp_interior < temperatura_menor) {
    digitalWrite(ventilacionpin, LOW);
  }
}
//___________________________________________________________________________________________
// Menu de Encendido o apagado de la luces y bomba
void lcd_menu1() {
  //Menu 1
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("B Cre.: ");
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
  lcd.print(humedad_plantin);
  lcd.print("%");

  lcd.setCursor(0, 0);
  lcd.print("Temp.: ");
  lcd.print(temp_exterior);
  lcd.print(" C");
}
// Menu de DHT22 De temperatura y humedad
void lcd_menu3() {
  //Menu 3
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Temp.: ");
  lcd.print(temp_interior);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Hum Anviente: ");
  lcd.print(humedad_ambiente);
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

  lcd.setCursor(0,1);
  lcd.print("Ip:");
  lcd.print(WiFi.localIP());;
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
  ThingSpeak.setField(1, humedad_plantin);
  ThingSpeak.setField(2, humedad_vege);
  ThingSpeak.setField(3, humedad_prin_flora_);
  ThingSpeak.setField(4, humedad_flora);
  ThingSpeak.setField(5, temp_exterior);   //LM35
  ThingSpeak.setField(6, temp_interior);  //DHT22
  ThingSpeak.setField(7, humedad_ambiente);      //DHT22
  ThingSpeak.setField(8, ppm);           //CO2
  ThingSpeak.writeFields(channelID, writeAPIKey);

  Serial.println("Datos enviados a ThingSpeak.");
}
//___________________________________________________________________________________________
//################################# Actualizacion WebSocket #####################################
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    // Cuando un cliente se conecta al servidor de WebSockets
    case WStype_CONNECTED:
      Serial.printf("[%u] Conectado al servidor\n", num);
      clientID = num;
      break;
    // Cuando un cliente envía un mensaje al servidor de WebSockets
    case WStype_TEXT:
      Serial.printf("[%u] Recibido mensaje: %s\n", num, payload);
      break;
    // Cuando un cliente se desconecta del servidor de WebSockets
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Desconectado del servidor\n", num);
      break;
  }
}
// Función para enviar datos a través del WebSocket
void sendSensorData() {
  // Crear un objeto JSON con los datos del sensor
  String json = "{\"humedad_plantin_web\":";
  json += humedad_plantin;
  json += ",\"humedad_vege_web\":";
  json += humedad_vege;
  json += ",\"humedad_prin_flora_web\":";
  json += humedad_prin_flora_;
  json += ",\"humedad_flora_web\":";
  json += humedad_flora;
  json += ",\"temp_interior_web\":";
  json += temp_interior;
  json += ",\"humedad_ambiente_web\":";
  json += humedad_ambiente;
  /*json += ",\"temp_interior_bmp280_web\":";
  json += bmp.readTemperature();
  json += ",\"pres_interior_bmp280_web\":";
  json += bmp.readPressure() / 100.0F;
  json += ",\"alti_interior_bmp280_web\":";
  json += bmp.readAltitude(1013.25);*/
  json += ",\"temp_exterior_web\":";
  json += temp_exterior;
  json += ",\"ppm_web\":";
  json += ppm;
  json += "}";

  // Enviar el objeto JSON a todos los clientes conectados al servidor de WebSockets
  webSocket.broadcastTXT(json);
}
//___________________________________________________________________________________________
void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Configuración WiFi</h1>";
  html += "<form method='post' action='/save'>";
  html += "<label for='ssid'>SSID:</label>";
  html += "<input type='text' name='ssid'><br>";
  html += "<label for='password'>Contraseña:</label>";
  html += "<input type='password' name='password'><br>";
  html += "<h1>Configuración de mediciones principlaes</h1>";
  html += "<h2>Etapa plantin</h2>";
  html += "<label for='mayor_porcentaje'>Porcentaje de humedad del suelo mayor:</label>";
  html += "<input type='number' name='mayor_porcentaje' min='0' max='100'><br>";
  html += "<label for='menor_porcentaje'>Porcentaje de humedad del suelo menor:</label>";
  html += "<input type='number' name='menor_porcentaje' min='0' max='100'><br>";
  html += "<h2>Etapa vegetacion</h2>";
  html += "<label for='mayor_porcentaje_vege'>Porcentaje de humedad del suelo mayor:</label>";
  html += "<input type='number' name='mayor_porcentaje_vege' min='0' max='100'><br>";
  html += "<label for='menor_porcentaje_vege'>Porcentaje de humedad del suelo menor:</label>";
  html += "<input type='number' name='menor_porcentaje_vege' min='0' max='100'><br>";  
  html += "<h2>Etapa principio de flora</h2>";
  html += "<label for='mayor_porcentaje_prin_flora'>Porcentaje de humedad del suelo mayor:</label>";
  html += "<input type='number' name='mayor_porcentaje_prin_flora' min='0' max='100'><br>";
  html += "<label for='menor_porcentaje_Prin_flora'>Porcentaje de humedad del suelo menor:</label>";
  html += "<input type='number' name='menor_porcentaje_Prin_flora' min='0' max='100'><br>";  
  html += "<h2>Etapa plantin</h2>";
  html += "<label for='mayor_porcentaje_flora'>Porcentaje de humedad del suelo mayor:</label>";
  html += "<input type='number' name='mayor_porcentaje_flora' min='0' max='100'><br>";
  html += "<label for='menor_porcentaje_flora'>Porcentaje de humedad del suelo menor:</label>";
  html += "<input type='number' name='menor_porcentaje_flora' min='0' max='100'><br>";  
  html += "<h2>Temperatura del invernadero</h2>";
  html += "<label for='temperatura_mayor'>Mayor temperatura:</label>";
  html += "<input type='number' name='temperatura_mayor' min='0' max='100'><br>";
  html += "<label for='temperatura_menor'>Menor temperatura:</label>";
  html += "<input type='number' name='temperatura_menor' min='0' max='100'><br>";  
  html += "<h2>Horarios de iluminacion</h2>";
  html += "<h3>(Solo hora no minutos Ej: 18)</h3>";
  html += "<label for='horaanochecer'>Hora de finalizacion:</label>";
  html += "<input type='number' name='horaanochecer' min='0' max='23'><br>";
  html += "<label for='horaamanecer'>Hora de inicio:</label>";
  html += "<input type='number' name='horaamanecer' min='0' max='23'><br>";  
  html += "<input type='submit' value='Guardar'>";
  html += "</form>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleSave() {
  ssid = server.arg("ssid");
  password = server.arg("password");
  mayor_porcentaje = server.arg("mayor_porcentaje").toInt();
  menor_porcentaje = server.arg("menor_porcentaje").toInt();
  mayor_porcentaje_vege = server.arg("mayor_porcentaje_vege").toInt();
  menor_porcentaje_vege = server.arg("menor_porcentaje_vege").toInt();
  mayor_porcentaje_prin_flora = server.arg("mayor_porcentaje_prin_flora").toInt();
  menor_porcentaje_Prin_flora = server.arg("menor_porcentaje_Prin_flora").toInt();
  mayor_porcentaje_flora = server.arg("mayor_porcentaje_flora").toInt();
  menor_porcentaje_flora = server.arg("menor_porcentaje_flora").toInt();
  temperatura_mayor = server.arg("temperatura_mayor").toInt();
  temperatura_menor = server.arg("temperatura_menor").toInt();
  horaanochecer = server.arg("horaanochecer").toInt();
  horaamanecer = server.arg("horaamanecer").toInt();
  shouldSaveConfig = true;
  
  String html = "<html><body>";
  html += "<h1>Configuración guardada</h1>";
  html += "<p>Los datos de la configuración han sido guardados.</p>";
  html += "<p>La red WiFi generada automáticamente se apagará en unos segundos.</p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
  
  // Apagar la red WiFi generada automáticamente después de 10 segundos
  delay(10000);
  WiFi.softAPdisconnect(true);
  Serial.println("Red WiFi generada automáticamente apagada");
}
