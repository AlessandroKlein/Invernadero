# Sistema de Invernadero ESP32 - Monitoreo y Control Automatizado

## Comandos de desarrollo
```bash
pio run                 # Compilar proyecto
pio run --target upload # Subir firmware
pio device monitor      # Monitor serie
```

## Descripción

Sistema completo de monitoreo y control automatizado para invernaderos basado en ESP32. Permite supervisar parámetros ambientales críticos y controlar dispositivos de forma automática o manual a través de una interfaz web moderna y intuitiva.

## Características Principales

### 🌱 Monitoreo Ambiental
- **Temperatura y Humedad**: Sensor AHT20 de alta precisión
- **Presión Atmosférica**: Sensor BMP280 con compensación de temperatura
- **Temperatura del Suelo**: Sensor DS18B20 resistente al agua
- **Humedad del Suelo**: 4 sensores analógicos independientes (pines 36, 39, 32, 33)
- **Luz Ambiental**: Sensor LDR con calibración personalizable
- **Calidad del Aire**: Sensor MQ-7 para detección de monóxido de carbono
- **Cálculos Avanzados**: Índice de calor, punto de rocío, déficit de presión de vapor (VPD)

### 🔌 Control de Dispositivos
- **6 Salidas Digitales Configurables**: Pines 2, 4, 16, 17, 5, 18
- **Tipos de Dispositivos Soportados**:
  - Electroválvulas para riego
  - Sistemas de iluminación LED
  - Ventiladores de circulación
  - Bombas de agua
  - Calefactores
  - Sistemas de enfriamiento
- **Control Automático Inteligente**:
  - **Riego Automático**: Basado en sensores de humedad del suelo con umbrales configurables
  - **Control de Temperatura**: Activación automática de ventiladores, calefactores y enfriadores
  - **Programación de Iluminación**: Horarios de encendido/apagado para luces de crecimiento
  - **Configuración Persistente**: Guardado automático en memoria flash del ESP32
  - **APIs REST**: Endpoints dedicados para configuración automática (/api/outputs/auto-config)
  - **Umbrales Personalizables**: Configuración individual por dispositivo y tipo de control

### 🌐 Interfaz Web Completa
- **Dashboard Principal**: Visualización en tiempo real con tooltips informativos
- **Configuración de Sensores**: Habilitación/deshabilitación individual
- **Configuración de Salidas**: Control manual y automático de dispositivos
- **Calibración Avanzada**: Sistema de calibración individual por sensor
- **Gestión de Red**: Configuración WiFi con IP estática opcional
- **Cliente MQTT**: Publicación automática de datos
- **Sincronización de Tiempo**: NTP con configuración de zona horaria
- **Gestión de Configuración**: Exportación/importación JSON
- **Actualizaciones OTA**: Firmware remoto sin cables
- **Reset de Fábrica**: Restauración completa del sistema

### 📡 Conectividad
- **WiFi 802.11 b/g/n**: Conexión inalámbrica principal
- **mDNS**: Acceso mediante `invernadero-esp32.local`
- **WebSocket**: Actualizaciones en tiempo real
- **API REST**: Endpoints completos para integración
- **MQTT**: Publicación automática de telemetría

## Hardware Requerido

### Microcontrolador
- **ESP32 DevKit v1** (recomendado)
- **Voltaje**: 3.3V/5V
- **Flash**: Mínimo 4MB
- **RAM**: 520KB

### Sensores Implementados
- **AHT20**: Temperatura y humedad ambiente (I2C)
- **BMP280**: Presión atmosférica (I2C)
- **DS18B20**: Temperatura del suelo (OneWire, pin 15)
- **MQ-7**: Monóxido de carbono (analógico, pin 35)
- **LDR**: Luz ambiental (analógico, pin 34)
- **4x Sensores de Humedad del Suelo**: Analógicos (pines 36, 39, 32, 33)

### Salidas de Control
- **6 Relés o MOSFETs**: Para control de dispositivos (pines 2, 4, 16, 17, 5, 18)
- **Fuente de Alimentación**: 5V/3A recomendada
- **Protección**: Fusibles y diodos de protección

## Estructura del Proyecto

```
Invernadero/
├── src/
│   ├── main.cpp              # Programa principal y loop
│   ├── sensors.cpp           # Gestión de sensores y salidas
│   ├── automatic_control.cpp # Sistema de control automático
│   ├── web_server.cpp        # Servidor web y APIs REST
│   ├── web_pages.cpp         # Páginas HTML y CSS
│   ├── config.cpp            # Sistema de configuración
│   ├── wifi_manager.cpp      # Gestión WiFi
│   ├── mqtt_manager.cpp      # Cliente MQTT
│   ├── time_manager.cpp      # Sincronización NTP
│   ├── ota_manager.cpp       # Actualizaciones OTA
│   └── sd_manager.cpp        # Gestión tarjeta SD (opcional)
├── include/
│   ├── automatic_control.hpp # Cabecera del control automático
│   └── *.hpp                 # Otros archivos de cabecera
├── platformio.ini            # Configuración del proyecto
└── README.md                 # Este archivo
```

## Instalación y Configuración

### 1. Preparación del Hardware
1. Conectar sensores según el pinout especificado
2. Instalar relés o MOSFETs para las salidas
3. Verificar alimentación y conexiones
4. Conectar dispositivos de control (bombas, ventiladores, etc.)

### 2. Compilación y Carga
```bash
# Clonar el repositorio
git clone <repository-url>
cd Invernadero

# Compilar
pio run

# Subir firmware
pio run --target upload

# Monitor serie (opcional)
pio device monitor
```

### 3. Configuración Inicial
1. **Primera conexión**: El ESP32 creará un punto de acceso WiFi
2. **Configurar WiFi**: Acceder a la interfaz web y configurar red
3. **Acceso web**: Usar IP asignada o `http://invernadero-esp32.local`
4. **Configurar sensores**: Habilitar/deshabilitar según hardware instalado
5. **Calibrar sensores**: Ajustar lecturas para mayor precisión
6. **Configurar salidas**: Asignar funciones y parámetros automáticos
7. **MQTT (opcional)**: Configurar broker para telemetría

## Uso del Sistema

### Dashboard Principal
- **Lecturas en Tiempo Real**: Todos los sensores con tooltips explicativos
- **Estado de Salidas**: Indicadores visuales de dispositivos activos
- **Alertas**: Notificaciones de condiciones críticas
- **Gráficos**: Tendencias de parámetros importantes

### Control Automático
- **Riego Inteligente**: Activación basada en humedad del suelo
- **Control Climático**: Calefacción/enfriamiento según temperatura
- **Iluminación Programada**: Encendido/apagado por horarios
- **Ventilación**: Control automático de ventiladores

### APIs REST Disponibles
```
GET  /api/sensors/data        # Lecturas actuales
GET  /api/sensors/config      # Configuración de sensores
POST /api/sensors/config      # Actualizar configuración
GET  /api/outputs/status      # Estado de salidas
POST /api/outputs/control     # Control manual de salidas
GET  /api/outputs/auto-config # Configuración de control automático
POST /api/outputs/auto-config # Guardar configuración automática
GET  /api/config/json         # Exportar configuración
POST /api/config/json         # Importar configuración
POST /config/reset            # Reset de fábrica
```

### Tópicos MQTT
El sistema publica automáticamente en:
```
invernadero/sensores/temperatura
invernadero/sensores/humedad
invernadero/sensores/presion
invernadero/sensores/suelo/1-4
invernadero/sensores/luz
invernadero/sensores/co
invernadero/salidas/estado
invernadero/sistema/estado
```

## Calibración de Sensores

### Sensores de Humedad del Suelo
1. Medir valor en aire seco (valor máximo)
2. Medir valor en agua (valor mínimo)
3. Aplicar calibración desde interfaz web
4. Verificar lecturas en diferentes condiciones

### Sensor de Luz (LDR)
1. Usar luxómetro de referencia
2. Ajustar factor de calibración
3. Verificar en diferentes condiciones de luz

### Sensor MQ-7 (CO)
1. Precalentamiento de 24-48 horas
2. Calibración en aire limpio
3. Verificación con gas de referencia (opcional)

## Especificaciones Técnicas

- **Microcontrolador**: ESP32 @ 240MHz
- **Memoria Flash**: 4MB
- **RAM**: 520KB
- **WiFi**: 802.11 b/g/n (2.4GHz)
- **Interfaces**: I2C, SPI, UART, ADC
- **Resolución ADC**: 12 bits (0-4095)
- **Sensores Soportados**: 10+ tipos diferentes
- **Salidas Digitales**: 6 configurables
- **Consumo**: ~150-200mA (todos los sensores activos)
- **Temperatura Operación**: -10°C a +60°C
- **Humedad Operación**: 0-95% RH (sin condensación)

## Librerías Utilizadas

- **ESPAsyncWebServer**: Servidor web asíncrono
- **ArduinoJson**: Procesamiento JSON
- **PubSubClient**: Cliente MQTT
- **Preferences**: Almacenamiento persistente
- **OneWire/DallasTemperature**: Sensores DS18B20
- **Adafruit_AHT20**: Sensor de temperatura/humedad
- **Adafruit_BMP280**: Sensor de presión
- **MQUnifiedsensor**: Sensores de gas MQ
- **ESPmDNS**: Descubrimiento de red
- **ArduinoOTA**: Actualizaciones remotas

## Solución de Problemas

### Problemas Comunes
1. **Sensores no detectados**: Verificar conexiones I2C y alimentación
2. **WiFi no conecta**: Revisar credenciales y señal
3. **Lecturas erróneas**: Calibrar sensores individualmente
4. **Salidas no funcionan**: Verificar relés y alimentación
5. **MQTT no conecta**: Revisar configuración de broker

### Diagnóstico
- Usar monitor serie para depuración
- Verificar estado de sensores en interfaz web
- Revisar logs de sistema
- Probar APIs REST individualmente

## Contribuciones

Las contribuciones son bienvenidas:
1. Fork del proyecto
2. Crear rama para nueva característica
3. Commit con descripción clara
4. Push y crear Pull Request
5. Documentar cambios realizados

## Licencia

Este proyecto está bajo licencia MIT. Ver archivo LICENSE para detalles.

## Autor

**Alessandro Klein**
- GitHub: [@AlessandroKlein](https://github.com/AlessandroKlein)
- Email: contacto@ejemplo.com

## Historial de Versiones

### v1.4.0 (2025-08-02) - Actual
- ✅ **Sistema de Control Automático Completo**: AutomaticControlManager implementado
- ✅ **Control Inteligente de Riego**: Basado en sensores de humedad del suelo
- ✅ **Control de Temperatura**: Automático para ventiladores, calefactores y enfriadores
- ✅ **Control de Iluminación**: Programación horaria para luces de crecimiento
- ✅ **APIs de Configuración**: Endpoints /api/outputs/auto-config para gestión
- ✅ **Persistencia de Configuración**: Guardado automático en memoria flash ESP32
- ✅ **Integración Completa**: Sistema totalmente funcional y probado

### v1.3.0 (2025-01-18)
- ✅ **Actualización de Menús**: Navegación completa mejorada
- ✅ **Corrección Config JSON**: Enlaces y funcionalidad corregidos

### v1.2.0 (2025-01-17)
- ✅ **Sistema de Salidas Completo**: 6 salidas configurables con control automático
- ✅ **Control Inteligente**: Riego, temperatura e iluminación automatizados
- ✅ **Interfaz Web Mejorada**: Dashboard responsive con navegación completa
- ✅ **Calibración Avanzada**: Sistema individual por sensor
- ✅ **APIs REST**: Endpoints completos para integración
- ✅ **Gestión de Configuración**: Exportación/importación JSON
- ✅ **Actualizaciones OTA**: Firmware remoto seguro

### v1.1.0 (2025-01-16)
- ✅ **CSS Externo**: Estilos optimizados y consistentes
- ✅ **Zona Horaria**: Configuración NTP mejorada
- ✅ **UI Mejorada**: Interfaz más intuitiva y moderna

### v1.0.0 (2025-01-15)
- ✅ **Lanzamiento Inicial**: Sistema base funcional
- ✅ **Sensores Múltiples**: AHT20, BMP280, DS18B20, MQ-7, LDR, suelo
- ✅ **Interfaz Web**: Dashboard básico y configuración
- ✅ **WiFi y MQTT**: Conectividad completa
- ✅ **Sistema de Configuración**: Persistencia de datos

## Soporte

Para reportar bugs, solicitar características o obtener ayuda:
- **Issues**: Crear issue en GitHub
- **Documentación**: Wiki del proyecto
- **Comunidad**: Foro de discusiones

---

**Sistema de Invernadero ESP32** - Automatización inteligente para cultivos indoor 🌱
