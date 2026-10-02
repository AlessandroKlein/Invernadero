# Estándar de documentación y wiki

> **Tipo:** Convención transversal | **Estado:** Estable | **Fecha:** 2026-10-02
>
> Estándar reutilizable para documentar **cualquier proyecto** de forma profunda y
> uniforme: arquitectura, decisiones, conexiones eléctricas/electrónicas, registro
> de modificaciones de archivos y publicación de la wiki. Aplicable a proyectos
> nuevos y existentes de AlessandroKlein.

---

## 1. Objetivo

Que cualquier persona (técnica o no) pueda entender un proyecto completo sin leer
el código: qué hace, cómo está construido, por qué se tomaron ciertas decisiones,
cómo se conecta cada componente y qué cambió en cada archivo y por qué.

Para lograrlo se definen dos cosas:

1. **La estructura de la wiki** (qué páginas existen y qué contiene cada una).
2. **Los formatos de registro** (conexiones, decisiones, modificaciones de archivos).

---

## 2. Estructura estándar de la wiki

Toda wiki debe tener, como mínimo, estas páginas (los nombres pueden adaptarse):

| Página | Contenido | Obligatoria |
|--------|-----------|:-----------:|
| `Home` | Resumen ejecutivo, qué es, características, acceso rápido, índice | ✅ |
| `Arquitectura` | Diagrama de capas/módulos, principios, estructura de carpetas | ✅ |
| `Hardware-y-Conexiones` | Mapa de pines + **conexiones detalladas** (ver §4) | ✅ (si aplica) |
| `Decisiones` (ADR) | Decisiones de arquitectura con contexto/consecuencias | ✅ |
| `Registro-de-cambios` | Historial de modificaciones de archivos (ver §5) | ✅ |
| `Configuracion` | Variables, archivos `.env`, parámetros editables | ✅ |
| `API` / `Endpoints` | Rutas, métodos, payloads, códigos de respuesta | si aplica |
| `Compilacion` / `Deploy` | Cómo compilar, flashear, desplegar, ejecutar | ✅ |
| `Glosario` | Términos en lenguaje simple | recomendado |
| `Evolucion` / `Roadmap` | Hitos, estado actual (✅/❌), plan | ✅ |
| `Mejoras` | Mejoras identificadas y su estado | recomendado |

> Regla de oro: **una página por tema**, no un único documento gigante. Cada página
> comienza con una cabecera `> **Tipo:** … | **Estado:** … | **Fecha:** YYYY-MM-DD`.

---

## 3. Cabecera y plantilla de página

Toda página comienza con la misma cabecera:

```markdown
# [Nombre de la página]

> **Tipo:** Embebidos | API | Web | Móvil | Backend | Referencia | Roadmap
> **Estado:** Estable | En desarrollo | Planificación
> **Fecha:** YYYY-MM-DD
```

Luego secciones numeradas (`## 1.`, `## 2.`, …). Los diagramas se escriben en
bloques `text` (ASCII) o Mermaid (si la plataforma lo soporta).

---

## 4. Documentación de conexiones (hardware / electrónica)

Cada conexión se documenta de forma **explícita** para que sea reproducible sin
adivinar. No alcanza con "conectar el sensor": hay que indicar resistencias,
capacitores, tensiones y pull-ups.

### 4.1 Tabla de pines (resumen)

```markdown
| Función | Pin | Tipo | Tensión | Notas |
|---------|-----|------|---------|-------|
| I²C SDA | 21 | bidireccional | 3,3 V | pull-up 4,7 kΩ |
| 1-Wire (DS18B20) | 4 | datos | 3,3 V | pull-up 4,7 kΩ a 3,3 V |
```

### 4.2 Ficha de conexión (por componente)

Para cada componente (sensor, módulo, driver) se documenta una ficha completa:

```markdown
### SHT31 (temperatura y humedad)

| Elemento | Valor / Detalle |
|----------|-----------------|
| Interfaz | I²C, dirección 0x44 |
| Tensión | 2,4–5,5 V (alimentado a 3,3 V) |
| Resistencia | pull-up de 4,7 kΩ en SDA y SCL (a 3,3 V) |
| Capacitor | 100 nF cerámico entre VDD y GND (desacople) |
| Conexión | VDD→3,3 V · GND→GND · SDA→GPIO21 · SCL→GPIO22 |
| Notas | No requiere calibración; ±0,2 °C / ±2 %RH |
```

### 4.3 Lista de verificación obligatoria

Cada ficha debe responder:

- [ ] ¿Qué interfaz usa (I²C/SPI/UART/ADC/GPIO/1-Wire/RS485/CAN)?
- [ ] ¿Qué pines/dirección ocupa?
- [ ] ¿Qué resistencias necesita y con qué valor (pull-up, divisor, limitadora)?
- [ ] ¿Qué capacitores necesita (desacople, filtro) y con qué valor?
- [ ] ¿Qué tensión de alimentación y qué niveles lógicos?
- [ ] ¿Qué protecciones (flyback, optoacoplador, fusible, aislamiento)?
- [ ] ¿Hay algún requisito de arranque/estado seguro?

---

## 5. Registro de modificaciones de archivos

Complementa al `CHANGELOG` (que resume **versiones**). El registro documenta
**por archivo** el *por qué* y el *cómo* de cada cambio, para trazabilidad total.

### 5.1 Ubicación y nombre

- Archivo: `docs/REGISTRO-CAMBIOS.md` (o `CHANGELOG-ARCHIVOS.md`).
- Se mantiene al día en **cada commit** que modifique comportamiento.

### 5.2 Formato de cada entrada

```markdown
## YYYY-MM-DD — Resumen breve

### `ruta/al/archivo.ext`
- **Qué:** descripción del cambio (qué se modificó).
- **Por qué:** motivo/contexto (bug, requisito, decisión, refactor).
- **Cómo:** técnica aplicada (algoritmo, patrón, librería, API usada).
- **Impacto:** qué otros módulos/archivos se ven afectados.
- **Referencia:** issue #N / commit `abc1234` / ADR-0007.
```

### 5.3 Ejemplo real

```markdown
## 2026-10-02 — Separar adquisición y control en FreeRTOS

### `src/main.cpp`
- **Qué:** se dividió `automationTask` en `sensorTask` + `controlTask`.
- **Por qué:** el principio de no bloqueo (SEMA §202-203); la comunicación no debe
  bloquear la adquisición y el control debe usar lecturas frescas.
- **Cómo:** dos tareas en el núcleo 0 con semáforo `sensorReady`; prioridad 3 (sensores)
  y 2 (control); ambas suscritas al watchdog.
- **Impacto:** `include/sensors/SensorManager.hpp` (getters protegidos por mutex).
- **Referencia:** ADR-0004, commit `1b197fc`.

### `include/sensors/SensorManager.hpp`
- **Qué:** se añadió `valueAt()` y se protegieron los getters.
- **Por qué:** evitar carrera de datos entre `sensorTask` (escribe) y `controlTask` (lee).
- **Cómo:** `xSemaphoreTake/Give` alrededor de la lectura de `values_[]`.
- **Impacto:** sin cambio de firma en los controladores.
```

---

## 6. Decisiones de arquitectura (ADR)

Las decisiones importantes se registran como **ADR** (Architecture Decision Record)
en `docs/adr/NNNN-titulo-corto.md`, con este formato:

```markdown
# NNNN. Título de la decisión

> **Estado:** Propuesta | Aceptada | Reemplazada | Obsoleta
> **Fecha:** YYYY-MM-DD

## Contexto
¿Cuál es el problema o la necesidad?

## Decisión
¿Qué se decidió y por qué?

## Consecuencias
¿Qué gana, qué cuesta, qué queda pendiente?
```

Numeración secuencial (`0001`, `0002`, …). Las decisiones vigentes se resumen en
la página wiki `Decisiones`.

---

## 7. Cómo publicar la wiki (y cómo lo hacen las empresas)

Hay tres modelos, de menor a mayor escala:

### 7.1 Wiki nativa de GitHub (por repositorio)

GitHub ofrece una **wiki por repositorio**, que internamente es otro repositorio git
(`https://github.com/USER/REPO.wiki.git`). Se edita desde la web o clonándola.

- ✅ Rápido, integrado, sin herramientas extra.
- ❌ **No se comparte entre repositorios**: cada proyecto tiene su propia wiki.
- ❌ Búsqueda y versionado limitados; difícil reutilizar plantillas.

> Es el modelo que usa hoy **Invernadero**.

### 7.2 Docs-as-code (lo que usan las empresas)

Las empresas grandes no editan wikis a mano: tratan la **documentación como código**.
Markdown en un repositorio git, renderizado por un generador de sitios estáticos:

| Herramienta | Para qué |
|-------------|----------|
| **MkDocs** (+ Material) | Documentación técnica en Markdown, muy usado en Python/empresas |
| **Docusaurus** | Docs de proyectos JS/frontend (React), muy usado por Meta |
| **mdBook** | Libros/documentación en Rust |
| **GitBook / ReadMe** | SaaS de documentación |
| **Confluence / Notion** | Wikis internas corporativas (no basadas en git) |

Flujo típico: `escribir Markdown → commit/pull request → CI compila → publica en
GitHub Pages / ReadTheDocs`. Esto da versionado, revisión por PR y reutilización.

### 7.3 Repositorio de documentación compartido (carpetas por proyecto) ← recomendado

**Sí, es posible y es la mejor opción** para compartir una misma wiki con datos por
proyecto tipo carpeta. Se crea **un único repositorio de documentación**, por ejemplo
`AlessandroKlein/wiki` o `AlessandroKlein/docs`, con esta estructura:

```text
docs/                       ← repositorio único
├── mkdocs.yml              ← configuración del sitio
├── docs/
│   ├── index.md            ← índice global
│   ├── invernadero/        ← carpeta por proyecto
│   │   ├── Home.md
│   │   ├── Arquitectura.md
│   │   ├── Hardware-y-Conexiones.md
│   │   ├── Registro-de-cambios.md
│   │   └── Decisiones.md
│   ├── sema/
│   │   ├── Home.md
│   │   └── ...
│   └── otro-proyecto/
│       └── ...
└── .github/workflows/      ← CI que publica en GitHub Pages
```

Con **MkDocs Material** se obtiene una wiki unificada con búsqueda global, tema
claro/oscuro y navegación por proyecto, todo versionado en git. Se publica
automáticamente en **GitHub Pages** con un workflow de GitHub Actions.

**Cómo montarlo (resumen):**

```bash
# 1. Repositorio único de documentación
git init wiki
cd wiki
pip install mkdocs-material
mkdocs new .                     # genera mkdocs.yml + docs/
# 2. Configurar navegación por carpeta en mkdocs.yml (nav)
# 3. Escribir cada proyecto en docs/<proyecto>/
# 4. CI (GitHub Actions) → mkdocs build → deploy a GitHub Pages
```

---

## 8. Plantillas listas para copiar

### 8.1 Página Home

```markdown
# [Proyecto]

> **Tipo:** … | **Estado:** … | **Fecha:** YYYY-MM-DD | **Versión:** x.y.z

## ¿Qué es?
…resumen en una frase…

## Características
- …

## Acceso rápido
| Servicio | Dirección |
|----------|-----------|
| … | … |

## Índice
- [Arquitectura](Arquitectura)
- [Hardware y conexiones](Hardware-y-Conexiones)
- [Registro de cambios](Registro-de-cambios)
```

### 8.2 Ficha de conexión (copiar por componente)

```markdown
### [Componente]

| Elemento | Valor / Detalle |
|----------|-----------------|
| Interfaz | … |
| Tensión | … |
| Resistencia | … |
| Capacitor | … |
| Conexión | … |
| Notas | … |
```

### 8.3 Entrada del registro de cambios

```markdown
### `ruta/al/archivo`
- **Qué:** …
- **Por qué:** …
- **Cómo:** …
- **Impacto:** …
- **Referencia:** …
```

---

## 9. Checklist final de una wiki completa

- [ ] `Home` con resumen y acceso rápido.
- [ ] `Arquitectura` con diagrama y estructura de carpetas.
- [ ] `Hardware-y-Conexiones` con **cada** componente y sus resistencias/capacitores.
- [ ] `Decisiones` (ADRs) con contexto y consecuencias.
- [ ] `Registro-de-cambios` por archivo (qué/por qué/cómo/impacto).
- [ ] `Compilacion`/`Deploy` reproducible.
- [ ] `Glosario` en lenguaje simple.
- [ ] `Evolucion` con estado actual (✅/❌).
- [ ] Publicada (GitHub wiki o MkDocs/Docusaurus) y actualizada en cada commit.
