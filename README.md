# PROYECTO INTEGRAL DE AUTOMATIZACIÓN MODULAR DE INVERNADERO BASADO EN ESP32

## 1. Descripción general

El presente proyecto propone el desarrollo de un sistema electrónico modular para la supervisión, automatización y control de invernaderos mediante microcontroladores ESP32.

El sistema estará diseñado para funcionar tanto en:

* invernaderos exteriores;
* invernaderos interiores;
* invernaderos con ventilación natural;
* invernaderos con ventilación artificial;
* invernaderos con apertura automática de techo o ventanas;
* invernaderos con iluminación natural;
* invernaderos con iluminación artificial;
* instalaciones con uno o varios sectores de riego;
* instalaciones con sensores ambientales básicos;
* instalaciones con sensores agrícolas avanzados.

La característica fundamental del sistema será su **configurabilidad mediante interfaz web**.

No será necesario modificar el firmware cada vez que una instalación tenga una cantidad diferente de sensores o actuadores.

Por ejemplo, el mismo firmware podrá utilizarse en una instalación con:

* un sensor de temperatura;
* dos ventiladores;
* una bomba;
* cuatro válvulas;

o en otra instalación con:

* dos sensores de temperatura;
* sensores de humedad del suelo;
* CO₂;
* iluminación;
* calefacción;
* humidificador;
* ventanas automáticas;
* techo automático;
* ocho zonas de riego.

Los elementos disponibles estarán definidos en el firmware, mientras que la instalación concreta determinará mediante la interfaz web cuáles están habilitados.

---

# 2. Objetivos

El sistema tendrá como objetivos principales:

1. Medir las condiciones ambientales del invernadero.
2. Medir las condiciones del suelo o sustrato.
3. Controlar automáticamente los actuadores.
4. Permitir funcionamiento autónomo sin conexión a Internet.
5. Permitir funcionamiento mediante red local.
6. Disponer de una página web local alojada en el ESP32.
7. Disponer de API REST.
8. Disponer de comunicación en tiempo real mediante WebSocket.
9. Permitir integración con un servidor central.
10. Permitir integración mediante MQTT.
11. Permitir administrar múltiples invernaderos desde una misma plataforma.
12. Registrar históricos y eventos.
13. Detectar fallos de sensores.
14. Detectar fallos de actuadores cuando sea posible.
15. Incorporar modos de funcionamiento seguro.
16. Permitir actualización OTA.
17. Permitir habilitar y deshabilitar sensores y actuadores desde la interfaz web.
18. Evitar que un usuario sin conocimientos de programación tenga que modificar el firmware.

---

# 3. Principio fundamental de diseño

El sistema deberá separar tres conceptos:

## Hardware disponible

Son los sensores y actuadores que físicamente pueden conectarse.

## Hardware habilitado

Son los elementos que realmente posee una determinada instalación.

## Funciones automáticas

Son las reglas que utilizan esos elementos.

Por ejemplo:

```text
Firmware
│
├── Sensor SHT31
├── Sensor DS18B20
├── Sensor humedad suelo
├── Sensor CO₂
├── Sensor luz
├── Sensor nivel
├── Sensor caudal
│
├── Bomba
├── Válvula 1
├── Válvula 2
├── Ventilador
├── Extractor
├── Calefacción
├── Iluminación
└── Ventana
```

Pero desde la página:

```text
SENSORES

☑ Temperatura/Humedad
☑ Temperatura DS18B20
☑ Humedad de suelo
☐ CO₂
☑ Nivel del tanque
☐ Caudal
☐ Luz PAR
```

Y:

```text
ACTUADORES

☑ Bomba
☑ Válvula 1
☑ Válvula 2
☑ Ventilador
☐ Extractor
☐ Calefacción
☐ Humidificador
☑ Iluminación
☐ Ventana automática
```

El firmware deberá adaptar automáticamente las funciones disponibles.

---

# 4. Arquitectura general

La arquitectura estará dividida en tres niveles.

```text
                 SERVIDOR CENTRAL
                       │
              HTTPS / MQTT / API
                       │
             ┌─────────┴─────────┐
             │                   │
          ESP32 #1             ESP32 #2
        Invernadero A         Invernadero B
             │                   │
       ┌─────┴─────┐       ┌─────┴─────┐
       │           │       │           │
    Sensores   Actuadores Sensores   Actuadores
```

Cada ESP32 podrá funcionar de manera autónoma.

Si el servidor central deja de funcionar:

```text
Servidor ❌
    │
    X
    │
ESP32
 │
 ├── sensores
 ├── reglas
 ├── riego
 ├── ventilación
 └── seguridad
```

El invernadero deberá continuar funcionando.

---

# 5. Modos de funcionamiento

Se establecerán cuatro modos principales.

## AUTO

El sistema controla automáticamente los actuadores.

## MANUAL

El usuario puede controlar determinados actuadores desde la web.

## PROGRAMADO

El sistema ejecuta horarios y programas definidos por el usuario.

## SEGURIDAD

El sistema desactiva determinadas funciones cuando existe una condición peligrosa.

Por ejemplo:

```text
TEMP > límite de emergencia
       ↓
calefacción OFF
       ↓
ventilación ON
```

---

# 6. Microcontrolador

El controlador principal será un ESP32.

Se deberá mantener compatibilidad conceptual con:

* ESP32-WROOM-32;
* ESP32 DevKitC;
* ESP32 DOIT DevKit V1;
* ESP32-S3.

La documentación oficial del ESP32 DevKitC confirma la disponibilidad de I²C, SPI, PWM, ADC, DAC, GPIO, contador de pulsos y TWAI/CAN, entre otros periféricos.

Para una primera implementación se podrá utilizar el ESP32 DevKit que ya se posee.

Para una PCB definitiva se recomienda evaluar ESP32-S3.

Todos ellos compartibles con N8 o N16

---

# 7. Sensores ambientales

## 7.1 Temperatura y humedad

El sensor principal recomendado será:

**SHT31**

Es una solución con buena relación entre prestaciones, disponibilidad y precio.

El SHT31 ofrece aproximadamente:

* ±2 %RH;
* ±0,2 °C;
* comunicación I²C;
* alimentación de 2,4 a 5,5 V;
* rango de humedad 0–100 %RH.

También existe la variante con cubierta protectora SHT31-DIS-P, especialmente interesante para proteger el sensor frente al entorno.

### Alternativa económica

Podrá admitirse:

**AHT20**

si el precio del SHT31 resulta elevado.

El software deberá abstraer el sensor:

```text
TemperatureHumiditySensor
       │
       ├── SHT31
       ├── AHT10
       └── AHT20
```

Por lo tanto, las reglas del sistema no deberán depender directamente del modelo físico.

---

# 8. Sensores DS18B20

Para mediciones distribuidas se utilizará DS18B20.

Es especialmente adecuado para:

* temperatura del agua;
* temperatura del tanque;
* temperatura del sustrato;
* temperatura de tuberías;
* temperatura exterior.

El DS18B20 utiliza un bus 1-Wire, permite múltiples dispositivos en el mismo bus y cada sensor dispone de un identificador único de 64 bits. Su precisión especificada es ±0,5 °C entre -10 y 85 °C.

La conexión será:

```text
ESP32 GPIO
     │
     ├──────── DS18B20 #1
     ├──────── DS18B20 #2
     ├──────── DS18B20 #3
     ├──────── DS18B20 #4
     └──────── DS18B20 #N
```

con:

```text
DATA ─── 4,7 kΩ ─── 3,3 V
```
La resistencia es una en comun entre todos los sensores DS18B20

Se recomienda utilizar sensores encapsulados en acero inoxidable para ambientes húmedos.

---

# 9. Humedad del suelo

Se utilizarán sensores capacitivos.

Como referencia puede utilizarse el DFRobot SEN0193 o sensores equivalentes.

El sensor SEN0193 utiliza medición capacitiva en lugar de resistiva y está diseñado para reducir problemas de corrosión. También incorpora regulación para trabajar aproximadamente entre 3,3 y 5,5 V.

No se recomienda utilizar sensores resistivos económicos como solución definitiva.

## Importante

Los valores de humedad del suelo no deben interpretarse directamente como porcentaje universal.

Cada tipo de suelo deberá calibrarse.

Por ejemplo:

```text
Lectura seca = 2850
Lectura húmeda = 1450
```

El firmware convertirá esos valores a:

```text
0 % = completamente seco
100 % = condición de calibración húmeda
```

---

# 10. ADC externo

Para los sensores analógicos se utilizará preferentemente:

**ADS1115**

El ADS1115 dispone de:

* 16 bits;
* 4 entradas;
* I²C;
* PGA;
* hasta 860 muestras por segundo;
* alimentación de 2 a 5,5 V.

Esto permitirá:

```text
ADS1115
│
├── A0 → humedad suelo zona 1
├── A1 → humedad suelo zona 2
├── A2 → humedad suelo zona 3
└── A3 → humedad suelo zona 4
```

Si se necesitan más entradas se podrá agregar un segundo ADS1115 utilizando otra dirección I²C.

---

# 11. Sensor de iluminación

Para una solución económica se utilizará:

**BH1750**

Permite medir iluminación mediante I²C y es sencillo de integrar.

El software deberá denominar genéricamente esta entrada:

```text
LightSensor
```

de modo que posteriormente pueda sustituirse por:

* BH1750;
* sensor de radiación;
* sensor PAR/PPFD.

---

# 12. Luz y cultivo

El sistema diferenciará:

```text
LUX
```

de:

```text
PPFD
```

Para una instalación doméstica se podrá utilizar lux como referencia.

Para instalaciones agrícolas más avanzadas se podrá incorporar un sensor PAR.

El sistema no deberá asumir que lux = PPFD.

---

# 13. Sensor de CO₂

El sensor de CO₂ será opcional.

Se recomienda:

**SCD40/SCD41**

pero se considerará un módulo avanzado debido a su mayor costo.

La instalación económica podrá funcionar perfectamente sin CO₂.

La configuración:

```text
CO₂
[ ] Habilitado
```

deberá activar/desactivar toda la lógica relacionada.

---

# 14. Nivel del depósito

Se recomienda combinar:

### Medición continua

Sensor ultrasónico impermeable.

### Seguridad

Dos sensores de flotador:

```text
FLOAT_LOW
FLOAT_HIGH
```

Esto permite:

```text
Nivel analógico
+
protección física
```

El sistema podrá mostrar:

```text
Tanque: 72 %
```

pero además:

```text
FLOAT_LOW = OK
FLOAT_HIGH = OK
```

---

# 15. Sensor de caudal

Se utilizará un caudalímetro con salida por pulsos.

El ESP32 calculará:

```text
litros/minuto
litros/hora
litros acumulados
```

Esto será especialmente importante para proteger las bombas.

Ejemplo:

```text
Bomba ON
   ↓
esperar 5 segundos
   ↓
¿hay caudal?
 ├── Sí → continuar
 └── No → detener bomba
```

Se utilizara un Optoacoplador para vajar de los voltajes nominales (5V, 12V o 24V) a 3.3V del ESP32

---

# 16. Sensor de lluvia

En instalaciones exteriores se podrá utilizar un pluviómetro de pulsos.

No se recomienda utilizar como sensor principal de lluvia las placas resistivas económicas destinadas a proyectos educativos.

El módulo deberá aparecer en la configuración:

```text
Sensor de lluvia
[✓] Habilitado
```

En un invernadero completamente interior:

```text
[ ] Deshabilitado
```

y ninguna función deberá depender de él.

Se pensara una comunicacion con estaciones meteorologias para obtener datos mas precisos ya que en muchso invernaderos esteriores poseen una.

---

# 17. Sensor exterior

En un invernadero exterior se recomienda un segundo sensor ambiental.

Se pensara una comunicacion con estaciones meteorologias para obtener datos mas precisos ya que en muchso invernaderos esteriores poseen una.

```text
EXTERIOR

Temperatura
Humedad
Luz
Lluvia
```

El sistema podrá entonces comparar:

```text
Interior:
26,2 °C

Exterior:
21,5 °C
```

y utilizar esa información para controlar ventilación y apertura.

---

### 18. Medición de pH

Para que el sistema sea modular y pueda utilizarse tanto en un invernadero convencional como en sistemas hidropónicos, fertirriego o instalaciones de mayor escala, la medición de pH se plantea mediante diferentes interfaces.

**18.1. Sensores analógicos económicos**

Se podrá utilizar el **DFRobot Gravity Analog pH Sensor/Meter Kit SEN0161** y variantes compatibles de la familia Gravity, incluyendo el **SEN024** cuando corresponda a la aplicación.

Estos módulos proporcionan una salida analógica proporcional a la medición realizada por el electrodo de pH y son una alternativa adecuada para instalaciones de bajo y medio costo.

Para obtener una medición estable se recomienda:

* Utilizar una entrada ADC externa de mayor resolución, preferentemente **ADS1115**, en lugar de depender exclusivamente del ADC interno del ESP32.
* Realizar calibración mediante soluciones patrón, normalmente pH 4,00, pH 7,00 y, cuando sea necesario, pH 10,00.
* Guardar en la configuración del ESP32 los parámetros de calibración.
* Implementar compensación de temperatura cuando la precisión requerida lo justifique.
* Separar eléctricamente y físicamente el circuito de pH de cargas de potencia, motores, bombas y relés.
* Implementar filtrado digital para reducir ruido.
* Detectar condiciones anormales como electrodo desconectado, señal fuera de rango o valores imposibles.

El sistema no debería considerar que el valor de pH es válido simplemente porque existe una tensión en la entrada analógica. Debe existir un estado de **sensor válido/no válido**.

---

**18.2. Medición de pH mediante RS485 / Modbus RTU**

Para instalaciones profesionales, distancias grandes entre sensores y controlador o cuando se requiere mayor robustez frente al ruido eléctrico, se incorporará como opción la utilización de **sensores/transmisores de pH con interfaz RS485 y protocolo Modbus RTU**.

La arquitectura será:

**Electrodo de pH → transmisor → RS485/Modbus RTU → ESP32**

El transmisor se encarga de acondicionar la señal extremadamente pequeña del electrodo y entregar al sistema una comunicación digital mucho más apropiada para instalaciones industriales.

Para implementar RS485 se podrán utilizar diferentes alternativas de interfaz, por ejemplo:

* **ADM2483** u otros transceptores/interfases RS485 aisladas.
* **SP3485** u otros transceptores RS485 de 3,3 V.
* Módulos RS485 aislados mediante **optoacopladores**, cuando la instalación requiera mayor aislamiento eléctrico.
* Transmisores de pH comerciales que ya incorporen directamente RS485 + Modbus RTU.

En una instalación con bombas, electroválvulas, motores, contactores y variadores de frecuencia, resulta especialmente interesante la utilización de **RS485 aislado**, ya que permite separar eléctricamente el bus de comunicaciones de la electrónica principal.

### 18.3. Ventajas de incorporar RS485

La incorporación de RS485 no debe limitarse únicamente al sensor de pH. Se puede definir un **bus de sensores industriales del invernadero**.

Por ejemplo:

```text
                         ┌── pH Modbus
                         ├── EC Modbus
                         ├── Temperatura Modbus
                         ├── Humedad Modbus
ESP32 ── RS485 ──────────┼── CO₂ Modbus
                         ├── Oxígeno disuelto
                         ├── ORP
                         ├── Caudal
                         ├── Nivel
                         └── Otros sensores
```

Cada dispositivo tendría una dirección Modbus diferente:

```text
ID 1 → pH
ID 2 → EC
ID 3 → temperatura
ID 4 → nivel
ID 5 → caudal
ID 6 → ORP
...
```

Esto permite que un único bus pueda incorporar numerosos dispositivos sin tener que reservar un GPIO o una entrada ADC independiente para cada sensor.

Además, RS485 permite utilizar **cables largos**, algo especialmente importante cuando el sensor está físicamente alejado del ESP32 o instalado en otro sector del invernadero.

---

### 18.4. Arquitectura recomendada para el sistema

El firmware deberá abstraer el origen de la medición.

Por ejemplo:

```text
pH_01
 ├── tipo: ANALOG
 ├── interfaz: ADS1115
 ├── dirección: A0
 ├── calibración: almacenada en NVS
 └── estado: habilitado

pH_02
 ├── tipo: MODBUS
 ├── interfaz: RS485
 ├── slave_id: 1
 ├── registro: configurable
 └── estado: habilitado
```

Desde la página web se podrá seleccionar qué sensores están instalados, sin necesidad de modificar el firmware.

La configuración podría incluir:

```json
{
  "ph": {
    "enabled": true,
    "interface": "MODBUS",
    "slave_id": 1,
    "poll_interval": 5000,
    "temperature_compensation": true
  }
}
```

Para un sensor analógico:

```json
{
  "ph": {
    "enabled": true,
    "interface": "ANALOG",
    "adc": "ADS1115",
    "channel": 0,
    "calibration": {
      "ph4": 3.02,
      "ph7": 2.51,
      "ph10": 2.01
    }
  }
}
```

Los valores concretos de calibración son solamente ilustrativos y deberán obtenerse durante la calibración real del sensor.

---

### 18.5. Ampliación del bus RS485

La incorporación de RS485 modifica además la filosofía general del proyecto.

El ESP32 no tendrá que estar limitado a sensores conectados directamente a sus GPIO, I²C, 1-Wire o ADC.

Se podrá disponer de:

```text
                  ┌─ I²C
                  │
ESP32 ────────────┼─ SPI
                  │
                  ├─ 1-Wire
                  │
                  ├─ ADC
                  │
                  └─ RS485 / Modbus
                           │
             ┌─────────────┼─────────────┐
             │             │             │
           pH            EC           Temp.
         Modbus         Modbus        Modbus
```

Esto permite diseñar el controlador como una **plataforma multiprotocolo**, en lugar de como una placa específica para un determinado conjunto de sensores.

Además, en futuras versiones se podrán incorporar sensores industriales de:

* pH
* EC/conductividad
* ORP
* temperatura
* humedad
* CO₂
* oxígeno disuelto
* caudal
* presión
* nivel
* radiación
* nutrientes
* sensores meteorológicos
* estaciones climáticas completas

siempre que el dispositivo utilice un protocolo compatible, especialmente **Modbus RTU sobre RS485**.

---

# 19. Conductividad eléctrica

La EC también será opcional.

Se utilizará para:

* hidroponía;
* fertirriego;
* control de concentración de nutrientes.

El sistema deberá poder trabajar perfectamente sin pH ni EC.

---

# 20. Actuadores

El sistema soportará los siguientes tipos.

Se pensara expansores como el 74HC595 / 74HCT595 para amplizar la cantidad de salidas. Las cuales pueden controlar salidas estáticas On/Off o PWM independiente, usas una técnica de Soft-PWM por hardware (enviando buffers mediante DMA o temporizadores del ESP32). A frecuencias estándar de conmutación de potencia (ej. 100Hz - 1kHz), la carga del procesador es nula. 

### Ventiladores

```text
FAN_1
FAN_2
FAN_3
```

### Extractores

```text
EXTRACTOR_1
EXTRACTOR_2
```

### Bombas

```text
PUMP_1
PUMP_2
```

### Electroválvulas

```text
VALVE_1
VALVE_2
...
VALVE_8
```

### Iluminación

```text
LIGHT_1
LIGHT_2
```

### Calefacción

```text
HEATER_1
```

### Humidificación

```text
HUMIDIFIER_1
```

### Ventanas

```text
WINDOW_OPEN
WINDOW_CLOSE
```

### Techo

```text
ROOF_OPEN
ROOF_CLOSE
```

### Sombreado

```text
SHADE_OPEN
SHADE_CLOSE
```

---

# 21. Expansión y control de salidas de potencia

La arquitectura de salidas del sistema se diseñará de forma modular, utilizando registros de desplazamiento **74HC595 o 74HCT595** para ampliar considerablemente la cantidad de salidas disponibles sin consumir un GPIO del ESP32 por cada actuador.

Los registros podrán conectarse en cascada:

```text
ESP32
 │
 ├── DATA
 ├── CLOCK
 └── LATCH
       │
       ▼
   74HC595 #1
       │
       ▼
   74HC595 #2
       │
       ▼
   74HC595 #3
       │
       ▼
   74HC595 #N
```

Cada 74HC595 proporciona 8 salidas digitales, por lo que varios dispositivos conectados en cascada permiten disponer de una gran cantidad de canales utilizando únicamente unas pocas señales del ESP32.

Por ejemplo:

```text
1 × 74HC595  →  8 salidas
2 × 74HC595  → 16 salidas
4 × 74HC595  → 32 salidas
8 × 74HC595  → 64 salidas
```

La cantidad final dependerá de las necesidades de cada instalación.

---

## 21.1. Tipos de salida

El sistema deberá soportar diferentes tipos de salida utilizando la misma arquitectura lógica.

### Salidas digitales ON/OFF

Destinadas a:

* bombas;
* electroválvulas;
* ventiladores;
* extractores;
* iluminación;
* calefacción;
* humidificadores;
* contactores;
* relés;
* alarmas;
* motores mediante sus correspondientes drivers.

Ejemplo:

```text
OUT_01 = ON
OUT_02 = OFF
OUT_03 = ON
OUT_04 = OFF
```

---

## 21.2. Salidas PWM

Las salidas también podrán utilizarse para controlar actuadores que admitan modulación por ancho de pulso.

Ejemplos:

* ventiladores DC;
* bombas DC compatibles;
* iluminación LED;
* válvulas proporcionales;
* drivers de potencia;
* sistemas de sombreado compatibles con PWM.

Ejemplo:

```text
FAN_1 = 30 %
FAN_2 = 65 %
LIGHT_1 = 80 %
```

Sin embargo, el 74HC595 se considerará únicamente como **elemento de expansión de salidas**, no como generador PWM autónomo.

La generación de PWM será responsabilidad del sistema de control del ESP32.

---

## 21.3. Soft-PWM mediante actualización de registros

Para permitir un gran número de canales PWM sin utilizar un periférico PWM independiente para cada salida, el firmware podrá implementar una arquitectura de **Soft-PWM basada en actualización periódica de los registros 74HC595**.

La idea general será:

```text
                ESP32
                  │
          Temporizador / DMA
                  │
             Buffer PWM
                  │
            SPI / Shift
                  │
                  ▼
             74HC595
                  │
        ┌─────────┼─────────┐
        │         │         │
      OUT 1     OUT 2     OUT 3
        │         │         │
       PWM       PWM       PWM
```

El ESP32 mantendrá un buffer con el estado de cada canal.

Por ejemplo:

```text
PWM_BUFFER[]

OUT_01 = 20 %
OUT_02 = 50 %
OUT_03 = 75 %
OUT_04 = 100 %
OUT_05 = 0 %
```

El firmware calculará los estados correspondientes y actualizará periódicamente los registros.

---

## 21.4. Uso de SPI

Aunque el 74HC595 puede controlarse mediante GPIO convencionales, se utilizará preferentemente el periférico **SPI del ESP32** para realizar las transferencias.

Esto permite desplazar grandes cantidades de bits rápidamente:

```text
ESP32 SPI
   │
   ├── DATA
   ├── CLOCK
   └── LATCH
        │
        ▼
   74HC595 #1
        │
        ▼
   74HC595 #2
        │
        ▼
   74HC595 #3
```

Por ejemplo, con 8 registros:

```text
8 × 8 = 64 bits
```

pueden actualizarse como una única trama.

Esto permite mantener un número elevado de salidas sin realizar decenas de operaciones GPIO individuales.

---

## 21.5. DMA y temporización

Cuando se requiera una cantidad elevada de canales PWM, la transmisión de los buffers podrá realizarse utilizando los mecanismos de hardware disponibles en el ESP32, incluyendo **DMA asociado al periférico SPI**, junto con temporizadores para determinar los instantes de actualización.

La arquitectura será conceptualmente:

```text
                ┌──────────────────┐
                │   PWM Scheduler  │
                └────────┬─────────┘
                         │
                         ▼
                  ┌─────────────┐
                  │ PWM Buffer  │
                  └──────┬──────┘
                         │
                    DMA / SPI
                         │
                         ▼
                  ┌─────────────┐
                  │ 74HC595 × N │
                  └──────┬──────┘
                         │
                         ▼
                  Drivers potencia
                         │
                         ▼
                     Actuadores
```

De esta manera, una vez preparada la transferencia, el hardware puede realizar el desplazamiento de los datos sin que la CPU tenga que ejecutar una operación individual por cada salida.

La carga de procesamiento deberá mantenerse muy baja incluso cuando exista una cantidad considerable de salidas.

---

## 21.6. Frecuencia PWM

El sistema deberá permitir configurar diferentes frecuencias de PWM según el tipo de carga.

Como rango inicial de diseño se considerará:

```text
100 Hz – 1 kHz
```

para aplicaciones de control de potencia de baja frecuencia.

La frecuencia final dependerá del actuador y del circuito de potencia utilizado.

No se utilizará necesariamente la misma frecuencia para todos los dispositivos.

Por ejemplo:

```text
Iluminación LED:
frecuencia determinada por el driver

Ventilador DC:
100–1.000 Hz según driver

Válvula proporcional:
según especificaciones del fabricante
```

Para cargas que requieran frecuencias mayores o PWM de alta precisión se podrán utilizar periféricos PWM hardware del ESP32 o controladores externos específicos.

---

## 21.7. Resolución PWM

La arquitectura deberá permitir seleccionar la resolución necesaria para cada aplicación.

Por ejemplo:

```text
8 bits:
0 – 255

10 bits:
0 – 1023

12 bits:
0 – 4095
```

Sin embargo, la resolución efectiva dependerá de la frecuencia PWM seleccionada y de la estrategia de generación utilizada.

Para la mayoría de los actuadores del invernadero será suficiente una resolución moderada.

Por ejemplo:

```text
0 %   → OFF
25 %  → 64
50 %  → 128
75 %  → 192
100 % → 255
```

---

## 21.8. Diferenciación entre salida lógica y salida de potencia

El 74HC595 **no deberá utilizarse para alimentar directamente actuadores**.

Sus salidas solamente proporcionarán las señales de control necesarias para los circuitos posteriores.

La arquitectura será:

```text
ESP32
   │
   ▼
74HC595
   │
   ▼
Driver de salida
   │
   ├── MOSFET
   ├── SSR
   ├── Relay
   └── Driver específico
   │
   ▼
Actuador
```

Por ejemplo, para una válvula de 24 V DC:

```text
ESP32
   │
   ▼
74HC595
   │
   ▼
MOSFET
   │
   ├──── Diodo flyback
   │
   ▼
Válvula 24 V
```

---

## 21.9. MOSFET para cargas DC

Para cargas DC se utilizarán preferentemente MOSFET adecuados a:

* tensión de alimentación;
* corriente;
* temperatura;
* frecuencia de conmutación;
* disipación;
* tensión de puerta.

El diseño deberá contemplar MOSFET de nivel lógico compatibles con la tensión de control utilizada por el circuito.

En cargas inductivas se incorporará la protección correspondiente.

---

## 21.10. Protección de cargas inductivas

Para:

* bombas DC;
* electroválvulas;
* relés;
* motores;
* solenoides;

se incorporará protección contra la tensión inducida durante la desconexión.

Para cargas DC convencionales podrá utilizarse un diodo flyback adecuadamente dimensionado.

```text
        +24 V
          │
       ACTUADOR
          │
          ├─────────|<|────────+
          │          Diodo     │
          │                    │
         MOSFET                │
          │                    │
         GND───────────────────+
```

El componente concreto deberá dimensionarse según la tensión, corriente y naturaleza de la carga.

---

## 21.11. Relés y SSR

Cuando sea necesario controlar cargas que no puedan manejarse directamente mediante MOSFET, el 74HC595 podrá controlar:

```text
74HC595
   │
   ▼
Driver
   │
   ▼
Relé / SSR
   │
   ▼
Carga
```

Esto será especialmente importante para:

* bombas de red eléctrica;
* motores;
* calefactores;
* extractores de gran potencia;
* iluminación de red;
* contactores.

Las cargas de red deberán mantenerse eléctricamente aisladas de la electrónica de baja tensión y deberán utilizar protecciones adecuadas.

---

## 21.12. Salidas PWM independientes

Cada canal lógico deberá poder configurarse individualmente.

Por ejemplo:

```text
OUT_01
Tipo: DIGITAL
Función: Bomba

OUT_02
Tipo: DIGITAL
Función: Válvula 1

OUT_03
Tipo: PWM
Función: Ventilador 1

OUT_04
Tipo: PWM
Función: Ventilador 2

OUT_05
Tipo: PWM
Función: Iluminación
```

De esta forma, la misma cadena de 74HC595 podrá utilizarse para distintos tipos de actuadores.

---

## 21.13. Configuración desde la interfaz web

La configuración de cada salida se realizará desde la aplicación.

Ejemplo:

```text
SALIDA 07

Nombre:
Ventilador principal

Tipo:
PWM

Frecuencia:
500 Hz

Resolución:
8 bits

Modo:
Automático

Mínimo:
20 %

Máximo:
100 %

Estado:
☑ Habilitado
```

Para una bomba:

```text
SALIDA 01

Nombre:
Bomba principal

Tipo:
ON/OFF

Modo:
Automático

Estado:
☑ Habilitado
```

El usuario no tendrá que modificar el código fuente.

---

## 21.14. Estados seguros

Durante el arranque, reinicio o pérdida de comunicación, las salidas deberán asumir un estado seguro definido en la configuración.

Por ejemplo:

```text
Bomba       → OFF
Válvulas    → OFF
Calefacción → OFF
Techo       → según estrategia de seguridad
Ventilación → según estrategia de seguridad
Luz         → configuración definida
```

Esto será especialmente importante porque el 74HC595 puede conservar temporalmente estados mientras se inicializa el ESP32.

El circuito deberá diseñarse para garantizar que las salidas de potencia permanezcan en un estado seguro durante el arranque.

---

## 21.15. Watchdog y recuperación

En caso de:

* bloqueo del firmware;
* reinicio del ESP32;
* pérdida de alimentación;
* error de comunicación;

el sistema deberá reinicializar los registros y los drivers de salida.

La secuencia será:

```text
RESET
  ↓
GPIO en estado seguro
  ↓
Inicialización SPI
  ↓
Inicialización 74HC595
  ↓
Carga del PWM_BUFFER
  ↓
Inicialización de drivers
  ↓
Inicio de automatización
```

---

## 21.16. Ventaja de la arquitectura

La utilización de 74HC595 permite reducir considerablemente la cantidad de GPIO necesarios.

Por ejemplo, para 32 salidas:

```text
Sin expansores:

32 GPIO
```

Con cuatro 74HC595:

```text
ESP32
 │
 ├── DATA
 ├── CLOCK
 └── LATCH
       │
       └── 4 × 74HC595
                │
                └── 32 salidas
```

Por lo tanto, el ESP32 conserva la mayoría de sus GPIO para:

* sensores;
* buses;
* comunicaciones;
* interrupciones;
* entradas de seguridad;
* expansión futura.

---

## 21.17. Arquitectura recomendada

La arquitectura final de salidas será:

```text
                         ESP32
                           │
                    ┌──────┴──────┐
                    │             │
                  SPI          TIMER/DMA
                    │             │
                    └──────┬──────┘
                           │
                      PWM BUFFER
                           │
                           ▼
                     74HC595 × N
                           │
             ┌─────────────┼─────────────┐
             │             │             │
           ON/OFF         PWM          ON/OFF
             │             │             │
             ▼             ▼             ▼
          MOSFET         MOSFET        DRIVER
             │             │             │
             ▼             ▼             ▼
          Válvula       Ventilador     Relé/SSR
             │             │             │
             └─────────────┴─────────────┘
                           │
                        CARGAS
```

Esta arquitectura permitirá construir una plataforma de control con una cantidad muy elevada de salidas utilizando un número reducido de GPIO del ESP32 y manteniendo la posibilidad de controlar tanto cargas digitales como cargas PWM.

La generación de PWM deberá implementarse de manera que la transferencia de los datos hacia los registros se realice mediante los periféricos hardware disponibles, DMA y temporización, cuando corresponda, reduciendo al mínimo la intervención directa de la CPU.

Para las aplicaciones que requieran PWM de alta frecuencia, elevada resolución o sincronización estricta, se utilizarán los periféricos PWM hardware del ESP32 o controladores externos dedicados, en lugar de forzar el uso de Soft-PWM sobre los 74HC595.

---

# 22. Separación eléctrica

El sistema deberá dividirse en:

```text
3,3 V
Lógica ESP32

5 V
Sensores/módulos compatibles

12/24 V
Válvulas y actuadores DC

230 V
Bombas, motores, calefacción, iluminación, etc.
```

La parte de alta tensión deberá permanecer físicamente separada de la electrónica de control.

La instalación deberá incorporar protecciones eléctricas apropiadas y ser realizada de acuerdo con las normas aplicables.

---

# 23. Expansión de entradas y salidas

Para la expansión de GPIOs y control de salidas digitales/PWM se consideran tres arquitecturas según la interfaz y topología requeridas:

---

### 1. Expansión vía I²C (Entradas/Salidas Bidireccionales)

Se utilizará el **MCP23017** (o su equivalente SPI **MCP23S17**).

Proporciona 16 GPIOs configurables como entrada o salida, dispone de interrupciones independientes (INTA/INTB) y permite hasta ocho dispositivos en el mismo bus mediante direccionamiento por hardware (pines A0, A1, A2).

---

### 2. Expansión vía SPI Independiente (Con pines CS dedicados)

Se utilizará el **MCP23S17**.

Es la versión SPI del MCP23017. Ofrece las mismas 16 E/S digitales e interrupciones por dispositivo, pero operando a mayor velocidad de bus. Requiere las líneas globales `SCLK`, `MOSI`, `MISO` y una línea de selección (*Chip Select*) dedicada por cada integrado adicional.

---

### 3. Expansión vía SPI en Cascada / Daisy Chain (Sin pines CS adicionales)

Se utilizará el **74HC595 / 74HCT595** (para salidas digitales) o el **TLC5947** (para salidas PWM de 12 bits).

Permiten encadenar múltiples integrados en serie (*Daisy Chain*) utilizando únicamente 3 pines del microcontrolador (`MOSI`, `SCLK`, `LATCH`), sin necesidad de añadir líneas CS adicionales a medida que crece el bus.

* **74HC595 / 74HCT595:** Registro de desplazamiento de 8 salidas tipo *totem-pole* (Push-Pull). Alimentado a 5V acepta niveles lógicos de 3.3V en sus entradas SPI y entrega 5V reales en las salidas.
* **TLC5947:** Expansor de 24 salidas PWM independientes por hardware.

La salida de datos del primer integrado (`DOUT` / `Q7'`) se conecta a la entrada de datos del siguiente (`DIN` / `DS`), compartiendo todos los chips el mismo reloj y señal de captura.

---

### Ejemplo de topologías:

```text
ESP32
 │
 ├── Bus I²C
 │    ├── ADS1115
 │    ├── SHT31
 │    ├── MCP23017 #1 (Dirección 0x20)
 │    └── MCP23017 #2 (Dirección 0x21)
 │
 ├── Bus SPI Independiente (Líneas CS dedicadas)
 │    ├── SCLK ──────┬──────────────┐
 │    ├── MOSI ──────┼──────────────┤
 │    ├── MISO ──────┼──────────────┤
 │    ├── CS_1 ───── MCP23S17 #1    │
 │    └── CS_2 ──────────────── MCP23S17 #2
 │
 └── Bus SPI en Cascada / Daisy Chain (3 pines fijos)
      ├── SCLK ──────┬──────────────┬──────────────┐
      ├── LATCH ─────┼──────────────┼──────────────┤
      └── MOSI ───> [DIN] 595 #1 [DOUT] ───> [DIN] 595 #2 [DOUT] ───> [DIN] 595 #3 ...

```

---

# 24. Organización de las salidas

Ejemplo:

```text
MCP23017 #1

GPIO A0 → Bomba 1
GPIO A1 → Válvula 1
GPIO A2 → Válvula 2
GPIO A3 → Válvula 3
GPIO A4 → Válvula 4
GPIO A5 → Ventilador 1
GPIO A6 → Ventilador 2
GPIO A7 → Extractor 1

GPIO B0 → Calefacción
GPIO B1 → Humidificador
GPIO B2 → Luz 1
GPIO B3 → Luz 2
GPIO B4 → Alarma
GPIO B5 → Reserva
GPIO B6 → Reserva
GPIO B7 → Reserva
```

Segundo expansor:

```text
MCP23017 #2

Ventana
Techo
Sombreado
Finales de carrera
Entradas digitales
Reservas
```

---

# 25. Entradas de seguridad

Las siguientes entradas tendrán prioridad sobre las órdenes normales:

```text
EMERGENCY_STOP

TANK_LOW

PUMP_FAULT

WINDOW_LIMIT_OPEN

WINDOW_LIMIT_CLOSE

ROOF_LIMIT_OPEN

ROOF_LIMIT_CLOSE
```

---

# 26. Control de techo y ventanas

En un invernadero exterior:

```text
Temperatura alta
+
humedad adecuada
+
sin lluvia
        ↓
abrir techo
```

Pero:

```text
Lluvia
     ↓
cerrar techo
```

También se deberá incorporar:

```text
FINAL_OPEN
FINAL_CLOSE
```

para evitar que el motor siga funcionando cuando ya llegó al extremo.

---

# 27. Invernadero interior

En un invernadero interior se deshabilitarán:

```text
Techo
Lluvia
Ventanas exteriores
```

y podrán habilitarse:

```text
Ventiladores
Extractores
Calefacción
Humidificador
Deshumidificador
Iluminación
CO₂
Riego
```

La misma aplicación seguirá funcionando.

---

# 28. Invernadero exterior

Podrá habilitar:

```text
Temperatura exterior
Humedad exterior
Lluvia
Techo
Ventanas
Ventilación
Sombreado
Riego
```

---

# 29. Iluminación natural

Si:

```text
Luz natural = suficiente
```

la iluminación artificial podrá permanecer apagada.

Si:

```text
Luz natural < mínimo
```

se podrá activar:

```text
LIGHT_1
```

---

# 30. Iluminación artificial

La configuración permitirá:

```text
Modo:
☐ Manual
☑ Horario
☐ Automático por luz
☐ Automático por PPFD
```

También:

```text
Hora inicio: 06:00
Hora final: 22:00

Intensidad: 80 %
```

---

# 31. Control de temperatura

La temperatura se controlará mediante histéresis.

Ejemplo:

```text
Objetivo: 24 °C

Ventilación ON:
28 °C

Ventilación OFF:
25 °C
```

Esto evita que el ventilador esté constantemente encendiéndose y apagándose.

---

# 32. Control de humedad

Ejemplo:

```text
RH mínima: 55 %
RH objetivo: 70 %
RH máxima: 85 %
```

Si:

```text
RH < 55 %
```

se podrá activar humidificación.

Si:

```text
RH > 85 %
```

se podrá activar ventilación o extracción.

---

# 33. Control de humedad del suelo

Cada zona podrá tener su propia configuración.

```text
ZONA 1

Mínimo: 35 %
Objetivo: 55 %
Máximo: 70 %
```

Y:

```text
ZONA 2

Mínimo: 45 %
Objetivo: 60 %
Máximo: 75 %
```

Esto evita obligar a todos los cultivos a utilizar el mismo nivel de humedad.

---

# 34. Sistema de riego

Cada zona tendrá:

```text
Sensor de suelo
Válvula
```

y podrá compartir:

```text
Bomba principal
```

Ejemplo:

```text
PUMP_1
   │
   ├── VALVE_1 → Zona 1
   ├── VALVE_2 → Zona 2
   ├── VALVE_3 → Zona 3
   └── VALVE_4 → Zona 4
```

---

# 35. Secuencia de riego

```text
Necesidad de riego
       ↓
comprobar tanque
       ↓
comprobar emergencia
       ↓
abrir válvula
       ↓
encender bomba
       ↓
esperar caudal
       ↓
confirmar caudal
       ↓
regar
       ↓
alcanzar objetivo
       ↓
bomba OFF
       ↓
válvula OFF
```

---

# 36. Protección de bomba

El sistema deberá detectar:

### Bomba funcionando sin agua

```text
Bomba ON
Caudal = 0
```

Resultado:

```text
Bomba OFF
ALARMA
```

### Rotura de tubería

```text
Caudal > máximo esperado
```

Resultado:

```text
Bomba OFF
ALARMA
```

### Tanque vacío

```text
FLOAT_LOW = activo
```

Resultado:

```text
Bomba bloqueada
```

---

# 37. Configuración desde página web

La configuración deberá permitir modificar sin programación:

```text
CONFIGURACIÓN GENERAL

Nombre
ID del dispositivo
Zona horaria
Unidades
Idioma
Modo de operación
```

---

# 38. Configuración de sensores

Cada sensor tendrá:

```text
Nombre
Tipo
Dirección
Zona
Habilitado
Intervalo de lectura
Calibración
Valor mínimo
Valor máximo
```

Ejemplo:

```text
Sensor:

Nombre:
Temperatura principal

Tipo:
SHT31

Zona:
General

Estado:
☑ Habilitado

Intervalo:
5 segundos
```

---

# 39. Configuración de actuadores

Cada actuador tendrá:

```text
Nombre
Tipo
GPIO lógico
Zona
Habilitado
Modo manual
Modo automático
Tiempo mínimo ON
Tiempo mínimo OFF
```

El usuario no deberá conocer qué GPIO físico se utiliza.

Por ejemplo:

```text
Ventilador principal
```

en lugar de:

```text
GPIO 27
```

---

# 40. Configuración de zonas

El sistema tendrá una estructura:

```text
Invernadero
│
├── Zona 1
│   ├── sensores
│   ├── válvula
│   └── riego
│
├── Zona 2
│   ├── sensores
│   ├── válvula
│   └── riego
│
└── Zona 3
```

Esto permitirá ampliar la instalación sin cambiar la arquitectura.

---

# 41. API REST

El ESP32 deberá disponer de:

```text
/api/v1/
```

### Estado general

```http
GET /api/v1/status
```

### Sensores

```http
GET /api/v1/sensors
```

### Actuadores

```http
GET /api/v1/actuators
```

### Configuración

```http
GET /api/v1/config
```

```http
PUT /api/v1/config
```

### Eventos

```http
GET /api/v1/events
```

### Alarmas

```http
GET /api/v1/alarms
```

---

# 42. Control de actuadores

Ejemplo:

```http
POST /api/v1/actuators/pump1
```

```json
{
    "state": true
}
```

Sin embargo, las órdenes manuales deberán estar sujetas al sistema de seguridad.

Por ejemplo, una orden:

```text
Bomba ON
```

será rechazada si:

```text
TANQUE VACÍO
```

---

# 43. Configuración

Ejemplo:

```http
PUT /api/v1/config
```

```json
{
    "climate": {
        "temperature_min": 18,
        "temperature_target": 24,
        "temperature_max": 28
    }
}
```

---

# 44. WebSocket

Se utilizará:

```text
/ws
```

para enviar datos en tiempo real.

Ejemplo:

```json
{
    "temperature": 24.6,
    "humidity": 71.2,
    "soil": [53, 49, 62],
    "tank": 76,
    "pump": false,
    "fan": true
}
```

Esto permitirá que el dashboard se actualice sin recargar la página.

---

# 45. MQTT

Cuando exista un servidor central se utilizará MQTT.

Ejemplo:

```text
greenhouse/GH001/state
greenhouse/GH001/sensors
greenhouse/GH001/actuators
greenhouse/GH001/events
greenhouse/GH001/alarms
greenhouse/GH001/config
greenhouse/GH001/cmd
```

---

# 46. Funcionamiento unitario

Cuando no existe servidor central:

```text
ESP32
│
├── Web local
├── API
├── WebSocket
├── Configuración
├── Automatización
└── Historial local
```

El usuario podrá acceder desde:

```text
http://invernadero.local
```

o mediante la IP asignada.

---

# 47. Funcionamiento centralizado

Cuando existe servidor:

```text
                   SERVIDOR

                 PostgreSQL
                     │
                 API REST
                     │
              Dashboard Web
                     │
                 MQTT Broker
                     │
       ┌─────────────┼─────────────┐
       │             │             │
     GH001         GH002         GH003
       │             │             │
     ESP32         ESP32         ESP32
```

---

# 48. Servidor central

El servidor podrá gestionar:

* usuarios;
* permisos;
* invernaderos;
* dispositivos;
* sensores;
* actuadores;
* configuraciones;
* históricos;
* alarmas;
* eventos;
* firmware;
* actualizaciones OTA.

---

# 49. Base de datos

Se recomienda PostgreSQL.

Tablas principales:

```text
users
greenhouses
devices
zones
sensors
actuators
sensor_readings
events
alarms
configurations
irrigation_events
firmware_versions
```

---

# 50. Identificación de dispositivos

Cada ESP32 tendrá:

```text
device_id
```

Ejemplo:

```text
GH-001
GH-002
GH-003
```

Además se almacenará:

```text
MAC
chip ID
firmware version
hardware version
last seen
```

---

# 51. Configuración almacenada

La configuración se guardará en memoria no volátil.

Ejemplo:

```json
{
    "device": {
        "id": "GH001",
        "name": "Invernadero Principal"
    },

    "features": {
        "temperature": true,
        "humidity": true,
        "soil": true,
        "co2": false,
        "rain": true,
        "tank": true,
        "flow": true,
        "roof": true,
        "lighting": true,
        "heating": false,
        "humidifier": false
    }
}
```

---

# 52. Ventaja del sistema de funciones

Esto permite que el firmware sea único.

Por ejemplo:

### Instalación A

```text
CO₂ = OFF
Calefacción = OFF
Techo = OFF
Riego = ON
```

### Instalación B

```text
CO₂ = ON
Calefacción = ON
Techo = ON
Riego = ON
```

Ambas utilizan el mismo firmware.

---

# 53. Detección automática de hardware

Cuando sea posible, el sistema podrá detectar:

```text
SHT31 encontrado
ADS1115 encontrado
MCP23017 encontrado
SCD41 no encontrado
```

La web podrá mostrar:

```text
DISPOSITIVOS DETECTADOS

✓ SHT31
✓ ADS1115
✓ MCP23017
✗ SCD41
✓ DS18B20 x 4
```

Pero la detección física no deberá habilitar automáticamente funciones peligrosas.

La activación deberá requerir confirmación/configuración.

---

# 54. Calibración

La web deberá incluir:

```text
CALIBRACIÓN
```

para:

### Humedad de suelo

```text
Seco
[Guardar]

Húmedo
[Guardar]
```

### pH

```text
pH 4
pH 7
pH 10
```

### EC

Según el sensor y solución de calibración correspondiente.

---

# 55. Diagnóstico

La página deberá disponer de:

```text
DIAGNÓSTICO

WiFi          OK
MQTT          OK
SHT31         OK
DS18B20       OK
ADS1115       OK
MCP23017      OK
Bomba         OK
Caudal        OK
Tanque        OK
```

---

# 56. Estado de sensores

Nunca se deberá asumir que una lectura es válida.

Cada sensor tendrá:

```text
OK
WARNING
ERROR
DISCONNECTED
OUT_OF_RANGE
```

Ejemplo:

```text
Temperatura:
24,5 °C
Estado: OK
```

Si un DS18B20 devuelve una lectura inválida:

```text
Temperatura:
---
Estado:
ERROR
```

La automatización deberá saber que ese valor no puede utilizarse.

---

# 57. Sistema de seguridad

La seguridad tendrá prioridad sobre la automatización.

Jerarquía:

```text
EMERGENCIA
     ↓
SEGURIDAD
     ↓
MANUAL
     ↓
AUTOMÁTICO
     ↓
PROGRAMACIÓN
```

---

# 58. Watchdog

El firmware deberá implementar watchdog.

Si una tarea se bloquea:

```text
bloqueo
   ↓
watchdog
   ↓
reinicio
   ↓
recuperación
```

Después del reinicio se restaurará la configuración.

---

# 59. OTA

El firmware deberá poder actualizarse sin conectar físicamente el ESP32.

```text
Servidor
   ↓
Firmware
   ↓
OTA
   ↓
ESP32
   ↓
verificación
   ↓
reinicio
```

Se deberá utilizar un sistema de rollback para evitar dejar inutilizado el dispositivo por una actualización defectuosa.

---

# 60. Página principal

La pantalla principal deberá mostrar:

```text
┌───────────────────────────────────────────┐
│ INVERNADERO PRINCIPAL                     │
│ Estado: AUTOMÁTICO                        │
├───────────────────────────────────────────┤
│                                           │
│ Temperatura       24,6 °C                 │
│ Humedad           71 %                    │
│ Suelo Z1          54 %                    │
│ Suelo Z2          61 %                    │
│ Tanque            76 %                    │
│ Luz               18.200 lux              │
│                                           │
├───────────────────────────────────────────┤
│ ACTUADORES                                │
│                                           │
│ Ventilador       ● ON                     │
│ Extractor        ○ OFF                    │
│ Bomba            ○ OFF                    │
│ Iluminación      ● ON                     │
│ Calefacción      ○ OFF                    │
│                                           │
├───────────────────────────────────────────┤
│ ALARMAS                                   │
│ ✓ Sin alarmas                             │
└───────────────────────────────────────────┘
```

---

# 61. Página de configuración

```text
CONFIGURACIÓN

[ Sensores ]
[ Actuadores ]
[ Zonas ]
[ Clima ]
[ Riego ]
[ Iluminación ]
[ Red ]
[ MQTT ]
[ Seguridad ]
[ Sistema ]
```

---

# 62. Configuración de funciones

Ejemplo:

```text
CONTROL CLIMÁTICO

☑ Control de temperatura
☑ Control de humedad
☑ Ventilación
☐ Calefacción
☐ Humidificación
☑ Control de CO₂
```

El usuario podrá habilitar o deshabilitar funciones sin modificar código.

---

# 63. Configuración del tipo de invernadero

Se incluirá:

```text
TIPO DE INSTALACIÓN

( ) Interior
( ) Exterior
( ) Mixto
```

Pero esta selección no determinará exclusivamente las funciones.

Por ejemplo, un invernadero exterior podrá tener:

```text
☑ Techo automático
☑ Ventilación
☑ Riego
☐ Calefacción
☐ CO₂
```

---

# 64. Configuración de clima

```text
TEMPERATURA

Mínimo:       18 °C
Objetivo:     24 °C
Máximo:       28 °C
Emergencia:   35 °C

HISTÉRESIS:
2 °C
```

---

# 65. Configuración de humedad

```text
HUMEDAD

Mínimo:       55 %
Objetivo:     70 %
Máximo:       85 %

HISTÉRESIS:
5 %
```

---

# 66. Configuración de riego

```text
RIEGO

Modo:
[ Automático ]

Humedad mínima:
35 %

Humedad objetivo:
55 %

Tiempo máximo:
15 min

Caudal mínimo:
1,0 L/min

Horario permitido:
04:00 - 09:00
18:00 - 22:00
```

---

# 67. Configuración de ventilación

```text
VENTILACIÓN

Activar:
28 °C

Desactivar:
25 °C

Humedad máxima:
85 %

Tiempo mínimo ON:
60 s

Tiempo mínimo OFF:
120 s
```

---

# 68. Configuración de techo

```text
TECHO

☑ Habilitado

Abrir:
28 °C

Cerrar:
24 °C

Cerrar por lluvia:
☑

Cerrar por viento:
☑

Final de carrera:
☑
```

---

# 69. Sensor de viento

Para instalaciones exteriores avanzadas se recomienda incorporar:

```text
anemómetro
```

Esto permite implementar una condición como:

```text
viento fuerte
     ↓
cerrar techo
```

Esta función es particularmente importante en estructuras exteriores.

---

# 70. Sombreado

Podrá configurarse:

```text
Luz máxima:
80.000 lux

Abrir sombreado:
> 80.000

Retirar sombreado:
< 60.000
```

También se podrá utilizar PPFD cuando exista un sensor compatible.

---

# 71. Automatización combinada

Las decisiones no se tomarán exclusivamente con un sensor.

Ejemplo:

```text
Temperatura alta
+
humedad alta
+
exterior más frío
+
sin lluvia
```

puede provocar:

```text
abrir techo
```

Mientras:

```text
temperatura alta
+
lluvia
```

puede provocar:

```text
cerrar techo
activar extractor
```

---

# 72. Prioridad de condiciones

Ejemplo:

```text
NORMAL
  ↓
temperatura alta
  ↓
ventilación

temperatura crítica
  ↓
ventilación máxima

lluvia
  ↓
cerrar techo

viento fuerte
  ↓
cerrar techo

emergencia
  ↓
estado seguro
```

---

# 73. Almacenamiento local

El ESP32 podrá almacenar:

* configuración;
* eventos recientes;
* alarmas;
* estadísticas;
* estado anterior.

Para históricos extensos se recomienda:

```text
servidor central
```

o almacenamiento externo como:

```text
microSD
```

si el sistema debe funcionar completamente desconectado.

---

# 74. Comunicación con servidor

La comunicación central será:

```text
ESP32
 │
 ├── MQTT → tiempo real
 │
 └── HTTPS REST → configuración/administración
```

El ESP32 deberá poder seguir funcionando si ambos servicios están desconectados.

---

# 75. Seguridad de comunicaciones

Para instalaciones conectadas a Internet se utilizará:

```text
HTTPS
MQTTS
```

y autenticación.

Espressif documenta soporte para OTA mediante HTTPS y mecanismos de seguridad como Secure Boot y Flash Encryption para aplicaciones de producción.

---

# 76. Seguridad de usuarios

La aplicación central deberá permitir:

```text
ADMIN
```

```text
OPERADOR
```

```text
CONSULTA
```

Por ejemplo:

### Administrador

Puede modificar todo.

### Operador

Puede:

* activar riego;
* modificar parámetros;
* consultar alarmas.

### Consulta

Solo puede visualizar.

---

# 77. Registro de eventos

Ejemplo:

```text
28/09/2026 08:01

Riego Zona 1 iniciado.

Motivo:
Humedad de suelo < 35 %

08:01:05

Caudal detectado:
2,7 L/min

08:08

Riego finalizado.

Humedad:
54 %
```

---

# 78. Alarmas

Se implementarán como mínimo:

```text
TEMP_HIGH
TEMP_LOW

HUMIDITY_HIGH
HUMIDITY_LOW

SOIL_DRY

TANK_LOW

PUMP_NO_FLOW
PUMP_FLOW_HIGH

SENSOR_ERROR

NETWORK_ERROR

MQTT_ERROR

ROOF_ERROR

WINDOW_ERROR

ACTUATOR_ERROR
```

---

# 79. Alarmas con recuperación

No todas las alarmas requieren intervención humana.

Ejemplo:

```text
WiFi desconectado
```

No deberá detener el invernadero.

En cambio:

```text
Bomba ON
Caudal = 0
```

deberá detener la bomba.

---

# 80. Arquitectura de software

El firmware se dividirá en módulos:

```text
src/

main.cpp

config/
    ConfigManager
    Defaults
    Calibration

sensors/
    SensorManager
    SHT31
    AHT20
    DS18B20
    SoilMoisture
    BH1750
    CO2
    Flow
    Tank
    Rain
    Wind

actuators/
    ActuatorManager
    Pump
    Valve
    Fan
    Heater
    Humidifier
    Light
    Window
    Roof
    Shade

control/
    ClimateController
    IrrigationController
    LightingController
    RoofController
    SafetyController

network/
    WiFiManager
    MQTTManager
    NetworkManager

api/
    REST
    WebSocket
    Authentication

storage/
    NVS
    History

system/
    Watchdog
    OTA
    Diagnostics
```

---

# 81. Principio de abstracción

El controlador no deberá tener código como:

```cpp
if (SHT31.temperature() > 28)
```

directamente en toda la aplicación.

En su lugar:

```text
SensorManager
      ↓
temperature
      ↓
ClimateController
      ↓
ActuatorManager
      ↓
fan
```

Esto permitirá cambiar SHT31 por AHT20 sin modificar el sistema de climatización.

---

# 82. Configuración por JSON

La configuración podría tener una estructura semejante a:

```json
{
  "greenhouse": {
    "id": "GH001",
    "name": "Invernadero Principal",
    "type": "outdoor"
  },

  "features": {
    "climate": true,
    "irrigation": true,
    "lighting": true,
    "co2": false,
    "heating": false,
    "humidification": false,
    "roof": true,
    "windows": false,
    "shade": true
  },

  "sensors": {
    "sht31": true,
    "ds18b20": true,
    "soil": true,
    "light": true,
    "co2": false,
    "rain": true,
    "tank": true,
    "flow": true
  },

  "actuators": {
    "pump": true,
    "valves": 4,
    "fans": 2,
    "extractors": 1,
    "lights": 1,
    "heater": false,
    "humidifier": false,
    "roof": true
  }
}
```

---

# 83. Principio de seguridad de configuración

Deshabilitar un elemento deberá significar que:

```text
no se utiliza
```

pero no deberá provocar:

```text
GPIO flotante
```

ni activar accidentalmente una salida.

Al arrancar:

```text
todas las salidas → estado seguro
```

y posteriormente:

```text
cargar configuración
        ↓
inicializar hardware
        ↓
verificar sensores
        ↓
iniciar automatización
```

---

# 84. Configuración predeterminada

El firmware deberá incorporar una configuración inicial.

Ejemplo:

```text
Temperatura:
18 / 24 / 28 °C

Humedad:
55 / 70 / 85 %

Suelo:
35 / 55 / 70 %

Riego:
automático deshabilitado hasta configuración

CO₂:
deshabilitado

Calefacción:
deshabilitada

Techo:
deshabilitado

Iluminación:
deshabilitada
```

Esto evita activar automáticamente un actuador que físicamente todavía no fue configurado.

---

# 85. Restablecimiento de fábrica

La placa deberá disponer de:

```text
BOOT / RESET
```

y un procedimiento como:

```text
mantener botón 5 segundos
```

para:

```text
restaurar configuración
```

pero sin borrar necesariamente el firmware.

---

# 86. Conectividad inicial

Al instalar un ESP32 nuevo:

```text
ESP32
 ↓
Access Point temporal
 ↓
configuración WiFi
 ↓
red local
 ↓
mDNS
```

El usuario podrá configurar:

```text
SSID
Password
hostname
MQTT
servidor
zona horaria
```

---

# 87. Arquitectura de red

El dispositivo deberá soportar:

```text
WiFi
```

y opcionalmente:

```text
Ethernet
```

en una versión futura.

Para instalaciones grandes, Ethernet puede resultar más apropiado que Wi-Fi.

---

# 88. CAN/TWAI futuro

El diseño deberá dejar abierta la posibilidad de utilizar:

```text
CAN / TWAI
```

para nodos distribuidos.

Ejemplo:

```text
ESP32 CENTRAL
      │
     CAN
      │
 ┌────┼─────┐
 │    │     │
Nodo Nodo  Nodo
Riego Clima Actuadores
```

Esto permitirá evitar largas conexiones de sensores hacia un único ESP32.

---

# 89. Diseño modular por nodos

Para un invernadero pequeño:

```text
1 ESP32
```

Para uno grande:

```text
ESP32 central
+
ESP32 riego
+
ESP32 clima
+
ESP32 actuadores
```

Todos podrán utilizar el mismo concepto de firmware.

---

# 90. Lista de materiales recomendada

## Control

* ESP32 DevKitC / ESP32 DOIT / ESP32-S3.
* Fuente 5 V adecuada.
* Regulación 3,3 V si corresponde.
* Caja eléctrica.

## Sensores económicos/recomendados

* SHT31.
* DS18B20.
* Sensores capacitivos de humedad.
* ADS1115.
* BH1750.
* Caudalímetro de pulsos.
* Flotadores.
* Sensor ultrasónico impermeable.

## Sensores avanzados

* SCD40/SCD41.
* PAR/PPFD.
* pH.
* EC.
* anemómetro.
* pluviómetro.

## Expansión

* MCP23017.
* Borneras.
* Optoacopladores.
* MOSFET.
* Diodos flyback.
* SSR.
* Relés.
* Contactores.

## Actuadores

* Bombas.
* Electroválvulas.
* Ventiladores.
* Extractores.
* Iluminación.
* Calefactor.
* Humidificador.
* Motores/actuadores lineales.
* Sistema de sombreado.

---

# 91. Selección por relación precio/prestaciones

La estrategia será:

### Económico

```text
ESP32
SHT31/AHT20
DS18B20
capacitivo
BH1750
ADS1115
MCP23017
```

### Intermedio

Agregar:

```text
caudal
nivel
lluvia
viento
```

### Avanzado

Agregar:

```text
SCD40/SCD41
PAR
pH
EC
```

Esto permite que el mismo proyecto se adapte a diferentes presupuestos.

---

# 92. Elementos que NO serán obligatorios

El firmware no deberá exigir:

```text
CO₂
pH
EC
PAR
lluvia
viento
techo
calefacción
humidificador
```

Todos serán módulos opcionales.

---

# 93. Elementos que sí se consideran recomendados

Para una instalación básica:

```text
ESP32
SHT31
DS18B20
humedad de suelo
nivel de tanque
caudal
bomba
válvula
ventilación
```

Con esto ya se puede construir un sistema agrícola funcional.

---

# 94. Instalación exterior recomendada

Configuración:

```text
ESP32
│
├── SHT31 interior
├── SHT31 exterior
├── DS18B20 agua
├── DS18B20 suelo
├── 4 × humedad suelo
├── BH1750
├── lluvia
├── viento
├── nivel
├── caudal
│
├── bomba
├── 4 válvulas
├── 2 ventiladores
├── extractor
├── techo
└── sombreado
```

---

# 95. Instalación interior recomendada

```text
ESP32
│
├── SHT31
├── DS18B20
├── 4 × humedad suelo
├── BH1750
├── nivel
├── caudal
│
├── bomba
├── 4 válvulas
├── ventiladores
├── extractor
├── iluminación
├── calefacción
└── humidificador
```

---

# 96. Instalación avanzada

```text
ESP32
│
├── SHT31
├── DS18B20
├── suelo
├── CO₂
├── PAR
├── pH
├── EC
├── temperatura exterior
├── humedad exterior
├── lluvia
├── viento
├── nivel
└── caudal

ACTUADORES

├── bomba
├── válvulas
├── ventiladores
├── extractores
├── calefacción
├── humidificador
├── luces
├── CO₂
├── techo
├── ventanas
└── sombreado
```

---

# 97. Filosofía de funcionamiento

El usuario no debería tener que pensar:

```text
¿Qué GPIO uso?
¿Qué librería necesito?
¿Cómo activo el sensor?
¿Qué código tengo que modificar?
```

Debe pensar:

```text
Tengo un sensor SHT31.

→ Lo conecto.

→ Entro a Configuración.

→ Sensores.

→ Agregar sensor.

→ SHT31.

→ Habilitar.

→ Guardar.
```

Y para un actuador:

```text
Tengo un ventilador.

→ Lo conecto al canal correspondiente.

→ Configuración.

→ Actuadores.

→ Ventilador 1.

→ Habilitar.

→ Definir función.

→ Guardar.
```

---

# 98. Principio de escalabilidad

El proyecto deberá poder comenzar con:

```text
ESP32
+
SHT31
+
DS18B20
+
1 humedad suelo
+
1 bomba
+
1 ventilador
```

y posteriormente crecer hasta:

```text
ESP32
+
varios expansores
+
varias zonas
+
CO₂
+
pH
+
EC
+
iluminación
+
techo
+
ventanas
+
servidor central
```

sin cambiar la arquitectura principal.

---

# 99. Arquitectura final

```text
                         SERVIDOR CENTRAL
                               │
                    ┌──────────┴──────────┐
                    │                     │
                  REST                  MQTT
                    │                     │
                    └──────────┬──────────┘
                               │
                        ┌──────▼──────┐
                        │    ESP32    │
                        │             │
                        │ Web local   │
                        │ REST API    │
                        │ WebSocket   │
                        │ MQTT        │
                        │ OTA         │
                        │ NVS         │
                        │ Watchdog    │
                        └──────┬──────┘
                               │
                ┌──────────────┼──────────────┐
                │              │              │
             I²C BUS        1-WIRE          GPIO
                │              │              │
        ┌───────┼──────┐       │       ┌──────┼───────┐
        │       │      │       │       │      │       │
      SHT31  ADS1115 MCP23017 DS18B20 Flow  Level   Rain
        │       │      │       │       │      │       │
        └───────┴──────┴───────┴───────┴──────┴───────┘
                               │
                         DRIVERS
                               │
             ┌─────────────────┼─────────────────┐
             │                 │                 │
           BOMBAS           VÁLVULAS        VENTILACIÓN
             │                 │                 │
             └─────────────────┼─────────────────┘
                               │
                        INVERNADERO
```

# 100. Conclusión

El proyecto deberá desarrollarse como una **plataforma de automatización modular**, no como un firmware específico para una única instalación.

La elección de sensores se realizará priorizando:

1. disponibilidad;
2. precio;
3. precisión suficiente;
4. facilidad de reemplazo;
5. documentación;
6. facilidad de integración;
7. confiabilidad.

Por este motivo, los sensores de alta gama como CO₂ NDIR, PAR, pH y EC serán módulos opcionales, mientras que el núcleo utilizará componentes de mejor relación precio/prestaciones como SHT31, DS18B20, sensores capacitivos, ADS1115, MCP23017 y BH1750. El SHT31, por ejemplo, proporciona una precisión de aproximadamente ±2 %RH y ±0,2 °C, mientras que el DS18B20 ofrece ±0,5 °C en su rango especificado.

El resultado deberá ser un único sistema capaz de adaptarse mediante configuración web a:

```text
INVERNADERO INTERIOR
       +
INVERNADERO EXTERIOR
       +
VENTILACIÓN
       +
CALEFACCIÓN
       +
ILUMINACIÓN
       +
RIEGO
       +
TECHO AUTOMÁTICO
       +
VENTANAS
       +
SOMBREADO
       +
CO₂
       +
pH / EC
```

sin necesidad de modificar el código fuente.

La configuración será almacenada en memoria no volátil y el sistema deberá conservar la capacidad de funcionar autónomamente aun cuando se pierda la comunicación con el servidor central.

El ESP32 será, por tanto, el **controlador de campo autónomo**, mientras que el servidor central será la **capa de supervisión, configuración, históricos y administración de múltiples dispositivos**.

La incorporación de RS485 cambia de forma importante la conclusión original. El proyecto deja de ser simplemente un controlador ESP32 con sensores conectados directamente y pasa a plantearse como una **plataforma modular de automatización de invernaderos y fertirriego**, capaz de combinar sensores económicos directamente conectados con instrumentación industrial mediante buses de comunicación.

La arquitectura final queda conceptualmente:

```text
                         INVERNADERO
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
    SENSORES                ESP32              ACTUADORES
        │                     │                     │
        ├─ I²C                │                     ├─ Bombas
        ├─ 1-Wire             │                     ├─ Válvulas
        ├─ ADC                │                     ├─ Ventiladores
        ├─ GPIO               │                     ├─ Iluminación
        └─ RS485/Modbus ──────┤                     ├─ Calefacción
                              │                     ├─ Humidificación
                              │                     ├─ Ventanas
                              │                     └─ Techo
                              │
                 ┌────────────┴────────────┐
                 │                         │
             WEB LOCAL                API / MQTT
                 │                         │
                 └────────────┬────────────┘
                              │
                       SERVIDOR CENTRAL
                              │
                    ┌─────────┴─────────┐
                    │                   │
                 Historial          Panel Web
                 PostgreSQL         Multinvernadero
```

El uso de **RS485/Modbus** permite además separar físicamente los sensores del controlador, reducir la cantidad de entradas necesarias en el ESP32 y facilitar la expansión de la instalación. Una misma infraestructura puede comenzar con sensores económicos como **SEN0161/SEN024**, y posteriormente incorporar transmisores industriales sin tener que rediseñar completamente el controlador.

Por lo tanto, el sistema se diseña desde el principio con una filosofía **modular, escalable y multiprotocolo**, donde cada sensor o actuador puede habilitarse, deshabilitarse, configurarse y calibrarse desde la interfaz web.

La combinación de **ESP32 + I²C + SPI + 1-Wire + ADC externo + RS485/Modbus + expansión de salidas mediante 74HC595/74HCT595** permite cubrir desde un pequeño invernadero doméstico hasta instalaciones de mayor complejidad con fertirriego, climatización, instrumentación industrial y múltiples zonas de control.

La principal ventaja de esta arquitectura es que **la instalación no queda atada a un único tipo de sensor ni a una cantidad fija de entradas y salidas del ESP32**. El controlador constituye el núcleo del sistema y los sensores, actuadores y módulos de comunicación funcionan como componentes intercambiables.

Esto también permite que una futura versión del proyecto incorpore sensores Modbus adicionales simplemente configurando su **dirección, registros y parámetros de comunicación desde la interfaz web**, sin modificar necesariamente el firmware principal.

# 101. Evolución de la arquitectura del proyecto

A partir de las funcionalidades implementadas en las versiones anteriores, el proyecto evolucionará desde un controlador de invernadero basado en ESP32 hacia una plataforma modular de automatización distribuida.

La arquitectura deberá permitir que una instalación pequeña funcione únicamente con un ESP32, mientras que instalaciones grandes puedan utilizar múltiples nodos, buses RS485/Modbus, Ethernet, WiFi y uno o varios gateways conectados a un servidor central.

La arquitectura propuesta será:

```text
                         SERVIDOR CENTRAL
                                │
                  ┌─────────────┴─────────────┐
                  │                           │
               HTTPS/API                    MQTT
                  │                           │
                  └─────────────┬─────────────┘
                                │
                         RED IP / LAN / WAN
                                │
              ┌─────────────────┼─────────────────┐
              │                 │                 │
          GATEWAY A         GATEWAY B         ESP32 DIRECTO
       Ethernet / WiFi    Ethernet / WiFi          │
              │                 │                   │
           RS485             RS485               Sensores
              │                 │                Actuadores
       ┌──────┼──────┐      ┌───┼────┐
       │      │      │      │   │    │
      pH     EC    ORP     Temp CO₂ Caudal
      │       │      │      │   │    │
      └───────┴──────┴──────┴───┴────┘
                    BUS INDUSTRIAL
```

El sistema deberá conservar siempre la siguiente propiedad fundamental:

```text
SERVIDOR CENTRAL = supervisión, administración y coordinación

ESP32 / NODO DE CAMPO = control autónomo y seguridad
```

La pérdida de comunicación con el servidor nunca deberá provocar la pérdida del control básico del invernadero.

---

# 102. Principio de autonomía local

Cada dispositivo deberá almacenar localmente toda la información necesaria para continuar funcionando.

Se almacenará localmente:

* configuración de sensores;
* configuración de actuadores;
* parámetros de automatización;
* horarios;
* límites de seguridad;
* configuración de red;
* configuración RS485;
* configuración Modbus;
* calibraciones;
* identificación del dispositivo;
* identificación del invernadero;
* configuración de zonas;
* estado de funcionamiento;
* modo de operación;
* parámetros de recuperación;
* configuración del servidor central;
* versión de firmware;
* parámetros de actualización OTA.

La memoria no volátil deberá utilizar un sistema de configuración con versión.

Ejemplo:

```json
{
    "config_version": 12,
    "device_id": "GH-001",
    "greenhouse_id": "GREENHOUSE-001",
    "mode": "AUTO"
}
```

Si el dispositivo pierde:

```text
Internet
WiFi
Ethernet
MQTT
Servidor central
DNS
```

deberá continuar funcionando con la última configuración válida almacenada localmente.

---

# 103. Modelo de configuración distribuida

La configuración tendrá dos posibles fuentes:

```text
MODO LOCAL

ESP32
  │
  └── Web local
       │
       └── Configuración local


MODO SERVIDOR CENTRAL

Servidor
   │
   └── Configuración remota
          │
          ↓
        ESP32
          │
          └── Guarda copia local
```

La configuración deberá tener un propietario lógico.

Se propone:

```json
{
    "configuration_source": "LOCAL"
}
```

o:

```json
{
    "configuration_source": "CENTRAL"
}
```

Cuando:

```text
configuration_source = LOCAL
```

la configuración podrá modificarse desde la página web del ESP32.

Cuando:

```text
configuration_source = CENTRAL
```

la configuración deberá administrarse desde el servidor central.

La página local seguirá disponible para:

* visualizar información;
* consultar diagnósticos;
* comprobar conectividad;
* consultar versión;
* consultar alarmas;
* realizar mantenimiento;
* acceder a recuperación;
* realizar determinadas operaciones de emergencia.

Sin embargo, las configuraciones operativas principales deberán quedar bloqueadas o en modo solo lectura cuando el dispositivo esté administrado por el servidor central.

---

# 104. Política de sincronización de configuración

Toda configuración deberá utilizar un sistema de versiones.

Ejemplo:

```text
config_version = 42
```

Cuando el servidor central modifica una configuración:

```text
Servidor
   │
   │ Config v43
   ↓
ESP32
   │
   ├── valida
   ├── aplica
   ├── guarda
   └── confirma
```

El dispositivo deberá responder:

```json
{
    "device_id": "GH-001",
    "config_version": 43,
    "status": "APPLIED"
}
```

Si la configuración no puede aplicarse:

```json
{
    "device_id": "GH-001",
    "config_version": 43,
    "status": "REJECTED",
    "reason": "INVALID_SENSOR_CONFIGURATION"
}
```

La configuración anterior deberá conservarse hasta que la nueva configuración sea validada correctamente.

Se recomienda implementar:

```text
CONFIG ACTUAL
CONFIG NUEVA
CONFIG ANTERIOR
```

para permitir rollback de configuración.

---

# 105. Página web local del ESP32

El ESP32 deberá disponer de una interfaz web local completa.

Acceso mediante:

```text
http://invernadero.local
```

o:

```text
http://invernadero-esp32.local
```

o mediante la dirección IP asignada.

La interfaz deberá estar dividida en módulos.

## Dashboard

Mostrar:

* temperatura;
* humedad;
* presión;
* VPD;
* CO₂;
* pH;
* EC;
* ORP;
* humedad del suelo;
* nivel de tanques;
* caudal;
* lluvia;
* velocidad del viento;
* dirección del viento;
* luminosidad;
* PAR/PPFD;
* estado de bombas;
* estado de válvulas;
* ventiladores;
* iluminación;
* calefacción;
* ventanas;
* techo;
* sombreado;
* alarmas;
* conexión al servidor;
* conexión WiFi/Ethernet;
* estado MQTT;
* estado RS485;
* versión del firmware.

Los sensores no instalados no deberán aparecer como errores.

Deberán diferenciarse:

```text
NO INSTALADO
DESHABILITADO
SIN COMUNICACIÓN
ERROR
VALOR VÁLIDO
VALOR FUERA DE RANGO
```

---

# 106. Configuración de red

La página web deberá incorporar un apartado específico:

```text
CONFIGURACIÓN
 └── RED
```

Debe permitir seleccionar:

```text
☑ WiFi
☐ Ethernet
```

y configurar:

* SSID;
* contraseña;
* DHCP;
* IP estática;
* máscara;
* gateway;
* DNS primario;
* DNS secundario;
* hostname;
* mDNS;
* prioridad de interfaz;
* servidor NTP;
* zona horaria;
* servidor central;
* MQTT;
* HTTPS;
* puerto HTTP;
* puerto HTTPS;
* timeout;
* reconexión automática.

---

# 107. Configuración WiFi mediante búsqueda de redes

La configuración WiFi deberá incorporar un asistente.

Al presionar:

```text
BUSCAR REDES
```

el ESP32 realizará un escaneo WiFi.

La interfaz mostrará:

```text
REDES DISPONIBLES

☑ Casa
☑ Invernadero
☑ Oficina
☑ IoT
☑ ESP32-Gateway
```

Al seleccionar una red:

```text
SSID:
[ Invernadero              ]

Contraseña:
[ *********************** ]

[ CONECTAR ]
```

El usuario no deberá escribir manualmente el SSID.

Se deberá mostrar:

* intensidad de señal;
* canal;
* tipo de seguridad;
* SSID;
* estado de conexión.

---

# 108. Modo Access Point de configuración

El ESP32 deberá disponer de un modo AP de recuperación.

Ejemplo:

```text
Invernadero-Setup
```

El usuario podrá conectarse directamente al ESP32 aunque no exista una red WiFi configurada.

La página de configuración deberá permitir:

```text
1. Seleccionar red WiFi
2. Introducir contraseña
3. Configurar IP
4. Configurar DNS
5. Configurar hostname
6. Configurar mDNS
7. Configurar servidor central
8. Guardar
9. Reiniciar
```

Si no se consigue conexión después de un número configurable de intentos, el dispositivo deberá poder volver automáticamente al modo AP de configuración.

---

# 109. Ethernet

La arquitectura deberá contemplar Ethernet como interfaz de red de primera clase.

No deberá considerarse únicamente como una función futura aislada.

El software deberá abstraer la interfaz de red:

```text
NetworkInterface
       │
       ├── WiFi
       │
       └── Ethernet
```

De esta manera, las capas superiores no deberán saber si la comunicación utiliza WiFi o Ethernet.

El sistema deberá permitir:

```text
WiFi solamente
Ethernet solamente
WiFi + Ethernet
```

Cuando ambas interfaces estén disponibles podrá configurarse una prioridad.

Ejemplo:

```text
PRIORIDAD:

1. Ethernet
2. WiFi
```

o:

```text
1. WiFi
2. Ethernet
```

---

# 110. Ethernet como Gateway industrial

Una función especialmente importante será permitir que determinados ESP32 funcionen como gateways.

Ejemplo:

```text
                    SERVIDOR CENTRAL
                           │
                      Ethernet/WiFi
                           │
                    ESP32 GATEWAY
                           │
                         RS485
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
      SENSOR             SENSOR             SENSOR
       pH                  EC                 ORP
```

El gateway podrá:

* leer sensores Modbus;
* controlar actuadores Modbus;
* administrar el bus;
* almacenar temporalmente datos;
* convertir Modbus → MQTT;
* convertir Modbus → REST;
* convertir Modbus → WebSocket;
* reenviar alarmas;
* realizar diagnóstico;
* administrar dispositivos RS485.

Esto permitirá utilizar una placa de la misma familia del proyecto como **gateway universal**.

---

# 111. Arquitectura de nodos

Se definirán diferentes tipos de dispositivos.

## FIELD NODE

Controlador de campo.

```text
ESP32
+
Sensores
+
Actuadores
```

## RS485 NODE

Dispositivo conectado al bus industrial.

```text
Sensor
+
Microcontrolador
+
RS485
```

## GATEWAY NODE

Dispositivo que conecta:

```text
RS485
     ↕
ESP32
     ↕
Ethernet / WiFi
```

## CENTRAL SERVER

Servidor que administra:

```text
Gateways
Field Nodes
Sensores
Actuadores
Invernaderos
Usuarios
Históricos
Alarmas
Firmware
Configuraciones
```

---

# 112. Bus RS485 generalizado

RS485 deberá convertirse en una capa común del proyecto.

No estará limitada a pH.

Se deberá contemplar:

* pH;
* EC;
* ORP;
* temperatura;
* humedad;
* CO₂;
* oxígeno disuelto;
* caudal;
* presión;
* nivel;
* radiación;
* PAR;
* sensores meteorológicos;
* estaciones meteorológicas;
* analizadores de agua;
* sensores industriales;
* módulos de entradas digitales;
* módulos de entradas analógicas;
* módulos de salidas digitales;
* módulos de relés;
* módulos de control de motores.

La arquitectura será:

```text
ESP32
 │
 └── RS485
      │
      ├── pH
      ├── EC
      ├── ORP
      ├── Temp
      ├── CO₂
      ├── Caudal
      ├── Nivel
      └── Módulos I/O
```

---

# 113. Abstracción de sensores industriales

El firmware no deberá tratar cada sensor Modbus como un caso completamente independiente.

Se deberá crear una abstracción:

```text
IndustrialSensor
```

con parámetros como:

```text
device_id
slave_id
manufacturer
model
protocol
baudrate
parity
stop_bits
register_map
poll_interval
timeout
retry_count
calibration
unit
range_min
range_max
```

Ejemplo:

```json
{
    "device_id": "SENSOR-PH-001",
    "interface": "RS485",
    "protocol": "MODBUS_RTU",
    "slave_id": 10,
    "baudrate": 9600,
    "parity": "NONE",
    "stop_bits": 1,
    "poll_interval": 5000
}
```

---

# 114. Perfiles de sensores Modbus

Para evitar tener que programar manualmente cada sensor, se deberá utilizar un sistema de perfiles.

Ejemplo:

```text
MODBUS SENSOR PROFILE

Nombre:
DFRobot pH Industrial

Fabricante:
DFRobot

Modelo:
XXXX

Baudrate:
9600

Registro pH:
40001

Tipo:
FLOAT32

Escala:
0.01

Unidad:
pH
```

Otros perfiles:

```text
PH_GENERIC
EC_GENERIC
ORP_GENERIC
TEMP_GENERIC
CO2_GENERIC
FLOW_GENERIC
LEVEL_GENERIC
PRESSURE_GENERIC
WEATHER_GENERIC
```

El sistema deberá permitir agregar nuevos perfiles sin modificar la lógica principal del sistema.

---

# 115. Auto descubrimiento RS485

El sistema deberá incorporar una función de descubrimiento de dispositivos.

Desde la interfaz:

```text
RS485
   ↓
BUSCAR DISPOSITIVOS
```

El gateway realizará un escaneo de:

```text
baudrate
paridad
stop bits
direcciones
```

cuando corresponda.

El resultado podrá ser:

```text
DISPOSITIVOS ENCONTRADOS

ID 10
Fabricante: XXXXX
Modelo: pH Sensor
Estado: OK

ID 11
Fabricante: XXXXX
Modelo: EC Sensor
Estado: OK

ID 12
Fabricante: XXXXX
Modelo: Temperature Sensor
Estado: OK
```

---

# 116. Asignación automática de ID Modbus

El protocolo Modbus RTU no define por sí mismo un mecanismo universal para descubrir y asignar automáticamente direcciones únicas a todos los dispositivos.

Por este motivo, el proyecto deberá implementar un procedimiento de commissioning propio para los dispositivos que sean diseñados específicamente para esta plataforma.

Cada dispositivo de la familia deberá poseer un identificador único permanente:

```text
DEVICE_UID
```

Ejemplo:

```text
INV-7F29A8C2
```

Durante el proceso de incorporación:

```text
DEVICE UID
      ↓
DISCOVERY
      ↓
IDENTIFICACIÓN
      ↓
ASIGNACIÓN DE SLAVE ID
      ↓
GUARDADO
      ↓
VERIFICACIÓN
```

La dirección Modbus asignada podrá cambiar, pero el UID deberá permanecer permanente.

Esto permite que el servidor central identifique el dispositivo aunque posteriormente cambie su dirección Modbus.

---

# 117. Modo de incorporación de dispositivos

Se implementará un modo:

```text
PAIRING / COMMISSIONING
```

En este modo un dispositivo nuevo podrá incorporarse al sistema.

Ejemplo:

```text
1. Conectar dispositivo RS485
2. Buscar dispositivos
3. Detectar UID
4. Seleccionar dispositivo
5. Asignar nombre
6. Asignar función
7. Asignar zona
8. Asignar dirección Modbus
9. Guardar
10. Probar comunicación
11. Registrar en servidor central
```

El dispositivo deberá quedar registrado como:

```text
PROVISIONED
```

Una vez finalizado el proceso.

---

# 118. Identidad de dispositivos

La dirección Modbus no deberá utilizarse como identidad principal.

Se utilizará:

```text
DEVICE_UID
```

y además:

```text
device_id
serial_number
hardware_revision
firmware_version
```

La dirección Modbus será solamente un parámetro de comunicación.

Ejemplo:

```text
DEVICE_UID:
INV-SENSOR-000124

MODBUS_ID:
17
```

Si posteriormente cambia:

```text
MODBUS_ID:
21
```

seguirá siendo el mismo dispositivo.

---

# 119. Configuración automática de sensores industriales

La interfaz deberá poder mostrar:

```text
NUEVO SENSOR DETECTADO

UID:
INV-SENSOR-000124

Tipo:
pH

Fabricante:
XXXXX

Modelo:
XXXXX

¿Agregar al sistema?

[ AGREGAR ]
```

Después:

```text
Nombre:
pH Tanque 1

Zona:
Fertirriego

Unidad:
pH

Intervalo:
5 segundos

[ GUARDAR ]
```

No deberá ser necesario modificar el código fuente.

---

# 120. Gestión de zonas

El sistema deberá incorporar una abstracción de zonas.

Ejemplo:

```text
INVERNADERO
│
├── ZONA 1
│   ├── Sensores
│   ├── Riego
│   └── Ventilación
│
├── ZONA 2
│   ├── Sensores
│   ├── Riego
│   └── Ventilación
│
├── ZONA 3
│
└── ZONA FERTIRRIEGO
```

Cada sensor y actuador deberá poder pertenecer a una zona.

Esto permitirá controlar instalaciones grandes sin crear lógica específica para cada instalación.

---

# 121. Motor de automatización por reglas

Se deberá evolucionar el sistema automático hacia un motor de reglas configurable.

Ejemplo:

```text
SI
    temperatura > 28 °C
Y
    ventilador habilitado
ENTONCES
    FAN_01 = ON
```

Otro ejemplo:

```text
SI
    pH < 5.8
ENTONCES
    ALARMA = ACTIVE
```

Otro:

```text
SI
    humedad_suelo < 35 %
Y
    tanque > 20 %
Y
    caudal = OK
ENTONCES
    BOMBA = ON
```

Las reglas deberán poder configurarse desde la interfaz web.

---

# 122. Interlocks de seguridad

El motor de automatización deberá incorporar interlocks.

Ejemplo:

```text
BOMBA ON
   ↓
DEBE EXISTIR CAUDAL
```

Si no existe:

```text
BOMBA ON
CAUDAL = 0
   ↓
ALARMA
   ↓
BOMBA OFF
```

Otros interlocks:

```text
Tanque vacío → bomba OFF

Viento excesivo → techo OFF

Lluvia → techo cerrado

Temperatura excesiva → ventilación ON

Falla de sensor crítico → modo seguro

Sobrecorriente → actuador OFF
```

---

# 123. Históricos locales

El ESP32 deberá mantener un buffer local de datos.

Ejemplo:

```text
Sensor
  ↓
Memoria local
  ↓
Servidor central
```

Si el servidor está desconectado:

```text
Sensor
  ↓
Memoria local
       X servidor
```

Cuando vuelva la conexión:

```text
Memoria local
      ↓
SYNC
      ↓
Servidor central
```

Esto permitirá evitar pérdidas de datos durante interrupciones temporales.

---

# 124. Sincronización de históricos

Cada registro deberá tener:

```text
timestamp
device_id
sensor_id
value
unit
quality
```

Ejemplo:

```json
{
    "timestamp": "2026-09-28T12:00:00-03:00",
    "device_id": "GH-001",
    "sensor_id": "TEMP-01",
    "value": 24.6,
    "unit": "C",
    "quality": "GOOD"
}
```

La calidad deberá permitir:

```text
GOOD
WARNING
INVALID
TIMEOUT
OUT_OF_RANGE
CALIBRATION
DISCONNECTED
```

---

# 125. Fecha y hora

La configuración deberá incluir:

```text
Zona horaria
NTP
Servidor NTP
Horario de verano
Formato de fecha
Formato de hora
```

La zona horaria deberá preferentemente almacenarse como identificador IANA.

Ejemplo:

```text
America/Argentina/Buenos_Aires
```

en lugar de almacenar solamente:

```text
UTC-3
```

Esto permite representar correctamente las reglas de zona horaria.

---

# 126. DNS

La página de configuración deberá permitir:

```text
DNS automático
DNS manual
```

Ejemplo:

```text
DNS 1:
1.1.1.1

DNS 2:
8.8.8.8
```

El sistema deberá permitir además resolver:

```text
servidor.invernadero.local
```

o un dominio configurado por el usuario.

---

# 127. mDNS

Cada dispositivo deberá disponer de hostname configurable.

Ejemplo:

```text
gh-001.local
gh-002.local
gateway-001.local
```

El hostname deberá derivarse inicialmente del `device_id`, pero podrá modificarse desde la web.

---

# 128. Configuración del servidor central

La página local deberá disponer de:

```text
SERVIDOR CENTRAL

☐ Administrado por servidor central

URL:
https://servidor.example.com

Puerto:
443

MQTT:
broker.example.com

Puerto MQTT:
8883

Device ID:
GH-001

Token:
**************

[ PROBAR CONEXIÓN ]
```

Si:

```text
☐ Administrado por servidor central
```

el dispositivo funcionará de forma local.

Si:

```text
☑ Administrado por servidor central
```

el dispositivo pasará al modo centralizado.

---

# 129. Registro de dispositivo en servidor central

El proceso de incorporación será:

```text
ESP32
 │
 ├── Device UID
 ├── Hardware
 ├── Firmware
 └── Capabilities
        │
        ↓
Servidor Central
        │
        ├── valida
        ├── registra
        └── asigna identidad lógica
```

Ejemplo:

```json
{
    "device_uid": "ESP32-A8F123",
    "device_id": "GH-001",
    "hardware": "ESP32-WROOM",
    "firmware": "7.0.0",
    "capabilities": [
        "WIFI",
        "RS485",
        "I2C",
        "SPI",
        "GPIO"
    ]
}
```

---

# 130. Servidor central

El servidor central será una aplicación independiente del firmware.

Su función será:

```text
Administración
Supervisión
Históricos
Usuarios
Permisos
Configuración
Alarmas
Firmware
OTA
Actualizaciones
Inventario
Diagnóstico
```

La arquitectura recomendada será:

```text
                    WEB
                     │
                     ↓
              FRONTEND WEB
                     │
                     ↓
                  API
                     │
       ┌─────────────┼─────────────┐
       │             │             │
 PostgreSQL        MQTT        Servicios
       │             │             │
       └─────────────┼─────────────┘
                     │
                  GATEWAYS
                     │
                  DEVICES
```

---

# 131. Componentes del servidor central

El servidor central deberá disponer como mínimo de:

```text
1. Frontend Web
2. API REST
3. WebSocket
4. MQTT Broker
5. Base de datos PostgreSQL
6. Servicio de autenticación
7. Servicio de configuración
8. Servicio de alarmas
9. Servicio de OTA
10. Servicio de actualización
11. Servicio de sincronización
12. Servicio de descubrimiento
13. Servicio de diagnóstico
14. Sistema de auditoría
15. Sistema de backups
```

---

# 132. Dashboard central

El dashboard deberá mostrar:

```text
INVERNADEROS

GH-001  ● ONLINE
GH-002  ● ONLINE
GH-003  ● OFFLINE
GH-004  ⚠ ALARMA
```

Al entrar en un invernadero:

```text
Temperatura
Humedad
VPD
CO₂
pH
EC
Nivel
Caudal
Luz
Viento
Lluvia
Riego
Ventilación
Iluminación
```

con gráficos históricos.

---

# 133. Vista de dispositivo

Cada dispositivo deberá disponer de una ficha:

```text
GH-001

Estado:
ONLINE

Device UID:
ESP32-A8F123

IP:
192.168.1.100

MAC:
XX:XX:XX:XX:XX:XX

Firmware:
7.2.1

Hardware:
V2.0

Uptime:
18 días

CPU:
XX %

RAM:
XX %

WiFi:
-52 dBm

RS485:
OK

MQTT:
CONNECTED

Servidor:
CONNECTED
```

---

# 134. Gestión de usuarios

El servidor deberá disponer de usuarios y roles.

Ejemplo:

```text
ADMIN
OPERATOR
VIEWER
MAINTENANCE
```

Permisos independientes:

```text
Ver datos
Modificar configuración
Control manual
Modificar automatización
Actualizar firmware
Administrar usuarios
Eliminar dispositivos
Modificar red
```

---

# 135. Auditoría

Todas las modificaciones importantes deberán registrarse.

Ejemplo:

```text
2026-09-28 11:32
Usuario: admin
Dispositivo: GH-001
Cambio: FAN_01
Anterior: 22 °C
Nuevo: 24 °C
```

También:

```text
Configuración modificada
Firmware actualizado
Actuador activado manualmente
Sensor agregado
Sensor eliminado
Alarma reconocida
Usuario creado
```

---

# 136. Sistema de alarmas

Las alarmas deberán existir tanto localmente como en el servidor.

Tipos:

```text
SENSOR_ERROR
SENSOR_TIMEOUT
VALUE_OUT_OF_RANGE
LOW_TANK
NO_FLOW
OVER_TEMPERATURE
HIGH_WIND
RAIN
COMMUNICATION_ERROR
RS485_ERROR
MQTT_ERROR
SERVER_ERROR
OTA_ERROR
CONFIG_ERROR
ACTUATOR_ERROR
```

Cada alarma deberá tener:

```text
ID
timestamp
device
zone
severity
status
message
source
acknowledged
resolved
```

---

# 137. Severidad de alarmas

Se deberán utilizar niveles:

```text
INFO
WARNING
ERROR
CRITICAL
```

Ejemplo:

```text
WARNING
Sensor de humedad sin respuesta
```

o:

```text
CRITICAL
Bomba activa sin caudal
```

---

# 138. API central

La API deberá estar versionada.

Ejemplo:

```text
/api/v1/devices
/api/v1/greenhouses
/api/v1/zones
/api/v1/sensors
/api/v1/actuators
/api/v1/alarms
/api/v1/events
/api/v1/configurations
/api/v1/firmware
/api/v1/users
```

Nunca se deberá romper una API existente sin incrementar su versión.

---

# 139. MQTT central

La estructura MQTT deberá evolucionar hacia una jerarquía uniforme.

Ejemplo:

```text
invernadero/GH-001/state
invernadero/GH-001/telemetry
invernadero/GH-001/sensors/#
invernadero/GH-001/actuators/#
invernadero/GH-001/events
invernadero/GH-001/alarms
invernadero/GH-001/config
invernadero/GH-001/commands
invernadero/GH-001/ota
```

Para dispositivos industriales:

```text
invernadero/GH-001/rs485/#
```

---

# 140. Shadow del dispositivo

El servidor central deberá mantener un "Device Shadow".

El Shadow representará:

```text
CONFIGURACIÓN DESEADA
CONFIGURACIÓN ACTUAL
ESTADO ACTUAL
```

Ejemplo:

```json
{
    "desired": {
        "fan_min_temp": 24
    },
    "reported": {
        "fan_min_temp": 24
    }
}
```

Si:

```text
desired != reported
```

el dispositivo deberá sincronizarse.

Esto permitirá recuperar automáticamente la configuración después de una reconexión.

---

# 141. OTA local

La página web local deberá incorporar:

```text
SISTEMA
 └── ACTUALIZACIÓN DE FIRMWARE
```

Mostrar:

```text
Versión instalada:
7.1.0

Última versión disponible:
7.2.0

Estado:
NUEVA VERSIÓN DISPONIBLE
```

y:

```text
[ VER CAMBIOS ]
[ ACTUALIZAR ]
```

---

# 142. Manifest de firmware

El proyecto deberá mantener un archivo JSON en GitHub.

Ejemplo:

```json
{
    "project": "Invernadero",
    "channel": "stable",
    "version": "7.2.0",
    "release_date": "2026-09-28",
    "firmware_url": "https://...",
    "sha256": "...",
    "release_notes_url": "https://...",
    "min_bootloader": "1.0.0",
    "min_hardware": "1.0"
}
```

El dispositivo consultará periódicamente el manifest.

---

# 143. Verificación de actualización

El proceso deberá ser:

```text
ESP32
  │
  ├── consulta manifest
  │
  ├── compara versión
  │
  ├── detecta actualización
  │
  ↓
WEB LOCAL
  │
  └── "Nueva versión disponible"
```

La actualización no deberá ejecutarse automáticamente salvo que el usuario lo habilite expresamente.

---

# 144. OTA desde servidor central

El servidor central deberá utilizar el mismo sistema de manifest.

La administración podrá mostrar:

```text
Firmware

Actual:
7.1.0

Disponible:
7.2.0

Dispositivos afectados:
12

Compatibilidad:
10 compatibles
2 requieren actualización de hardware
```

El administrador podrá seleccionar:

```text
☑ GH-001
☑ GH-002
☐ GH-003
```

y realizar:

```text
ACTUALIZAR SELECCIONADOS
```

---

# 145. Actualización por grupos

Para instalaciones grandes no se deberá actualizar todo simultáneamente.

Se deberá permitir:

```text
GRUPO PILOTO
GRUPO 1
GRUPO 2
GRUPO 3
```

Proceso:

```text
1. Actualizar un dispositivo
2. Verificar
3. Esperar período de observación
4. Actualizar siguiente grupo
5. Continuar
```

Esto reduce el riesgo de una actualización defectuosa generalizada.

---

# 146. Rollback OTA

El dispositivo deberá utilizar particiones OTA y rollback cuando el hardware/partición lo permita.

Proceso:

```text
Firmware actual
      ↓
Descarga
      ↓
Verificación SHA-256
      ↓
Instalación
      ↓
Reinicio
      ↓
Boot de prueba
      ↓
Health Check
      │
 ┌────┴────┐
 OK       ERROR
 │          │
 ↓          ↓
CONFIRMAR  ROLLBACK
```

Nunca deberá considerarse exitosa una actualización solamente porque el archivo se descargó correctamente.

---

# 147. Compatibilidad de firmware

Cada firmware deberá declarar:

```text
firmware_version
hardware_version
bootloader_version
config_schema_version
protocol_version
```

Ejemplo:

```json
{
    "firmware": "7.2.0",
    "hardware": "2.0",
    "config_schema": 14,
    "protocol": 3
}
```

Esto permitirá evitar cargar firmware incompatible.

---

# 148. Canales de actualización

El manifest podrá soportar:

```text
stable
beta
development
```

El usuario podrá elegir desde la configuración:

```text
Canal:
[ STABLE ]
```

Por defecto:

```text
STABLE
```

---

# 149. Diagnóstico remoto

El servidor deberá poder consultar:

```text
CPU
RAM
Flash
Uptime
Temperatura interna
WiFi RSSI
Ethernet
MQTT
RS485
Sensores
Actuadores
Watchdog
Reinicios
Errores
Logs
```

Esto permitirá diagnosticar un dispositivo sin estar físicamente presente.

---

# 150. Registro de reinicios

El ESP32 deberá registrar la causa de cada reinicio.

Ejemplo:

```text
POWER_ON
SOFTWARE_RESET
WATCHDOG
BROWNOUT
PANIC
OTA
FACTORY_RESET
UNKNOWN
```

El servidor podrá mostrar:

```text
Últimos reinicios

28/09 08:31 → POWER_ON
27/09 22:14 → WATCHDOG
26/09 19:42 → OTA
```

---

# 151. Registro de eventos local

El dispositivo deberá disponer de un Event Log.

Ejemplo:

```text
11:20 SENSOR pH ONLINE
11:21 BOMBA_01 ON
11:22 CAUDAL OK
11:25 BOMBA_01 OFF
11:30 MQTT DISCONNECTED
11:31 MQTT RECONNECTED
```

---

# 152. Seguridad de comunicaciones

Las comunicaciones con el servidor deberán utilizar preferentemente:

```text
HTTPS
MQTTS
```

cuando la infraestructura lo permita.

El dispositivo deberá autenticarse mediante credenciales o identidad propia.

No se deberán utilizar credenciales globales idénticas para todos los dispositivos.

Cada dispositivo deberá tener identidad propia.

---

# 153. Certificados

Para instalaciones de mayor tamaño se deberá contemplar:

```text
TLS
CA
Certificado de dispositivo
Clave privada
```

Esto permitirá implementar posteriormente autenticación mutua:

```text
ESP32 ⇄ Servidor
```

mediante certificados.

---

# 154. Seguridad de configuración local

La página local deberá disponer de autenticación.

Se deberán separar:

```text
VIEW
OPERATOR
ADMIN
MAINTENANCE
```

El acceso a:

```text
WiFi
Ethernet
MQTT
Servidor
OTA
Factory Reset
```

deberá requerir permisos elevados.

---

# 155. Configuración de fábrica

El sistema deberá conservar un mecanismo de recuperación.

Se podrá utilizar:

```text
Botón físico
+
Web
+
API
```

El reset podrá tener diferentes niveles:

```text
RESET NETWORK
RESET CONFIGURATION
RESET AUTOMATION
FACTORY RESET
```

No deberá ser necesario borrar todo el sistema para solucionar un problema de WiFi.

---

# 156. Exportación e importación de configuración

La web local y el servidor central deberán permitir:

```text
EXPORTAR CONFIGURACIÓN
```

generando:

```text
config.json
```

y:

```text
IMPORTAR CONFIGURACIÓN
```

Antes de aplicar una configuración se deberá:

```text
validar
crear backup
aplicar
probar
confirmar
```

---

# 157. Backup automático

El servidor central deberá realizar backups de:

```text
Base de datos
Configuraciones
Usuarios
Históricos
Firmware metadata
Perfiles Modbus
Reglas
Escenarios
```

Los backups deberán poder restaurarse.

---

# 158. Base de datos central ampliada

Además de las tablas actuales, se recomienda incorporar:

```text
users
roles
permissions

greenhouses
zones
devices
device_capabilities
device_credentials

sensors
sensor_types
sensor_profiles
sensor_readings

actuators
actuator_types
actuator_states

modbus_devices
modbus_profiles
modbus_registers

automations
automation_rules
schedules

alarms
events
audit_logs

configurations
configuration_versions

firmware_versions
firmware_releases
ota_jobs
ota_results

network_interfaces
gateways

notifications
```

---

# 159. Arquitectura de datos

La base de datos deberá separar:

```text
DEFINICIÓN
```

de:

```text
TELEMETRÍA
```

Por ejemplo:

```text
sensors
```

define qué sensor existe.

Mientras:

```text
sensor_readings
```

contiene las mediciones.

Esto permitirá administrar millones de mediciones sin mezclar configuración con telemetría.

---

# 160. Retención de históricos

El servidor deberá permitir configurar:

```text
Retención:
7 días
30 días
90 días
1 año
Personalizado
```

También se podrán generar datos agregados:

```text
1 segundo
1 minuto
5 minutos
1 hora
1 día
```

para reducir almacenamiento.

---

# 161. Visualización de históricos

Los gráficos deberán permitir:

```text
Última hora
Últimas 6 horas
24 horas
7 días
30 días
Personalizado
```

y superponer:

```text
Temperatura
Humedad
VPD
CO₂
pH
EC
Riego
Iluminación
```

Esto permitirá analizar la relación entre condiciones ambientales y acciones realizadas.

---

# 162. Integración pH + EC + automatización

La incorporación de pH y EC permitirá crear automatizaciones de fertirriego.

Ejemplo:

```text
pH
EC
Caudal
Nivel
Temperatura
```

podrán formar parte de una única estrategia de control.

Ejemplo conceptual:

```text
SI
    EC < objetivo
Y
    tanque disponible
Y
    caudal correcto
ENTONCES
    ejecutar dosificación
```

Todas estas funciones deberán poder deshabilitarse si la instalación no dispone de los sensores correspondientes.

---

# 163. Sensores industriales adicionales

La arquitectura deberá contemplar sensores industriales de:

### Calidad del agua

```text
pH
EC
ORP
Temperatura
Turbidez
Oxígeno disuelto
```

### Clima

```text
Temperatura
Humedad
Presión
CO₂
Radiación
PAR
Viento
Lluvia
```

### Hidráulica

```text
Caudal
Presión
Nivel
```

### Suelo

```text
Temperatura
Humedad
Conductividad
```

### Actuación industrial

```text
Relés Modbus
I/O digitales
I/O analógicas
Variadores
Controladores
```

---

# 164. Módulos Modbus propios

Una ventaja importante del proyecto será poder crear módulos propios.

Ejemplo:

```text
ESP32
 +
RS485
 +
8 entradas digitales
```

o:

```text
ESP32
 +
RS485
 +
8 salidas digitales
```

o:

```text
ESP32
 +
RS485
 +
4 entradas analógicas
```

Estos módulos podrán funcionar como dispositivos Modbus RTU estándar.

---

# 165. Módulo universal I/O

Se podrá desarrollar posteriormente un módulo:

```text
INVERNADERO I/O MODULE
```

con:

```text
8 DI
8 DO
4 AI
2 AO
RS485
```

El servidor podrá detectarlo y mostrar automáticamente sus recursos.

---

# 166. Sistema de capacidades

Cada dispositivo deberá anunciar sus capacidades.

Ejemplo:

```json
{
    "capabilities": [
        "TEMP",
        "HUMIDITY",
        "RS485",
        "RELAY_8",
        "PWM_4"
    ]
}
```

El servidor podrá construir automáticamente la interfaz según estas capacidades.

Esto permitirá evitar interfaces rígidas.

---

# 167. Interfaz dinámica

Si un dispositivo tiene:

```text
4 sensores
```

se mostrarán cuatro sensores.

Si otro tiene:

```text
32 sensores
```

la interfaz se adaptará.

Lo mismo para:

```text
salidas
zonas
RS485
Modbus
alarmas
```

---

# 168. Descubrimiento de dispositivos IP

Además del descubrimiento RS485, el servidor podrá detectar dispositivos mediante:

```text
mDNS
MQTT
registro manual
provisioning
```

Ejemplo:

```text
Nuevo dispositivo detectado

GH-009
IP: 192.168.1.109
UID: ESP32-XXXX
Firmware: 7.1.0

[ REGISTRAR ]
```

---

# 169. Provisioning

Se deberá crear un proceso de incorporación seguro.

```text
DISPOSITIVO NUEVO
       ↓
PROVISIONING
       ↓
IDENTIDAD
       ↓
CREDENCIALES
       ↓
SERVIDOR
       ↓
CONFIGURACIÓN
       ↓
OPERACIÓN
```

Esto será especialmente importante cuando existan decenas o cientos de dispositivos.

---

# 170. Gateway multi-bus

Una futura placa gateway podrá incorporar:

```text
Ethernet
WiFi
RS485
CAN
I²C
SPI
GPIO
```

Su función será conectar diferentes tecnologías.

Ejemplo:

```text
                     Ethernet
                        │
                        ↓
                  ESP32 GATEWAY
                        │
       ┌────────────────┼────────────────┐
       │                │                │
     RS485             CAN              WiFi
       │                │                │
   sensores          módulos          sensores
```

Esto permitirá evolucionar posteriormente el proyecto hacia instalaciones mucho más grandes.

---

# 171. Arquitectura de red para instalaciones grandes

Una instalación grande podrá utilizar:

```text
                    SERVIDOR
                       │
                  Ethernet
                       │
                 CORE GATEWAY
                       │
          ┌────────────┼────────────┐
          │            │            │
       Gateway A    Gateway B    Gateway C
          │            │            │
        RS485        RS485        RS485
          │            │            │
       ZONA 1       ZONA 2       ZONA 3
```

Cada gateway podrá controlar una zona física.

Esto reduce:

* longitud de cables;
* cantidad de sensores conectados directamente;
* carga de un único ESP32;
* complejidad del cableado.

---

# 172. Funcionamiento ante pérdida del servidor

Debe existir una política explícita:

```text
Servidor perdido
      ↓
NO detener automatización
      ↓
Continuar control local
      ↓
Guardar eventos
      ↓
Guardar históricos
      ↓
Intentar reconexión
      ↓
Sincronizar al recuperar conexión
```

La automatización crítica nunca deberá depender de una consulta al servidor para cada decisión.

---

# 173. Funcionamiento ante pérdida de red

Si se pierde:

```text
WiFi
```

pero existe:

```text
Ethernet
```

el sistema deberá intentar Ethernet.

Si se pierde:

```text
Ethernet
```

pero existe:

```text
WiFi
```

podrá intentar WiFi.

Si ambas fallan:

```text
CONTROL LOCAL
```

deberá continuar funcionando.

---

# 174. Arquitectura de prioridad de control

La prioridad recomendada será:

```text
1. SEGURIDAD
2. PROTECCIÓN HARDWARE
3. MANUAL LOCAL DE EMERGENCIA
4. AUTOMATIZACIÓN LOCAL
5. PROGRAMACIONES
6. COMANDOS DEL SERVIDOR
7. COMANDOS REMOTOS NO CRÍTICOS
```

Un comando remoto nunca deberá poder saltarse un interlock de seguridad.

---

# 175. Estado de cada dispositivo

Todos los dispositivos deberán utilizar una máquina de estados.

Ejemplo:

```text
BOOT
 ↓
INIT
 ↓
SELF_TEST
 ↓
NETWORK
 ↓
SYNC
 ↓
RUN
 ↓
DEGRADED
 ↓
RECOVERY
```

Estados posibles:

```text
BOOTING
INITIALIZING
ONLINE
OFFLINE
DEGRADED
ERROR
MAINTENANCE
UPDATING
RECOVERY
```

---

# 176. Estado de calidad de datos

Cada sensor deberá proporcionar:

```text
value
unit
timestamp
quality
```

Ejemplo:

```json
{
    "sensor": "PH-01",
    "value": 6.32,
    "unit": "pH",
    "quality": "GOOD"
}
```

Esto evitará que un valor inválido sea interpretado como una medición real.

---

# 177. Diagnóstico RS485

La página local deberá incluir:

```text
RS485 DIAGNOSTICS
```

mostrando:

```text
Baudrate
Parity
Stop bits
TX
RX
CRC errors
Timeouts
Retries
Devices found
Last response
```

Esto será especialmente útil durante la instalación.

---

# 178. Herramienta Modbus

La interfaz de mantenimiento podrá incorporar un lector Modbus.

Ejemplo:

```text
Slave ID:
10

Function:
03

Register:
40001

Quantity:
2

[ READ ]
```

Resultado:

```text
40001 = 632
40002 = 245
```

Esta herramienta deberá estar restringida a usuarios con permisos de mantenimiento.

---

# 179. Perfiles de hardware

Cada placa deberá declarar:

```text
HARDWARE PROFILE
```

Ejemplo:

```text
ESP32-GH-V1
ESP32-GH-V2
ESP32-GATEWAY-V1
ESP32-IO-V1
```

Cada perfil indicará:

```text
GPIO
ADC
SPI
I2C
RS485
Ethernet
PWM
74HC595
```

Esto permitirá mantener un firmware común con diferentes variantes de hardware.

---

# 180. Arquitectura de firmware recomendada

La estructura deberá evolucionar hacia:

```text
src/
│
├── core/
│   ├── device
│   ├── state
│   ├── scheduler
│   └── capabilities
│
├── config/
│   ├── config_manager
│   ├── schema
│   └── validation
│
├── network/
│   ├── wifi
│   ├── ethernet
│   ├── mdns
│   ├── dns
│   └── ntp
│
├── protocols/
│   ├── mqtt
│   ├── rest
│   ├── websocket
│   ├── modbus
│   └── rs485
│
├── sensors/
│   ├── analog
│   ├── i2c
│   ├── onewire
│   └── industrial
│
├── actuators/
│   ├── digital
│   ├── pwm
│   ├── relay
│   ├── mosfet
│   └── motor
│
├── automation/
│   ├── climate
│   ├── irrigation
│   ├── lighting
│   ├── fertigation
│   └── safety
│
├── web/
│   ├── dashboard
│   ├── configuration
│   ├── diagnostics
│   └── ota
│
├── storage/
│   ├── nvs
│   ├── history
│   └── events
│
├── ota/
│   ├── manifest
│   ├── updater
│   └── rollback
│
└── diagnostics/
    ├── logs
    ├── watchdog
    └── health
```

---

# 181. API del dispositivo

La API local deberá evolucionar hacia:

```text
/api/v1/device
/api/v1/status
/api/v1/capabilities

/api/v1/sensors
/api/v1/sensors/{id}

/api/v1/actuators
/api/v1/actuators/{id}

/api/v1/zones
/api/v1/automation

/api/v1/config
/api/v1/config/schema

/api/v1/network
/api/v1/rs485
/api/v1/modbus

/api/v1/events
/api/v1/alarms

/api/v1/diagnostics

/api/v1/firmware
/api/v1/ota
```

---

# 182. WebSocket

El WebSocket deberá transmitir eventos en tiempo real.

Ejemplo:

```json
{
    "type": "sensor_update",
    "sensor": "TEMP-01",
    "value": 24.7,
    "unit": "C"
}
```

También:

```json
{
    "type": "alarm",
    "severity": "WARNING",
    "message": "Sin caudal"
}
```

y:

```json
{
    "type": "device_state",
    "state": "ONLINE"
}
```

---

# 183. Telemetría

La telemetría deberá utilizar un modelo común.

```text
timestamp
device_id
resource_id
value
unit
quality
```

Esto permitirá utilizar exactamente el mismo modelo para:

```text
sensor local
sensor Modbus
sensor remoto
sensor industrial
```

---

# 184. Sistema de plugins de sensores

En una etapa avanzada, los sensores deberán funcionar mediante drivers.

Ejemplo:

```text
SensorDriver
    │
    ├── SHT31
    ├── AHT20
    ├── DS18B20
    ├── BH1750
    ├── SEN0161
    ├── SEN024
    ├── ModbusPH
    ├── ModbusEC
    └── ModbusGeneric
```

El sistema podrá detectar qué drivers están disponibles y qué recursos están habilitados.

---

# 185. Sistema de plugins de actuadores

De la misma forma:

```text
ActuatorDriver
    │
    ├── Digital
    ├── PWM
    ├── Relay
    ├── MOSFET
    ├── SSR
    ├── Motor
    ├── ModbusRelay
    └── ModbusIO
```

---

# 186. Expansión de salidas

La arquitectura de expansión deberá utilizar el sistema definido anteriormente:

```text
ESP32
 │
 └── SPI
      │
      ├── 74HC595
      ├── 74HC595
      ├── 74HC595
      └── 74HC595
```

Cada registro proporciona ocho salidas lógicas.

Las salidas deberán conectarse posteriormente a:

```text
MOSFET
Relay
SSR
Driver
Contactor
H-Bridge
```

El 74HC595/74HCT595 no deberá utilizarse directamente para alimentar cargas de potencia.

---

# 187. PWM expandido

El sistema podrá utilizar el 74HC595 como expansión lógica para canales PWM cuando el diseño de firmware utilice transferencia SPI, temporización y buffers adecuados.

Sin embargo, deberá diferenciarse:

```text
74HC595
=
expansor de salidas lógicas
```

de:

```text
ESP32 hardware PWM
=
generación PWM real
```

Para canales que requieran alta frecuencia, alta resolución o sincronización estricta se utilizará el PWM hardware del ESP32 o un controlador PWM dedicado.

---

# 188. Seguridad eléctrica

La arquitectura deberá separar:

```text
3.3 V lógica
5 V lógica
12 V
24 V
230 V AC
```

Las cargas de potencia deberán utilizar drivers apropiados.

Para cargas inductivas:

```text
MOSFET / Relay
+
protección flyback
```

Para cargas de red:

```text
aislamiento
+
SSR/contactor
+
protecciones
+
fusible
```

El diseño de PCB deberá mantener separación física entre lógica y potencia.

---

# 189. Watchdog y recuperación

Todos los dispositivos deberán incorporar:

```text
Watchdog
```

y supervisión de:

```text
WiFi
Ethernet
MQTT
RS485
sensores
automatización
memoria
```

Un fallo de comunicación no deberá bloquear el loop principal.

---

# 190. Pruebas automáticas

El proyecto deberá incorporar pruebas para:

```text
Configuración
JSON
Sensores
Modbus
RS485
Automatización
Alarmas
OTA
Rollback
Networking
MQTT
API
```

Se deberán crear simuladores para sensores y dispositivos Modbus cuando sea posible.

---

# 191. Simulador de dispositivos

Se podrá crear un modo:

```text
SIMULATION_MODE
```

que permita generar:

```text
Temperatura
Humedad
pH
EC
Nivel
Caudal
CO₂
Viento
Lluvia
```

sin hardware físico.

Esto permitirá probar la plataforma central antes de conectar sensores reales.

---

# 192. Modo mantenimiento

Cada dispositivo deberá tener:

```text
NORMAL
MAINTENANCE
```

En mantenimiento se podrán realizar:

```text
Test GPIO
Test relés
Test PWM
Test RS485
Scan Modbus
Test sensores
Test red
Test MQTT
OTA
Exportación
Diagnóstico
```

Las funciones peligrosas deberán requerir confirmación.

---

# 193. Control manual seguro

El control manual deberá tener:

```text
ON
OFF
AUTO
```

y opcionalmente:

```text
DURATION
```

Ejemplo:

```text
BOMBA 1

[ ON 30 segundos ]
```

Al finalizar:

```text
AUTO
```

o:

```text
OFF
```

según la configuración.

---

# 194. Interfaz de instalación

Se deberá crear un asistente:

```text
CONFIGURACIÓN INICIAL
```

Paso 1:

```text
Identificación
```

Paso 2:

```text
Red
```

Paso 3:

```text
Sensores
```

Paso 4:

```text
RS485
```

Paso 5:

```text
Actuadores
```

Paso 6:

```text
Zonas
```

Paso 7:

```text
Automatización
```

Paso 8:

```text
Servidor central
```

Paso 9:

```text
Prueba del sistema
```

Paso 10:

```text
Finalizar
```

---

# 195. Plantillas de instalación

El servidor podrá ofrecer plantillas:

```text
INVERNADERO INTERIOR
INVERNADERO EXTERIOR
HIDROPONÍA
FERTIRRIEGO
VIVER0
INVERNADERO MULTIZONA
```

La plantilla configurará inicialmente:

```text
sensores
actuadores
reglas
zonas
alarmas
dashboard
```

El usuario podrá modificar posteriormente cualquier parámetro.

---

# 196. Compatibilidad futura

La arquitectura deberá diseñarse evitando dependencias innecesarias de un único modelo de ESP32.

El objetivo será permitir:

```text
ESP32
ESP32-S3
ESP32 Ethernet
ESP32 Gateway
```

y futuras variantes siempre que proporcionen las interfaces necesarias.

---

# 197. Escalabilidad final

El sistema deberá poder crecer progresivamente:

```text
Nivel 1

ESP32
+
SHT31
+
DS18B20
+
suelo
+
bomba
```

hasta:

```text
Nivel 2

ESP32
+
74HC595
+
RS485
+
pH
+
EC
+
CO₂
+
varias zonas
```

hasta:

```text
Nivel 3

Múltiples ESP32
+
Ethernet
+
WiFi
+
RS485
+
Gateways
+
Servidor central
+
PostgreSQL
+
MQTT
+
OTA
```

y finalmente:

```text
Nivel 4

Múltiples invernaderos
+
múltiples gateways
+
múltiples buses industriales
+
usuarios
+
roles
+
históricos
+
alarmas
+
automatización distribuida
+
gestión centralizada de firmware
```

---

# 198. Arquitectura final propuesta

La arquitectura completa del proyecto quedará:

```text
                              INTERNET / LAN
                                    │
                             ┌──────▼──────┐
                             │   SERVIDOR  │
                             │   CENTRAL   │
                             └──────┬──────┘
                                    │
                   ┌────────────────┼────────────────┐
                   │                │                │
                  API              MQTT             OTA
                   │                │                │
                   └────────────────┼────────────────┘
                                    │
                              RED IP LOCAL
                                    │
                 ┌──────────────────┼──────────────────┐
                 │                  │                  │
            GATEWAY 01         GATEWAY 02         ESP32 DIRECTO
            Ethernet/WiFi      Ethernet/WiFi
                 │                  │
               RS485              RS485
                 │                  │
       ┌─────────┼─────────┐      ├─────────┐
       │         │         │      │         │
      pH        EC        ORP    CO₂       TEMP
       │         │         │      │         │
       └─────────┴─────────┴──────┴─────────┘

                       NODOS DE CAMPO
                              │
             ┌────────────────┼────────────────┐
             │                │                │
           I²C              SPI              ADC
             │                │                │
         sensores         74HC595         ADS1115
             │                │                │
             └────────────────┼────────────────┘
                              │
                         ESP32 FIELD
                              │
                   ┌──────────┴──────────┐
                   │                     │
                SENSORES              ACTUADORES
                                      │
                            ┌─────────┼─────────┐
                            │         │         │
                          Relay     MOSFET     SSR
                            │         │         │
                          Bombas   Ventil.   Iluminación
```

---

# 199. Principio fundamental de la arquitectura final

El sistema deberá cumplir siempre las siguientes reglas:

```text
1. El dispositivo debe poder funcionar sin servidor.

2. El servidor nunca debe ser necesario para mantener
   una función automática crítica.

3. Toda configuración importante debe almacenarse localmente.

4. El servidor debe mantener una copia de la configuración.

5. Las configuraciones deberán tener versión.

6. Las actualizaciones deberán poder revertirse.

7. Los sensores deberán poder agregarse sin modificar
   la lógica principal del sistema.

8. RS485 deberá ser un bus industrial general,
   no únicamente un bus para pH.

9. Modbus deberá utilizar perfiles configurables.

10. Los dispositivos deberán tener una identidad permanente
    independiente de su dirección Modbus.

11. Ethernet y WiFi deberán ser interfaces intercambiables
    dentro de la arquitectura de red.

12. La página local deberá permitir recuperar y mantener
    el dispositivo aun cuando el servidor central no esté disponible.

13. El servidor central deberá administrar múltiples
    dispositivos e instalaciones.

14. La automatización deberá ejecutarse localmente.

15. Las comunicaciones deberán considerarse una capa
    independiente de la lógica de control.
```

---

# 200. Conclusión y visión final del proyecto

El proyecto evolucionará desde un sistema de automatización de un único invernadero hacia una plataforma distribuida de automatización agrícola.

El ESP32 continuará siendo el núcleo de control de campo, pero dejará de estar limitado a sensores conectados directamente a sus GPIO.

La plataforma podrá integrar:

```text
GPIO
I²C
SPI
1-Wire
ADC
PWM
RS485
Modbus RTU
WiFi
Ethernet
MQTT
REST
WebSocket
```

Esto permitirá combinar sensores económicos y fáciles de conseguir con instrumentación industrial.

Un sistema pequeño podrá utilizar:

```text
ESP32
+
SHT31
+
DS18B20
+
humedad de suelo
+
bomba
+
ventilador
```

Mientras que un sistema avanzado podrá utilizar:

```text
ESP32
+
Ethernet
+
RS485
+
pH industrial
+
EC industrial
+
ORP
+
CO₂
+
caudal
+
presión
+
nivel
+
estación meteorológica
+
múltiples zonas
+
múltiples actuadores
+
gateway
+
servidor central
```

sin modificar el concepto fundamental de la plataforma.

La página web local será el elemento de configuración y mantenimiento de cada dispositivo cuando este funcione de manera independiente.

Cuando el dispositivo esté configurado como administrado por un servidor central, el servidor pasará a ser la interfaz principal de administración, manteniendo siempre una copia local de la configuración necesaria para garantizar la autonomía.

La incorporación de gateways Ethernet/WiFi + RS485 permitirá desplegar instalaciones grandes donde una única placa no tenga que concentrar todos los sensores.

La incorporación de perfiles Modbus permitirá integrar instrumentación industrial de diferentes fabricantes.

La incorporación de un sistema de identidad permanente permitirá realizar descubrimiento, provisioning y administración de dispositivos sin depender exclusivamente de las direcciones Modbus.

La incorporación de un manifest de firmware alojado en GitHub permitirá implementar un sistema de actualización OTA controlado, con detección de nuevas versiones, verificación de integridad, compatibilidad de hardware, registro de actualización y rollback.

Finalmente, el servidor central permitirá transformar el proyecto en una plataforma capaz de administrar:

```text
                 ┌──────────────────────┐
                 │  SERVIDOR CENTRAL    │
                 └──────────┬───────────┘
                            │
          ┌─────────────────┼─────────────────┐
          │                 │                 │
      INVERNADERO 1     INVERNADERO 2     INVERNADERO 3
          │                 │                 │
       GATEWAYS          GATEWAYS          GATEWAYS
          │                 │                 │
       NODOS              NODOS              NODOS
          │                 │                 │
       SENSORES          SENSORES          SENSORES
       ACTUADORES        ACTUADORES        ACTUADORES
```

La meta final no será simplemente construir un firmware para un invernadero.

La meta será desarrollar una **plataforma modular, distribuida, escalable y configurable de automatización de invernaderos**, capaz de comenzar con una instalación pequeña y crecer progresivamente hasta sistemas multizona y multinvernadero con instrumentación industrial, redes Ethernet/WiFi, buses RS485/Modbus y administración centralizada.

El principio fundamental seguirá siendo:

```text
                     SERVIDOR CENTRAL
                           │
                 SUPERVISIÓN / GESTIÓN
                           │
                    RED / MQTT / API
                           │
                    GATEWAY / NODO
                           │
                 ┌─────────┴─────────┐
                 │                   │
              SENSORES           ACTUADORES
                 │                   │
                 └──────── ESP32 ────┘
                           │
                    AUTOMATIZACIÓN
                           │
                       AUTÓNOMA
```

**El servidor administra.
El gateway comunica.
El ESP32 controla.
Los sensores informan.
Los actuadores ejecutan.
La automatización continúa funcionando aunque la red desaparezca.**
