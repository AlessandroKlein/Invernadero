# AlessandroKlein — Design System & Engineering Style Guide

> **Principio general:** Simple por fuera. Modular por dentro.

Este documento define el estándar de diseño visual, UX, arquitectura de software, organización de proyectos, documentación, configuración, seguridad, desarrollo y mantenimiento para los proyectos de AlessandroKlein.

---

# 1. Propósito

Este Design System establece una base común para todos los proyectos presentes y futuros.

Debe permitir que diferentes proyectos tecnológicos mantengan:

* una identidad visual coherente;
* una arquitectura modular;
* componentes reutilizables;
* páginas extensibles;
* funcionalidades instalables posteriormente;
* funcionalidades habilitables/deshabilitables;
* compatibilidad entre versiones;
* configuración centralizada;
* seguridad consistente;
* documentación uniforme;
* facilidad de mantenimiento y evolución.

El estándar debe ser independiente de una tecnología específica.

Puede aplicarse a:

* ESP32 / firmware;
* C++;
* PlatformIO;
* PHP;
* Python;
* Node.js;
* PostgreSQL;
* MySQL;
* React;
* Vue;
* HTML/CSS/JavaScript;
* aplicaciones web;
* aplicaciones de escritorio;
* APIs;
* servidores;
* sistemas embebidos;
* herramientas internas.

---

# 2. Principios

Todos los proyectos deben seguir estos principios:

1. **Simple por fuera.**
2. **Modular por dentro.**
3. **Claridad antes que decoración.**
4. **Una responsabilidad por módulo.**
5. **Evitar dependencias innecesarias.**
6. **No duplicar lógica.**
7. **Diseñar para crecer.**
8. **Diseñar para poder desactivar funcionalidades.**
9. **Diseñar para poder instalar funcionalidades posteriormente.**
10. **La configuración debe controlar capacidades sin modificar código cuando sea posible.**
11. **El sistema debe continuar funcionando aunque una funcionalidad opcional no esté instalada.**
12. **Las funcionalidades opcionales no deben contaminar el núcleo del sistema.**

---

# 3. Filosofía visual

La interfaz debe transmitir:

* tecnología;
* precisión;
* confiabilidad;
* orden;
* simplicidad;
* funcionalidad;
* aspecto profesional/industrial.

Evitar:

* exceso de efectos;
* gradientes innecesarios;
* animaciones decorativas;
* interfaces sobrecargadas;
* colores sin significado;
* información técnica innecesaria para el usuario común.

---

# 4. Arquitectura modular

La modularidad es un requisito transversal del estándar.

Cada funcionalidad debe poder conceptualizarse como un módulo independiente.

Un módulo debe:

* tener una responsabilidad clara;
* tener una interfaz definida;
* minimizar dependencias;
* poder habilitarse;
* poder deshabilitarse;
* poder actualizarse independientemente cuando sea posible;
* declarar sus dependencias;
* declarar su versión;
* declarar sus capacidades;
* poder ser eliminado sin romper el núcleo del sistema.

La arquitectura debe separar:

```text
CORE
 ├── módulos obligatorios
 └── módulos opcionales
```

Ejemplo:

```text
Aplicación
│
├── Core
│
├── Dashboard
│
├── Usuarios
│
├── Dispositivos
│
├── Sensores
│
├── Automatización
│
├── Alarmas
│
├── Reportes
│
├── OTA
│
└── Diagnóstico
```

No todas las instalaciones tienen que utilizar todos los módulos.

---

# 5. Páginas modulares

Las páginas de una aplicación **NO deben diseñarse como monolitos**.

Una página debe estar compuesta por bloques funcionales independientes.

Ejemplo:

```text
Dashboard
│
├── Header
├── DeviceStatus
├── KPIGrid
├── TemperatureChart
├── HumidityChart
├── IrrigationStatus
├── AlarmPanel
├── WeatherWidget
└── SystemHealth
```

Cada bloque debe poder:

* instalarse;
* registrarse;
* habilitarse;
* deshabilitarse;
* ocultarse;
* configurarse;
* actualizarse independientemente;
* tener permisos propios cuando corresponda.

La página actúa como **contenedor/orquestador**, no como lugar donde se concentra toda la lógica.

---

# 6. Bloques de página

Los bloques visuales deben considerarse unidades reutilizables.

Ejemplo:

```text
Page
 ├── Section
 │    ├── Card
 │    ├── Widget
 │    └── Table
 │
 └── Section
      ├── Chart
      ├── Status
      └── Actions
```

Un bloque debe tener:

* nombre;
* identificador;
* versión;
* configuración;
* estado;
* permisos;
* dependencias;
* punto de montaje;
* documentación.

Ejemplo conceptual:

```json
{
  "id": "temperature-chart",
  "version": "1.2.0",
  "enabled": true,
  "position": "dashboard.main",
  "dependencies": [],
  "permissions": [
    "sensors.read"
  ]
}
```

---

# 7. Sistema de módulos instalables

El sistema debe permitir que una funcionalidad pueda agregarse posteriormente sin tener que reconstruir toda la aplicación.

Ejemplo:

```text
Instalación inicial
│
├── Core
├── Dashboard
└── Usuarios

Posteriormente
│
├── + Reportes
├── + MQTT
├── + Weather
├── + Analytics
└── + Maintenance
```

La instalación de un módulo debe ser independiente del resto del sistema siempre que técnicamente sea posible.

El núcleo debe conocer únicamente la interfaz pública del módulo.

---

# 8. Registro de módulos

Debe existir un **Module Registry** o mecanismo equivalente.

Ejemplo:

```text
ModuleRegistry
│
├── dashboard
├── users
├── devices
├── sensors
├── automation
├── alarms
├── reports
└── diagnostics
```

El registro debe permitir:

* descubrir módulos;
* conocer su versión;
* comprobar dependencias;
* comprobar compatibilidad;
* habilitar módulos;
* deshabilitar módulos;
* obtener capacidades;
* validar permisos;
* verificar estado.

---

# 9. Manifest de módulos

Cada módulo debería disponer de un manifest.

Ejemplo:

```json
{
  "id": "reports",
  "name": "Reports",
  "version": "1.0.0",
  "apiVersion": "1",
  "enabled": true,
  "dependencies": [
    "core",
    "database"
  ],
  "permissions": [
    "reports.read",
    "reports.export"
  ],
  "routes": [
    "/reports"
  ],
  "capabilities": [
    "pdf-export",
    "csv-export"
  ]
}
```

El manifest permite que el sistema conozca qué contiene un módulo sin tener que acoplarse a su implementación interna.

---

# 10. Dependencias entre módulos

Los módulos pueden declarar dependencias.

Ejemplo:

```text
Reports
 ├── Core
 ├── Database
 └── Authentication
```

El sistema debe comprobar:

1. módulo requerido instalado;
2. versión compatible;
3. dependencia habilitada;
4. permisos disponibles;
5. configuración válida.

No debe permitirse activar un módulo que tenga dependencias incompatibles.

---

# 11. Habilitar y deshabilitar funcionalidades

Toda funcionalidad opcional debe poder tener al menos estos estados:

```text
INSTALLED
ENABLED
DISABLED
ERROR
UPDATING
```

Ejemplo:

```text
Reports
Version: 1.4.0
Installed: YES
Enabled: NO
Status: DISABLED
```

Deshabilitar un módulo debe:

* impedir su ejecución;
* ocultar sus elementos de navegación;
* impedir sus acciones;
* conservar su configuración cuando sea conveniente;
* conservar sus datos salvo que el usuario solicite eliminarlos.

**Deshabilitar no significa desinstalar.**

---

# 12. Instalado ≠ habilitado

El sistema debe distinguir claramente:

```text
Installed
```

de:

```text
Enabled
```

Ejemplo:

```text
Weather Module
Installed: YES
Enabled: NO
```

Esto permite preparar una instalación y activar funcionalidades posteriormente.

---

# 13. Desinstalación

La desinstalación debe ser independiente de la deshabilitación.

Estados:

```text
Not Installed
Installed / Disabled
Installed / Enabled
```

Al desinstalar debe definirse qué ocurre con:

* configuración;
* datos;
* tablas;
* archivos;
* permisos;
* rutas;
* tareas programadas;
* eventos;
* widgets;
* menús.

Nunca debe eliminarse información histórica automáticamente sin una acción explícita cuando esa información pueda tener valor.

---

# 14. Feature Flags

Los Feature Flags deben utilizarse para controlar funcionalidades.

Ejemplo:

```json
{
  "features": {
    "reports": true,
    "weather": false,
    "advancedDiagnostics": true,
    "experimentalDashboard": false
  }
}
```

Los Feature Flags permiten:

* activar funciones;
* desactivar funciones;
* realizar pruebas;
* realizar despliegues progresivos;
* ocultar funciones experimentales;
* mantener compatibilidad.

---

# 15. Capability System

Las capacidades representan lo que un sistema puede hacer.

Ejemplo:

```text
device.sensors.read
device.sensors.write
device.ota
device.mqtt
device.modbus
device.sd
device.ethernet
```

La interfaz no debe asumir que todos los dispositivos poseen las mismas capacidades.

Debe preguntar:

```text
¿La capacidad existe?
```

antes de mostrar o ejecutar una función.

---

# 16. Capability-Based UI

La interfaz debe construirse en función de las capacidades disponibles.

Ejemplo:

```text
ESP32 básico
 ├── WiFi
 ├── Sensors
 └── Local Web

ESP32 + W5500
 ├── WiFi
 ├── Ethernet
 ├── Sensors
 └── Local Web

ESP32 + SD
 ├── WiFi
 ├── SD
 ├── History
 └── Export
```

La UI no debe mostrar botones de funcionalidades que el dispositivo no soporta.

---

# 17. Páginas dinámicas

Las páginas deben poder construirse dinámicamente a partir de módulos disponibles.

Conceptualmente:

```text
Page Renderer
      │
      ▼
Module Registry
      │
      ├── Module A
      ├── Module B
      ├── Module C
      └── Module D
             │
             ▼
        Enabled Modules
             │
             ▼
        Page Blocks
```

Esto permite que una misma página pueda variar según:

* hardware;
* permisos;
* configuración;
* módulos instalados;
* módulos habilitados;
* versión;
* capacidades;
* usuario.

---

# 18. Layout Slots

Las páginas deben utilizar puntos de montaje definidos.

Ejemplo:

```text
dashboard.header
dashboard.main
dashboard.sidebar
dashboard.footer
```

Un módulo puede registrar un bloque en un slot.

Ejemplo:

```json
{
  "module": "weather",
  "block": "weather-card",
  "slot": "dashboard.main",
  "priority": 30
}
```

Esto evita modificar manualmente la página cada vez que se agrega una funcionalidad.

---

# 19. Orden de bloques

Cada bloque puede tener una prioridad.

Ejemplo:

```text
Priority 10 → System Status
Priority 20 → Temperature
Priority 30 → Humidity
Priority 40 → Weather
Priority 50 → Irrigation
```

La página ordena automáticamente los bloques.

Cuando sea necesario, el usuario podrá personalizar el orden.

---

# 20. Configuración de bloques

Los bloques pueden tener configuración independiente.

Ejemplo:

```json
{
  "id": "temperature-chart",
  "enabled": true,
  "settings": {
    "period": "24h",
    "showAverage": true,
    "showMinMax": true
  }
}
```

La configuración del bloque no debe mezclarse innecesariamente con la configuración global.

---

# 21. Bloques reutilizables

Un mismo bloque puede utilizarse en múltiples páginas.

Ejemplo:

```text
TemperatureCard
 ├── Dashboard
 ├── Device
 ├── Sensors
 └── Reports
```

Debe evitarse implementar una versión diferente del mismo componente para cada página.

---

# 22. Lazy Loading

Cuando la tecnología utilizada lo permita, los módulos opcionales deben cargarse bajo demanda.

Ejemplo:

```text
Application
│
├── Core → carga inmediata
│
├── Dashboard → carga inmediata
│
├── Reports → carga bajo demanda
├── Analytics → carga bajo demanda
└── Maintenance → carga bajo demanda
```

Esto permite reducir:

* tiempo inicial;
* memoria;
* ancho de banda;
* complejidad de la interfaz.

---

# 23. Menú modular

El menú de navegación también debe ser modular.

Un módulo puede registrar:

```json
{
  "menu": {
    "label": "Reports",
    "icon": "chart",
    "route": "/reports",
    "permission": "reports.read"
  }
}
```

Si el módulo está:

```text
disabled
```

su entrada no debe aparecer.

Si el usuario no posee el permiso:

```text
reports.read
```

tampoco debe aparecer o debe quedar inaccesible según el modelo de seguridad.

---

# 24. Permisos por módulo

Los módulos pueden declarar permisos propios.

Ejemplo:

```text
reports.read
reports.create
reports.export
reports.delete
```

Esto permite aplicar RBAC sin acoplar el sistema al módulo.

---

# 25. Módulos opcionales vs Core

Debe existir una separación clara:

```text
CORE
```

contiene únicamente lo indispensable.

```text
OPTIONAL MODULES
```

contiene funcionalidades que pueden agregarse posteriormente.

Ejemplo:

```text
Core
 ├── Authentication
 ├── Configuration
 ├── Logging
 ├── Routing
 └── ModuleRegistry

Optional
 ├── Weather
 ├── Reports
 ├── Analytics
 ├── MQTT
 ├── Modbus
 └── AdvancedDiagnostics
```

El Core no debe depender de módulos opcionales.

---

# 26. Regla de dependencia

La dependencia debe ir preferentemente:

```text
Optional Module
        ↓
      Core
```

y no:

```text
Core
 ↓
Optional Module
```

El Core nunca debe requerir un módulo opcional para funcionar.

---

# 27. Módulos desacoplados

Un módulo no debe acceder directamente a la implementación interna de otro módulo.

Incorrecto:

```text
Reports
 └── accede directamente a DatabaseInternals
```

Correcto:

```text
Reports
   ↓
DatabaseService
   ↓
Database
```

Las interfaces públicas son preferibles a las dependencias internas.

---

# 28. Versionado de módulos

Cada módulo debe tener su propia versión.

Ejemplo:

```text
Core       3.0.0
Dashboard  2.4.0
Reports    1.2.0
Weather    1.0.3
```

El sistema debe poder determinar qué versión está instalada.

---

# 29. Compatibilidad de módulos

Un módulo puede declarar:

```text
minCoreVersion
maxCoreVersion
apiVersion
schemaVersion
```

Ejemplo:

```json
{
  "apiVersion": 2,
  "minCoreVersion": "3.0.0"
}
```

Esto evita instalar módulos incompatibles.

---

# 30. Migraciones de módulos

Si una nueva versión necesita modificar datos o configuración:

```text
Module v1
   ↓
Migration
   ↓
Module v2
```

Las migraciones deben:

* ser versionadas;
* ser reproducibles;
* validarse;
* registrar errores;
* evitar pérdida de información;
* poder auditarse.

---

# 31. Estado persistente de módulos

El sistema debe almacenar:

```text
module_id
version
installed
enabled
configuration
schema_version
status
```

Ejemplo:

```json
{
  "id": "reports",
  "installed": true,
  "enabled": false,
  "version": "1.4.0",
  "schemaVersion": 2
}
```

---

# 32. Recuperación ante errores

Si un módulo falla, el sistema debe evitar que derribe toda la aplicación cuando sea técnicamente posible.

Ejemplo:

```text
Core                 → RUNNING
Dashboard            → RUNNING
Weather              → ERROR
Reports              → RUNNING
```

La interfaz debe indicar:

```text
Weather
Estado: Error
```

sin convertir necesariamente todo el sistema en estado de error.

---

# 33. Aislamiento de módulos

Cuando la plataforma lo permita, los módulos deben estar aislados mediante:

* namespaces;
* paquetes;
* servicios;
* componentes;
* procesos;
* contenedores;
* plugins;
* interfaces;
* APIs.

El nivel de aislamiento dependerá de la tecnología.

---

# 34. Modularidad del frontend

La estructura recomendada:

```text
frontend/
├── core/
├── components/
├── layouts/
├── pages/
├── modules/
│   ├── dashboard/
│   ├── users/
│   ├── devices/
│   ├── sensors/
│   ├── reports/
│   └── weather/
├── services/
├── stores/
├── routes/
└── styles/
```

Cada módulo debe contener, cuando corresponda:

```text
module/
├── components/
├── pages/
├── services/
├── stores/
├── types/
├── config/
├── manifest.json
└── index.*
```

---

# 35. Página como composición

Una página no debería contener toda la implementación.

Incorrecto:

```text
Dashboard.vue
 ├── 2000 líneas
 ├── gráficos
 ├── API
 ├── permisos
 ├── configuración
 └── lógica de negocio
```

Preferible:

```text
Dashboard
 ├── DashboardHeader
 ├── KPIGrid
 ├── TemperatureWidget
 ├── HumidityWidget
 ├── IrrigationWidget
 └── AlarmWidget
```

---

# 36. Arquitectura de componentes

Separar:

```text
UI Components
      ↓
Feature Components
      ↓
Services
      ↓
API
```

Cada nivel debe tener una responsabilidad clara.

---

# 37. Componentes genéricos vs específicos

Genéricos:

```text
Button
Card
Modal
Table
Chart
Badge
Tabs
Form
```

Específicos:

```text
SensorCard
DeviceStatus
IrrigationControl
AlarmPanel
FirmwareUpdate
```

No convertir todo en componentes genéricos artificialmente.

---

# 38. Instalación posterior de páginas

Una nueva página debe poder agregarse como módulo.

Ejemplo:

```text
Sistema inicial
 ├── /dashboard
 ├── /devices
 └── /settings

Instalación posterior
 └── + /reports
```

La incorporación de `/reports` no debería requerir modificar manualmente todas las páginas existentes.

---

# 39. Instalación posterior de bloques

También debe ser posible instalar únicamente un bloque.

Ejemplo:

```text
Dashboard
```

original:

```text
Temperature
Humidity
```

posteriormente:

```text
+ CO2 Widget
+ Weather Widget
+ Energy Widget
```

sin rediseñar todo el Dashboard.

---

# 40. Configuración por instalación

Una instalación puede tener:

```text
Dashboard
 ├── Temperature       ENABLED
 ├── Humidity          ENABLED
 ├── CO2               DISABLED
 ├── Weather           ENABLED
 └── Energy            DISABLED
```

Otra instalación puede utilizar una configuración diferente.

El código base sigue siendo el mismo.

---

# 41. Modularidad por proyecto

La misma filosofía debe aplicarse al repositorio completo.

Ejemplo:

```text
project/
├── core/
├── modules/
├── plugins/
├── services/
├── drivers/
├── docs/
└── tests/
```

No debe crearse una arquitectura monolítica simplemente porque inicialmente haya pocas funcionalidades.

---

# 42. Firmware modular

En firmware:

```text
Firmware
├── Core
├── Config
├── Hardware
├── Sensors
├── Actuators
├── Controllers
├── Automation
├── Network
├── Protocols
├── Storage
├── Diagnostics
├── Security
└── OTA
```

Los drivers y funcionalidades opcionales deben registrarse mediante abstracciones.

Ejemplo:

```text
SensorRegistry
ActuatorRegistry
BusRegistry
ModuleRegistry
CapabilityRegistry
```

---

# 43. Hardware opcional

El firmware debe soportar hardware opcional sin asumir que siempre está presente.

Ejemplo:

```text
W5500
SD
ADS1115
MCP23017
74HC165
74HC595
RS485
```

Cada dispositivo debe poder:

```text
detectarse
registrarse
habilitarse
deshabilitarse
diagnosticarse
```

---

# 44. Capability Discovery

El sistema debe detectar capacidades disponibles.

Ejemplo:

```json
{
  "capabilities": [
    "wifi",
    "ethernet",
    "sd",
    "modbus",
    "temperature",
    "humidity"
  ]
}
```

El frontend utiliza esa información para construir la interfaz correspondiente.

---

# 45. Configuración dinámica

La configuración debe poder determinar qué módulos están activos.

Ejemplo:

```json
{
  "modules": {
    "weather": {
      "enabled": true
    },
    "reports": {
      "enabled": false
    }
  }
}
```

---

# 46. Backend modular

El servidor debe seguir la misma arquitectura.

```text
server/
├── core/
├── modules/
│   ├── users/
│   ├── devices/
│   ├── sensors/
│   ├── alarms/
│   ├── reports/
│   └── firmware/
├── services/
├── repositories/
├── api/
└── database/
```

Cada módulo debe poder registrar:

* endpoints;
* servicios;
* permisos;
* tareas;
* eventos;
* tablas/migraciones;
* configuraciones.

---

# 47. API modular

Los módulos pueden registrar endpoints.

Ejemplo:

```text
/api/v1/devices
/api/v1/sensors
/api/v1/reports
/api/v1/weather
```

Si `reports` no está instalado:

```text
/api/v1/reports
```

no debe existir o debe devolver una respuesta controlada de funcionalidad no disponible.

---

# 48. MQTT modular

Los módulos pueden registrar sus propios topics.

Ejemplo:

```text
device/{id}/sensors/...
device/{id}/weather/...
device/{id}/reports/...
```

No deben publicarse topics correspondientes a módulos deshabilitados salvo que exista una razón técnica documentada.

---

# 49. WebSocket modular

Los módulos también pueden registrar eventos WebSocket.

Ejemplo:

```text
weather.updated
alarm.created
report.generated
device.status.changed
```

El Core administra el transporte.

El módulo administra su evento.

---

# 50. Configuración modular

La configuración global debe separarse de la configuración de cada módulo.

Ejemplo:

```json
{
  "system": {},
  "network": {},
  "security": {},
  "modules": {
    "weather": {},
    "reports": {},
    "automation": {}
  }
}
```

Esto facilita:

* exportación;
* importación;
* migraciones;
* backup;
* restauración;
* clonación.

---

# 51. Exportación e importación

La exportación debe permitir decidir:

```text
Global configuration
Module configuration
User preferences
Device configuration
```

No deben exportarse automáticamente:

* contraseñas;
* tokens;
* claves privadas;
* credenciales;
* identidad sensible.

---

# 52. Backup de módulos

Antes de actualizar o desinstalar un módulo importante debe poder realizarse un backup de:

```text
configuration
schema
data
version
```

cuando corresponda.

---

# 53. Seguridad modular

La seguridad debe considerar:

```text
Module
 ├── Permissions
 ├── Routes
 ├── API
 ├── Configuration
 └── Actions
```

Un módulo no debe obtener más permisos de los necesarios.

Aplicar principio:

> **Least Privilege.**

---

# 54. Auditoría modular

Las acciones importantes deben registrar:

```text
timestamp
user
module
action
resource
result
```

Ejemplo:

```text
User: admin
Module: firmware
Action: update
Resource: device-001
Result: success
```

---

# 55. Observabilidad modular

Cada módulo debería poder exponer:

```text
status
health
version
errors
metrics
dependencies
```

Ejemplo:

```text
Reports
Status: RUNNING
Version: 1.2.0
Health: OK
```

---

# 56. Diagnóstico

El sistema debe permitir determinar:

```text
Core → OK
Module A → OK
Module B → ERROR
Dependency → MISSING
```

Esto debe facilitar soporte y mantenimiento.

---

# 57. UI de administración de módulos

Cuando exista una sección administrativa, debe mostrar:

```text
Modules
──────────────────────────────
Dashboard       Installed   ON
Reports         Installed   OFF
Weather         Installed   ON
Analytics       Not installed
```

Acciones posibles:

```text
Install
Enable
Disable
Update
Uninstall
Configure
View diagnostics
```

Las acciones peligrosas deben requerir confirmación.

---

# 58. Actualización modular

Una actualización de módulo debe poder realizarse sin actualizar todo el sistema cuando la arquitectura lo permita.

Ejemplo:

```text
Core 3.0.0
Reports 1.2.0
Weather 2.0.0
```

Actualizar:

```text
Weather 2.0.0 → 2.1.0
```

no debería requerir necesariamente actualizar:

```text
Core
Reports
```

---

# 59. Compatibilidad API

Los módulos deben utilizar APIs públicas y versionadas.

Nunca depender de APIs internas no documentadas.

Ejemplo:

```text
Core API v1
Core API v2
```

Un módulo debe declarar qué versión necesita.

---

# 60. Reglas de modularidad

Toda nueva funcionalidad debe evaluarse antes de implementarse:

### Pregunta 1

¿Es parte del Core?

### Pregunta 2

¿Es una funcionalidad opcional?

### Pregunta 3

¿Puede implementarse como módulo?

### Pregunta 4

¿Puede reutilizarse como componente/bloque?

### Pregunta 5

¿Tiene dependencias?

### Pregunta 6

¿Debe poder deshabilitarse?

### Pregunta 7

¿Debe poder instalarse posteriormente?

### Pregunta 8

¿Tiene configuración propia?

### Pregunta 9

¿Tiene permisos propios?

### Pregunta 10

¿Tiene migraciones propias?

---

# 61. Regla de diseño de páginas

**Ninguna página grande debe convertirse en un monolito.**

La página debe funcionar como:

```text
Layout
 +
Slots
 +
Modules
 +
Blocks
```

y no como:

```text
Page
 └── toda la aplicación
```

---

# 62. Regla de diseño de funcionalidades

Toda funcionalidad nueva debe intentar encapsular:

```text
UI
Logic
API
Configuration
Permissions
Events
Documentation
Tests
```

dentro de su módulo correspondiente.

---

# 63. Regla de instalación

Una funcionalidad nueva debe poder incorporarse con el mínimo impacto posible sobre:

* Core;
* módulos existentes;
* páginas existentes;
* base de datos;
* API;
* navegación.

El objetivo es:

> **Agregar funcionalidades, no reescribir el sistema.**

---

# 64. Regla de desactivación

Toda funcionalidad opcional debe tener un camino seguro de desactivación.

Deshabilitar una funcionalidad no debe provocar:

* errores de navegación;
* páginas rotas;
* referencias inexistentes;
* errores JavaScript;
* endpoints inválidos;
* tareas huérfanas;
* bloqueos del Core.

---

# 65. Regla de eliminación

Antes de eliminar un módulo se debe verificar:

```text
Dependencias
Datos
Configuración
Permisos
Rutas
Eventos
Jobs
Integraciones
```

---

# 66. Regla de consistencia

Un módulo debe comportarse igual independientemente de la página donde se utilice.

Ejemplo:

```text
TemperatureWidget
```

debe conservar:

* mismos estados;
* mismas unidades;
* misma semántica;
* mismos iconos;
* mismos colores;
* misma interacción.

---

# 67. Feature Flags vs Modules

No son lo mismo.

### Module

Representa una funcionalidad independiente.

```text
Reports
Weather
Analytics
```

### Feature Flag

Controla una variante o estado de una funcionalidad.

```text
newDashboard
advancedCharts
experimentalMode
```

Regla:

> **Usar módulos para funcionalidades independientes y Feature Flags para variaciones/control temporal de funcionalidades.**

---

# 68. Progressive Disclosure

La interfaz debe mostrar inicialmente lo esencial.

La información avanzada debe estar disponible mediante:

```text
Advanced
Details
Diagnostics
Technical information
```

---

# 69. Identidad visual

Mantener:

* tipografía consistente;
* colores semánticos;
* espaciado;
* iconografía;
* componentes;
* estados;
* radii;
* sombras;
* comportamiento responsive.

---

# 70. Sistema de colores

Utilizar tokens semánticos.

Ejemplo:

```css
--color-primary
--color-background
--color-surface
--color-text
--color-muted
--color-success
--color-warning
--color-danger
--color-info
```

Nunca depender de colores arbitrarios por componente.

---

# 71. Estados

Todos los componentes relevantes deben contemplar:

```text
default
hover
focus
active
disabled
loading
success
warning
error
```

---

# 72. Dark Mode

Soportar:

```text
Light
Dark
System
```

cuando la aplicación lo requiera.

---

# 73. Tipografía

La jerarquía debe ser clara:

```text
Display
H1
H2
H3
Body
Small
Caption
Technical
```

---

# 74. Iconografía

Utilizar una única familia de iconos por proyecto siempre que sea posible.

Los iconos deben:

* tener significado;
* ser consistentes;
* no reemplazar texto crítico;
* respetar accesibilidad.

---

# 75. Emoji

Los emojis no deben utilizarse como sustitutos de iconos técnicos en interfaces profesionales.

Pueden utilizarse en:

* documentación informal;
* mensajes internos;
* ejemplos;
* comunicación no crítica.

---

# 76. Espaciado

Utilizar una escala consistente.

Ejemplo:

```text
4
8
12
16
24
32
48
64
```

---

# 77. Border Radius

Mantener pocos niveles:

```text
small
medium
large
pill
```

---

# 78. Sombras

Utilizar sombras discretas.

No utilizar sombras decorativas excesivas.

---

# 79. Layout

Las interfaces deben priorizar:

* jerarquía;
* alineación;
* espacio;
* densidad controlada;
* lectura rápida.

---

# 80. Sidebar

La navegación debe ser consistente entre proyectos.

Debe soportar:

* módulos;
* submódulos;
* permisos;
* estados;
* badges;
* elementos deshabilitados cuando corresponda.

---

# 81. Dashboard

Los dashboards deben ser composiciones de widgets.

No deben convertirse en una página monolítica.

---

# 82. Cards

Las cards deben utilizarse para agrupar información relacionada.

Evitar convertir cada pequeño dato en una card.

---

# 83. KPIs

Los KPIs deben mostrar:

```text
Value
Unit
Context
Trend
Status
```

cuando corresponda.

---

# 84. Charts

Los gráficos deben:

* ser legibles;
* tener unidades;
* mostrar rango temporal;
* manejar ausencia de datos;
* manejar loading;
* manejar error;
* ser responsive.

---

# 85. Formularios

Los formularios deben:

* agrupar campos;
* validar;
* mostrar errores claramente;
* indicar unidades;
* diferenciar valores obligatorios/opcionales;
* permitir cancelar cambios.

---

# 86. Configuración avanzada

Las configuraciones técnicas deben utilizar progressive disclosure.

Ejemplo:

```text
General
Advanced
Hardware
Diagnostics
Developer
```

---

# 87. Confirmaciones

Acciones destructivas requieren confirmación.

Ejemplo:

```text
Delete module
Reset configuration
Factory reset
Remove device
```

---

# 88. Notificaciones

Utilizar:

```text
Success
Info
Warning
Error
```

Las notificaciones no deben ocultar información crítica.

---

# 89. Loading

Todo proceso que pueda tardar debe tener feedback visual.

---

# 90. Errores

Los errores deben indicar:

```text
What happened
Why
What can be done
Technical details
```

cuando corresponda.

---

# 91. Accessibility

Debe contemplarse:

* teclado;
* contraste;
* foco;
* labels;
* lectores de pantalla;
* tamaño táctil;
* estados no dependientes únicamente del color.

---

# 92. Responsive

Soportar como mínimo:

```text
Desktop
Tablet
Mobile
```

---

# 93. Tables

Las tablas deben priorizar:

* lectura;
* búsqueda;
* filtros;
* orden;
* responsive;
* acciones claras.

---

# 94. Frontend Architecture

Separar:

```text
Components
Pages
Modules
Services
Stores
Types
Routes
Styles
```

---

# 95. Feature Organization

Organizar por funcionalidad antes que por tipo técnico cuando el proyecto crezca.

Preferido:

```text
modules/devices/
modules/sensors/
modules/reports/
```

sobre una estructura excesivamente global:

```text
components/
services/
utils/
```

sin separación funcional.

---

# 96. Services

Los servicios deben encapsular comunicación externa y lógica de infraestructura.

---

# 97. Global State

Solo mantener globalmente lo que realmente sea global.

---

# 98. API Frontend

La comunicación con API debe estar centralizada y tipada cuando sea posible.

---

# 99. Naming Frontend

Usar nombres descriptivos y consistentes.

---

# 100. File Naming

Utilizar una convención única por tecnología.

---

# 101. Firmware

Mantener firmware modular.

---

# 102. main.cpp

`main.cpp` debe permanecer pequeño.

Su función principal:

```text
initialize
register
start
loop
```

No debe contener toda la lógica del sistema.

---

# 103. Managers

Los Managers administran recursos o subsistemas.

Ejemplo:

```text
ConfigManager
NetworkManager
StorageManager
SensorManager
ModuleManager
```

---

# 104. Controllers

Los Controllers implementan comportamiento.

Ejemplo:

```text
ClimateController
IrrigationController
LightingController
```

---

# 105. Registry

Los Registries administran elementos dinámicos.

Ejemplo:

```text
SensorRegistry
ActuatorRegistry
ModuleRegistry
CapabilityRegistry
```

---

# 106. Drivers

Los Drivers encapsulan hardware específico.

---

# 107. Abstraction

El código de aplicación no debe depender directamente del hardware cuando pueda evitarse.

---

# 108. Security

Seguridad desde el diseño.

---

# 109. Configuration

Toda configuración debe ser validada antes de aplicarse.

---

# 110. Versioned Configuration

Utilizar `schemaVersion`.

---

# 111. Import / Export

Exportar configuración de forma controlada.

---

# 112. Logs

Logs estructurados y con niveles.

---

# 113. Events

Utilizar eventos para desacoplar módulos cuando corresponda.

---

# 114. Identifiers

IDs estables y únicos.

---

# 115. Dates

ISO 8601 cuando corresponda.

---

# 116. Units

Las unidades deben ser explícitas.

---

# 117. API

La API debe ser versionada.

Ejemplo:

```text
/api/v1/
```

---

# 118. API Response

Mantener respuestas consistentes.

---

# 119. MQTT

Utilizar una jerarquía de topics estable.

---

# 120. WebSocket

Utilizar eventos estructurados.

---

# 121. OTA

Las actualizaciones deben validar:

* hardware;
* SoC;
* versión;
* compatibilidad;
* checksum;
* rollback.

---

# 122. Compatibility

Las versiones deben declarar compatibilidad.

---

# 123. Feature Flags

Los flags deben ser documentados y tener propietario funcional.

---

# 124. Capability System

La aplicación debe consultar capacidades antes de mostrar funciones dependientes de ellas.

---

# 125. Capability-Based Design

Diseñar contra capacidades, no contra modelos concretos.

---

# 126. Database

La base de datos debe estar versionada mediante migraciones.

---

# 127. Multi-Tenant

Cuando corresponda, separar claramente:

```text
Tenant
User
Device
Resource
```

---

# 128. RBAC

Utilizar roles y permisos.

---

# 129. Criticality

Clasificar acciones por criticidad.

---

# 130. Audit

Auditar operaciones críticas.

---

# 131. Offline-First

El sistema debe continuar funcionando cuando sea posible sin servidor central.

---

# 132. Git

Utilizar Git de manera estructurada.

---

# 133. Commits

Preferir Conventional Commits.

Ejemplo:

```text
feat:
fix:
refactor:
docs:
test:
chore:
```

---

# 134. Pull Requests

PRs pequeños y revisables.

---

# 135. CHANGELOG

Mantener cambios relevantes documentados.

---

# 136. Versioning

Utilizar Semantic Versioning cuando corresponda:

```text
MAJOR.MINOR.PATCH
```

---

# 137. Repository Structure

Mantener una estructura clara y predecible.

---

# 138. Documentation

Separar:

```text
README
docs/
Wiki
API docs
Architecture docs
```

---

# 139. README

El README explica rápidamente:

* qué es;
* para qué sirve;
* cómo instalar;
* cómo utilizar;
* arquitectura general.

---

# 140. Decision Records

Las decisiones importantes deben documentarse como ADR o documentos equivalentes.

---

# 141. Wiki

La Wiki debe contener información técnica ampliada.

---

# 142. Design Docs

Las decisiones arquitectónicas complejas deben documentarse antes o durante su implementación.

---

# 143. Screenshots

Las capturas deben mantenerse actualizadas.

---

# 144. Diagrams

Los diagramas deben reflejar la arquitectura actual.

---

# 145. Hardware Docs

Documentar:

* conexiones;
* pines;
* alimentación;
* protecciones;
* componentes.

---

# 146. BOM

Mantener BOM cuando el proyecto tenga hardware.

---

# 147. Hardware Configuration

La configuración de hardware debe estar separada de la lógica de aplicación.

---

# 148. Code Quality

Priorizar:

* legibilidad;
* mantenibilidad;
* modularidad;
* testabilidad.

---

# 149. Comments

Comentar el porqué, no únicamente el qué.

---

# 150. TODO

Los TODO importantes deben convertirse en issues o documentación.

---

# 151. Testing

Las funcionalidades importantes deben tener pruebas.

---

# 152. Firmware Testing

Separar:

```text
Unit
Integration
Hardware
System
```

---

# 153. Fail-Safe

Los sistemas críticos deben fallar de manera segura.

---

# 154. Physical Safety

Separar lógica y potencia.

---

# 155. Power vs Logic

No utilizar GPIO directamente para cargas que requieran drivers de potencia.

---

# 156. Environment Variables

Las credenciales y secretos no deben estar hardcodeados.

---

# 157. Docker

Cuando se utilice Docker, los servicios deben ser reproducibles.

---

# 158. Local Development

Documentar cómo ejecutar el proyecto localmente.

---

# 159. Scripts

Los scripts repetitivos deben automatizarse.

---

# 160. CI/CD

Validar automáticamente:

* compilación;
* tests;
* lint;
* documentación;
* artefactos.

---

# 161. Artifacts

Los artefactos generados deben distinguirse del código fuente.

---

# 162. Releases

Las releases deben incluir:

* versión;
* cambios;
* compatibilidad;
* artefactos;
* checksum.

---

# 163. Checksums

Utilizar SHA-256 para artefactos críticos.

---

# 164. Audit Logs

Registrar acciones críticas.

---

# 165. UX of Technical Systems

El usuario debe comprender el sistema sin conocer su implementación interna.

---

# 166. Progressive Disclosure

Mostrar primero lo importante y luego la información técnica.

---

# 167. Terminology

Mantener un vocabulario consistente.

---

# 168. Language

Separar identificadores de código del lenguaje de presentación.

---

# 169. Internationalization

Preparar la arquitectura para múltiples idiomas cuando corresponda.

---

# 170. Units and Locale

Separar:

```text
valor
unidad
formato
locale
```

---

# 171. Mobile

El sistema debe ser usable en dispositivos móviles.

---

# 172. Touch

Los elementos táctiles deben tener tamaño adecuado.

---

# 173. Dangerous Actions

Las acciones peligrosas deben requerir confirmación.

---

# 174. Emergency

Las funciones de emergencia deben tener prioridad visual y técnica.

---

# 175. Animations

Las animaciones deben tener una finalidad funcional.

---

# 176. Response Time

La UI debe informar cuando una operación no es inmediata.

---

# 177. Distributed State

Distinguir:

```text
Desired
Reported
Actual
```

---

# 178. Distributed Architecture

El sistema debe tolerar pérdida temporal de conectividad.

---

# 179. Offline Design

La pérdida del servidor no debe inutilizar un dispositivo autónomo cuando el diseño lo permita.

---

# 180. Synchronization

La sincronización debe definir:

* fuente;
* prioridad;
* timestamp;
* versión;
* conflicto.

---

# 181. Conflict Resolution

Las reglas de resolución de conflictos deben estar documentadas.

---

# 182. Observability

Cada sistema importante debe poder responder:

```text
¿Está funcionando?
¿Por qué no?
¿Qué versión tiene?
¿Qué módulos están activos?
¿Qué capacidades tiene?
```

---

# 183. Health Endpoint

Exponer health checks cuando corresponda.

---

# 184. Device Diagnostics

Mostrar información de diagnóstico.

---

# 185. Technical Information

Separar información técnica avanzada de la información normal.

---

# 186. Configuration Design

Las configuraciones deben ser explícitas y versionadas.

---

# 187. Config Save Workflow

Flujo recomendado:

```text
Edit
 ↓
Validate
 ↓
Preview
 ↓
Apply
 ↓
Persist
 ↓
Verify
```

---

# 188. Rollback

Toda configuración crítica debe poder recuperarse.

---

# 189. Hardware Config UX

La configuración de hardware avanzada debe estar protegida.

---

# 190. Device Naming

Los dispositivos deben tener nombres legibles además de IDs técnicos.

---

# 191. Inventory

Mantener inventario cuando corresponda.

---

# 192. Maintenance

La arquitectura debe permitir mantenimiento preventivo.

---

# 193. Calibration

Los sensores calibrables deben tener mecanismos de calibración documentados.

---

# 194. Alerts

Las alertas deben tener:

```text
severity
source
timestamp
status
acknowledgement
```

---

# 195. Severity

Utilizar niveles consistentes:

```text
INFO
NOTICE
WARNING
ERROR
CRITICAL
```

---

# 196. Alarm UX

Las alarmas críticas deben permanecer visibles hasta su reconocimiento o resolución.

---

# 197. AI / Development Tool Documentation

Las reglas para herramientas de IA deben estar documentadas.

---

# 198. AI Rule

Las herramientas de IA deben respetar:

* arquitectura;
* naming;
* módulos;
* documentación;
* seguridad;
* tests;
* no duplicación.

---

# 199. No Duplicate Logic

Una funcionalidad no debe implementarse dos veces por comodidad.

---

# 200. No Unnecessary Files

No crear archivos innecesarios.

---

# 201. Temporary Files

Los archivos temporales deben permanecer fuera del repositorio.

---

# 202. .gitignore

Debe mantenerse actualizado.

---

# 203. License

Definir licencia del proyecto.

---

# 204. SECURITY.md

Documentar cómo reportar vulnerabilidades.

---

# 205. CONTRIBUTING.md

Documentar cómo contribuir.

---

# 206. Living Documentation

La documentación debe evolucionar junto con el código.

---

# 207. Consistency Rule

Una misma funcionalidad debe comportarse de forma consistente en:

```text
Frontend
Backend
API
Firmware
Documentation
```

---

# 208. Definition of Done

Una funcionalidad no está terminada hasta que:

* funciona;
* está integrada;
* está probada;
* está documentada;
* respeta el Design System;
* contempla errores;
* contempla seguridad;
* contempla configuración;
* contempla compatibilidad.

Para funcionalidades modulares además:

* tiene manifest;
* declara dependencias;
* declara permisos;
* puede habilitarse/deshabilitarse;
* no rompe el Core;
* tiene versión;
* tiene migración si corresponde;
* tiene diagnóstico.

---

# 209. New Feature Checklist

Antes de agregar una funcionalidad:

```text
[ ] ¿Es Core o Module?
[ ] ¿Puede ser opcional?
[ ] ¿Tiene dependencias?
[ ] ¿Tiene permisos?
[ ] ¿Tiene configuración?
[ ] ¿Puede deshabilitarse?
[ ] ¿Puede instalarse posteriormente?
[ ] ¿Puede eliminarse?
[ ] ¿Tiene versión?
[ ] ¿Tiene tests?
[ ] ¿Tiene documentación?
[ ] ¿Tiene diagnóstico?
```

---

# 210. New Screen Checklist

```text
[ ] ¿La página está compuesta por bloques?
[ ] ¿Los bloques son reutilizables?
[ ] ¿Existen slots?
[ ] ¿Los módulos pueden registrar bloques?
[ ] ¿La página funciona sin módulos opcionales?
[ ] ¿Se respetan permisos?
[ ] ¿Se respetan capabilities?
[ ] ¿Tiene estados loading/error/empty?
[ ] ¿Es responsive?
[ ] ¿Es accesible?
```

---

# 211. New Device Checklist

```text
[ ] Driver
[ ] Detection
[ ] Registry
[ ] Capability
[ ] Configuration
[ ] Diagnostics
[ ] Documentation
[ ] Tests
```

---

# 212. New API Checklist

```text
[ ] Versionada
[ ] Autenticada
[ ] Autorizada
[ ] Documentada
[ ] Validada
[ ] Errores consistentes
[ ] Auditada cuando corresponde
```

---

# 213. Hardware Checklist

```text
[ ] Alimentación
[ ] GND
[ ] Protección
[ ] Driver
[ ] Pinout
[ ] Compatibilidad
[ ] Documentación
```

---

# 214. Reference Architecture

La arquitectura general recomendada:

```text
                    ┌──────────────────────┐
                    │        CORE          │
                    │ Configuration        │
                    │ Security             │
                    │ Logging              │
                    │ Registry             │
                    │ Events               │
                    └──────────┬───────────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
              ▼                ▼                ▼
         Frontend          Backend          Firmware
              │                │                │
              ▼                ▼                ▼
          Modules          Modules          Modules
              │                │                │
              ▼                ▼                ▼
           Blocks            API           Drivers
              │                │                │
              └────────────────┴────────────────┘
                               │
                         Capabilities
```

---

# 215. Firmware Architecture

```text
Core
│
├── Configuration
├── Module Manager
├── Capability Manager
├── Event Bus
├── Logging
├── Security
│
├── Hardware Modules
│   ├── GPIO
│   ├── I2C
│   ├── SPI
│   ├── RS485
│   └── Storage
│
├── Sensor Modules
├── Actuator Modules
├── Automation Modules
├── Network Modules
└── OTA Modules
```

---

# 216. Data Architecture

Los datos deben separar:

```text
Configuration
State
History
Events
Logs
Telemetry
```

---

# 217. Security Architecture

Separar:

```text
Authentication
Authorization
Permissions
Secrets
Audit
```

---

# 218. Modular Page Architecture

La arquitectura oficial de páginas será:

```text
Application
│
├── Core
│
├── Layout
│
├── Router
│
├── Module Registry
│
└── Pages
     │
     ├── Page
     │    │
     │    ├── Slot
     │    │    ├── Module Block
     │    │    ├── Module Block
     │    │    └── Module Block
     │    │
     │    └── Slot
     │         ├── Module Block
     │         └── Module Block
     │
     └── Page
```

La página no debe conocer necesariamente la implementación interna de cada bloque.

Debe conocer únicamente:

```text
slot
module
block
configuration
state
```

---

# 219. Instalable por Bloques

El estándar establece oficialmente:

> **Las páginas deben diseñarse como composiciones de bloques funcionales instalables y configurables.**

Esto significa que una aplicación puede comenzar con:

```text
Dashboard
 ├── Temperature
 └── Humidity
```

y posteriormente incorporar:

```text
+ CO2
+ Weather
+ Energy
+ Irrigation
+ Analytics
```

sin rediseñar estructuralmente el Dashboard.

---

# 220. Habilitación por Bloque

Cada bloque debe poder tener su propio estado:

```text
Installed
Enabled
Disabled
Error
```

Ejemplo:

```text
Dashboard

Temperature     ENABLED
Humidity        ENABLED
CO2             DISABLED
Weather         ENABLED
Energy          NOT INSTALLED
```

---

# 221. Instalación sin modificación del Core

Siempre que la plataforma lo permita, agregar un módulo debe seguir:

```text
Install Module
      ↓
Register Module
      ↓
Validate Dependencies
      ↓
Register Routes
      ↓
Register Permissions
      ↓
Register Blocks
      ↓
Enable
```

El Core debe permanecer sin modificaciones.

---

# 222. Desinstalación segura

El flujo debe ser:

```text
Disable
   ↓
Check Dependencies
   ↓
Stop Services
   ↓
Remove Routes
   ↓
Remove Blocks
   ↓
Remove Permissions
   ↓
Optional Data Removal
   ↓
Uninstall
```

---

# 223. Module Lifecycle

El ciclo de vida oficial será:

```text
AVAILABLE
    ↓
INSTALLED
    ↓
CONFIGURED
    ↓
ENABLED
    ↓
RUNNING
    ↓
DISABLED
    ↓
UNINSTALLED
```

Con estados de error:

```text
INSTALL_ERROR
CONFIG_ERROR
RUNTIME_ERROR
UPDATE_ERROR
```

---

# 224. Module Contract

Todo módulo importante debería implementar conceptualmente:

```text
install()
configure()
enable()
start()
stop()
disable()
update()
uninstall()
health()
```

No necesariamente con estos nombres exactos; el contrato depende de la tecnología.

---

# 225. Module Definition of Done

Un módulo se considera terminado cuando:

```text
[ ] Tiene ID único
[ ] Tiene versión
[ ] Tiene manifest
[ ] Declara dependencias
[ ] Declara capabilities
[ ] Declara permisos
[ ] Declara configuración
[ ] Puede habilitarse
[ ] Puede deshabilitarse
[ ] Puede diagnosticarse
[ ] Tiene tests
[ ] Tiene documentación
[ ] No rompe el Core
[ ] Tiene estrategia de actualización
[ ] Tiene estrategia de desinstalación
```

---

# 226. No Monolithic Pages

Queda establecido como regla del Design System:

> **No crear páginas monolíticas cuando la funcionalidad pueda separarse razonablemente en módulos o bloques.**

Esto no significa que absolutamente cada elemento visual deba convertirse en un plugin.

La modularidad debe aplicarse donde aporte:

* reutilización;
* independencia;
* mantenibilidad;
* configuración;
* extensibilidad;
* instalación posterior.

---

# 227. Modularidad sin sobreingeniería

La modularidad no debe convertirse en complejidad innecesaria.

No crear:

```text
1 módulo
para
1 botón
```

cuando no exista una razón arquitectónica.

La unidad mínima razonable es:

```text
Feature
Module
Block
Component
```

según el contexto.

---

# 228. Reference Project — Invernadero

El proyecto **Invernadero** será utilizado como proyecto de referencia para validar este estándar.

Debe evolucionar hacia:

```text
Core
│
├── Dashboard
│    ├── Temperature Block
│    ├── Humidity Block
│    ├── Soil Block
│    ├── Irrigation Block
│    └── Alarm Block
│
├── Sensors Module
├── Actuators Module
├── Automation Module
├── Weather Module
├── MQTT Module
├── Modbus Module
├── OTA Module
├── Diagnostics Module
└── Reports Module
```

No necesariamente todos deben instalarse en todas las implementaciones.

---

# 229. Evolution

El Design System debe evolucionar.

Cada cambio importante debe:

1. documentarse;
2. versionarse;
3. mantener compatibilidad cuando sea posible;
4. migrar proyectos existentes gradualmente.

---

# 230. Adoption

Los nuevos proyectos deben comenzar utilizando este estándar.

Los proyectos existentes pueden migrarse progresivamente.

No es necesario reescribir un proyecto completo únicamente para cumplir el estándar.

---

# 231. Official Standard

Este documento es la referencia principal para:

* diseño;
* UX;
* arquitectura;
* modularidad;
* estructura;
* documentación;
* desarrollo;
* seguridad;
* mantenimiento.

Cuando exista una contradicción entre una implementación y este estándar, debe documentarse la decisión mediante un ADR o documento equivalente.

---

# 232. Reference Project

**Invernadero** será el primer proyecto de referencia para validar:

* Design System;
* arquitectura modular;
* páginas por bloques;
* módulos instalables;
* Feature Flags;
* Capability System;
* frontend;
* backend;
* firmware;
* documentación.

---

# 233. Final Principle

> **Simple por fuera. Modular por dentro.**

Y específicamente para la arquitectura de interfaces:

> **Una página no es una implementación monolítica. Es una composición de bloques funcionales que pueden instalarse, configurarse, habilitarse, deshabilitarse y evolucionar independientemente.**

El objetivo final es que agregar una nueva capacidad signifique:

```text
Agregar módulo
      ↓
Registrar
      ↓
Configurar
      ↓
Habilitar
      ↓
Usar
```

y no:

```text
Modificar todo el sistema
      ↓
Modificar todas las páginas
      ↓
Modificar el Core
      ↓
Modificar navegación
      ↓
Modificar lógica existente
```

La arquitectura debe estar preparada desde el inicio para crecer de manera incremental.

---

# 234. Summary

El estándar completo se basa en:

```text
Design System
      │
      ├── Visual System
      ├── UX System
      ├── Component System
      │
      ├── Page System
      │     ├── Pages
      │     ├── Slots
      │     └── Blocks
      │
      ├── Module System
      │     ├── Install
      │     ├── Enable
      │     ├── Disable
      │     ├── Update
      │     └── Uninstall
      │
      ├── Capability System
      ├── Feature Flags
      ├── Permission System
      ├── Configuration System
      ├── API System
      ├── Data System
      ├── Firmware Architecture
      ├── Security
      ├── Testing
      ├── Documentation
      └── DevOps
```

### Regla fundamental

> **Todo proyecto debe poder comenzar siendo pequeño y crecer mediante módulos sin necesidad de convertirse en un monolito.**

### Regla específica para las páginas

> **Toda página compleja debe estar compuesta por bloques independientes, reutilizables y configurables, capaces de instalarse posteriormente y de habilitarse o deshabilitarse según las necesidades de cada instalación.**
