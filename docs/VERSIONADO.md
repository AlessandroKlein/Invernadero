# Versionado (numeración de versiones)

> **Tipo:** Convención transversal | **Estado:** Estable | **Fecha:** 2026-10-03
>
> Convención de numeración de versiones para todos los proyectos. Copiar a
> `docs/VERSIONADO.md` al iniciar un proyecto.

---
# Versionado (cómo numerar las versiones)

> **Tipo:** Convención | **Estado:** Estable | **Fecha:** 2026-10-02

Guía completa de la nomenclatura de versiones del ecosistema: firmware, servidor,
frontend, hardware, configuración y las **fases del proyecto** (`V8`, `V9`).

---

## 1. Regla base: Semantic Versioning 2.0.0

Toda versión del proyecto sigue **SemVer**:

```text
MAJOR.MINOR.PATCH[-preRelease][+buildMeta]

  3   .   29   .   0    -beta.1     +build.7
  │        │        │       │            └── metadatos de build (no afectan orden)
  │        │        │       └── pre-release: alpha / beta / rc
  │        │        └── PATCH: corrección de errores
  │        └── MINOR: funcionalidad nueva (retrocompatible)
  └── MAJOR: cambio incompatible
```

**Se incrementa solo el número que corresponde; los de la derecha vuelven a 0.**

```text
3.28.5 + feat  → 3.29.0   (MINOR sube, PATCH vuelve a 0)
3.28.5 + fix   → 3.28.6   (PATCH sube)
3.28.5 + break → 4.0.0    (MAJOR sube, el resto vuelve a 0)
```

## 2. El prefijo `v` — regla clave

| Contexto | Formato | Ejemplo |
|----------|---------|---------|
| **Etiqueta (tag) de Git** | `v` + versión | `v3.29.0` |
| **Release de GitHub** | `v` + versión | `v3.29.0` |
| **Variable del firmware** | **sin** `v` | `GH_FW_VERSION "3.29.0"` |
| **Manifest JSON** | **sin** `v` | `"version": "3.29.0"` |
| **CHANGELOG** | `[` versión `]` sin `v` | `## [3.29.0]` |
| **README / docs** | se usa `v` al hablar del release | "Avance v3.29.0" |

!!! warning "Siempre minúscula"
    La convención adoptada es **`v` minúscula**. Históricamente se usó `V`
    mayúscula (`V1.16.0`, `V2.30`) y por eso hay etiquetas mezcladas (ver §9).
    Los tags nuevos van **siempre en minúscula**.

## 3. ¿Qué número toca? (tabla de decisión)

| Cambio | Número | Ejemplos del proyecto |
|--------|:------:|----------------------|
| Rompe compatibilidad (config, API, protocolo) | **MAJOR** | quitar/renombrar un endpoint, cambiar tópicos MQTT → **ver «¿Cuándo pasar a MAJOR?» abajo** |
| Funcionalidad nueva retrocompatible | **MINOR** | `feat(...)`: nuevo endpoint, nuevo pool, nueva página web |
| Corrección de errores | **PATCH** | `fix(...)`: bug de lectura, timeout mal calculado |
| Solo documentación / comentarios | **ninguno** | un commit `docs:` no genera release |
| Refactor sin cambio de comportamiento | **ninguno** o PATCH | depende de si el usuario nota algo |

### Cuándo NO bumpear

- Cambios solo en `docs/`, comentarios, README sin cambio funcional.
- Reordenar código, renombrar variables internas, formateo.
- Subir la versión solo porque hubo un commit.

### ¿Cuándo es recomendable pasar a MAJOR? (ej. `3.29.0` → `4.0.0`)

!!! warning "MAJOR no significa «versión importante»"
    **MAJOR no es** "hice mucho trabajo", "tardé meses", "es un hito", "cambié el
    PCB" ni "reescribí el firmware". Significa **una sola cosa**: algo que ya
    funcionaba **dejó de funcionar** para quien depende del firmware.
    **Si nadie tiene que cambiar nada, no es MAJOR.**

#### 1. ¿Quién "depende" del firmware?

Antes de decidir, preguntate **quién se rompe** con el cambio:

| Dependiente | Qué usa del firmware |
|-------------|----------------------|
| **El servidor central** | API REST, tópicos MQTT, JSON de telemetría |
| **Las instalaciones ya flasheadas** | Su configuración guardada (NVS/JSON) |
| **El usuario** | Sus pines/cableado, reglas, umbrales, calibración |
| **Integraciones externas** | Endpoints REST, MQTT, WebSocket |

#### 2. Tabla de decisión

| Cambio | ¿MAJOR? | Por qué |
|--------|:-------:|---------|
| Agregar un endpoint nuevo | ❌ MINOR | nadie se rompe |
| Agregar un campo al JSON de estado | ❌ MINOR | quien lee sigue funcionando |
| **Quitar o renombrar** un endpoint | ✅ MAJOR | el servidor deja de encontrarlo |
| **Cambiar el significado** de un campo existente | ✅ MAJOR | errores silenciosos |
| Cambiar los tópicos MQTT (`greenhouse/...` → otro) | ✅ MAJOR | el servidor deja de recibir |
| Cambiar el formato de telemetría sin compatibilidad | ✅ MAJOR | el servidor no parsea |
| Cambiar el formato de config **sin** migración | ✅ MAJOR | el usuario debe reconfigurar a mano |
| Cambiar el formato de config **con** migración | ❌ MINOR | `migrate()` lo resuelve solo |
| Cambiar pines por defecto (obligando a recablear) | ✅ MAJOR | instalaciones existentes quedan mal |
| Agregar un sensor/expansor soportado | ❌ MINOR | es opcional para el usuario |
| Reordenar/limpiar código interno | ❌ ninguno | el usuario no lo ve |
| Reescribir un módulo por dentro (misma interfaz) | ❌ ninguno/PATCH | la API externa no cambia |

#### 3. Lo que NO justifica un MAJOR

| Lo que se suele pensar | ¿MAJOR? | Qué usar en su lugar |
|------------------------|:-------:|----------------------|
| "Hice muchos cambios" | ❌ | igual sigue MINOR (`3.29.0` → `3.30.0`) |
| "Tardé meses / es un hito" | ❌ | la **fase** del proyecto (`V10`) |
| "Cambié el PCB / hardware nuevo" | ❌ | `GH_HW_VERSION` (`rev1`) + **perfil** (`ESP32-GH-V2`) |
| "Es una generación nueva del invernadero" | ❌ | perfil de hardware + documentación |
| "Reescribí el firmware de cero" | ❌ | si API y config siguen iguales, no es MAJOR |
| "Cambié cómo se configura, pero migra solo" | ❌ | `schema_version++` + `migrate()` |
| "Arreglé muchos bugs" | ❌ | PATCH |
| "Cambié los valores por defecto" (sin romper) | ❌ | MINOR o ninguno |

#### 4. Cómo EVITAR tener que subir MAJOR (recomendado)

En lugar de romper y saltar a `4.0.0`, el proyecto ya tiene mecanismos para no
romper:

1. **Versionar la API**: en vez de cambiar `/api/v1/`, agregar `/api/v2/` y
   mantener `v1` funcionando un tiempo.
2. **Migrar la config automáticamente**: subir `GH_CONFIG_SCHEMA_VERSION` y
   agregar la conversión en `ConfigManager::migrate()`.
3. **Agregar, no quitar**: campos nuevos con default; endpoints nuevos; publicar
   en el tópico viejo y el nuevo durante la transición.
4. **Subir `GH_PROTOCOL_VERSION`** cuando cambia el diálogo con el servidor.
5. **Mantener alias**: si renombrás una clave, aceptar la anterior y traducirla.

> Con estas prácticas el proyecto puede crecer **sin MAJOR** durante mucho tiempo:
> hoy va por `3.29.0` y **todas** las `3.x` son compatibles entre sí.

#### 5. Casos concretos que SÍ serían `4.0.0`

- Mover el servidor a `/api/v2/` y **eliminar** `/api/v1/` sin convivencia.
- Cambiar los tópicos de `greenhouse/{device_id}/...` **sin** publicar en ambos.
- Romper el `firmware_manifest.json` (campos que el servidor necesita).
- Dejar de soportar `ESP32-GH-V1` (instalaciones que ya no podrían actualizar).
- **Eliminar** el mapa de pines sin reemplazo, obligando a recablear.

#### 6. Firmware (`3.x`) vs servidor (`0.x`)

| Artefacto | Versión | Qué significa |
|-----------|---------|---------------|
| Firmware | `3.29.0` | **estable**: MAJOR = rompe compatibilidad |
| Servidor | `0.5.0` | **desarrollo inicial**: mientras MAJOR sea `0`, la API puede cambiar sin aviso |
| Frontend | `1.1.0` | versionado aparte |

Cuando el servidor llegue a **`1.0.0`** empieza a aplicar la misma regla que el
firmware.

#### 7. Resumen en una frase

> **¿Alguien (el servidor, otra instalación o el usuario) tiene que hacer algo a
> mano para que siga funcionando?**
> **Sí → MAJOR. No → MINOR o PATCH.**

## 4. Pre-releases: `alpha`, `beta` (`b`) y `rc`

Cuando la versión **no está lista para uso general**:

| Tipo | Notación | Uso |
|------|----------|-----|
| Alfa | `3.30.0-alpha.1` | Experimental, puede romper |
| Beta | `3.30.0-beta.1` | Funcionalmente completa, en pruebas |
| Beta corta | `3.30.0-b1` | Abreviatura aceptada de `beta.1` |
| Release candidate | `3.30.0-rc.1` | Candidata final, solo correcciones |

**Orden de precedencia** (de menor a mayor):

```text
3.30.0-alpha.1 < 3.30.0-alpha.2 < 3.30.0-beta.1 < 3.30.0-rc.1 < 3.30.0
```

> Una pre-release **siempre es menor** que su versión final. `3.30.0-beta.1` < `3.30.0`.

### La `b` en este proyecto

La `b` se interpreta como **beta** (`3.30.0-b1` = `3.30.0-beta.1`). En la industria
también se usa `b` como *build* (`b7`), pero **en este proyecto `b` = beta**; para
build se usa el metadato `+build.N` (§5).

## 5. Metadatos de build (`+`)

No cambian el orden de versiones; sirven para trazabilidad:

```text
3.30.0+build.42
3.30.0+sha.1a2b3c4
3.30.0-beta.1+sha.1a2b3c4
```

El **SHA-256 del binario** no va en la versión: va en `firmware_manifest.json`
(campo `sha256`) y en las notas del release.

## 6. Canales de actualización

El firmware tiene un canal (`update_channel`), que se combina con el tipo de versión:

| Canal | Acepta | Uso |
|-------|--------|-----|
| `stable` | `X.Y.Z` | Producción |
| `beta` | `X.Y.Z` y `-beta.N` | Pruebas |
| `development` | todo, incluido `-alpha.N` | Desarrollo |

```text
stable     ← solo versiones finales
beta       ← finales + betas
development ← todo
```

## 7. Otras versiones del proyecto (¡no confundir!)

El firmware lleva **cuatro** números distintos. No son intercambiables:

| Constante | Ejemplo | Qué versiona | Cuándo sube |
|-----------|---------|--------------|-------------|
| `GH_FW_VERSION` | `3.29.0` | Firmware | Cada release |
| `GH_HW_VERSION` | `rev0` | Revisión del hardware | Cambio de placa |
| `GH_CONFIG_SCHEMA_VERSION` | `2` | Estructura del JSON de config | Cambio incompatible de config |
| `GH_PROTOCOL_VERSION` | `1` | Protocolo con el servidor | Cambio incompatible de protocolo |

### Versión de hardware: `revN`

```text
rev0  → primera revisión del PCB
rev1  → corrección de una pista / componente
rev2  → rediseño mayor
```

Los enteros **no** vuelven a cero: es acumulativa.

### Esquema de configuración: entero incremental

```text
schema 1 → estructura original
schema 2 → configuración por capas / net_interface / eth_*   (subida en v3.14.0)
schema 3 → (cuando haya un cambio incompatible de config)
```

Al subir el esquema hay que agregar la migración en
`ConfigManager::migrate()`.

## 8. Versiones por subsistema (prefijos)

Cada artefacto del ecosistema se versiona **de forma independiente** con un prefijo:

| Artefacto | Etiqueta | Ejemplos reales |
|-----------|----------|-----------------|
| Firmware del invernadero | `vX.Y.Z` | `v3.29.0` |
| Servidor central | `server-vX.Y.Z` | `server-v0.1.0` … `server-v0.5.0` |
| Frontend (preview) | `frontend-preview-vX.Y.Z` | `frontend-preview-v1.0.0` … `v1.1.0` |
| Documentación | `vX.Y.Z` (repo Docs) | `v1.0.0` |

> Así el servidor puede ir en `0.x` (inestable) mientras el firmware va en `3.x`,
> sin confundir al usuario sobre cuál se actualizó.

### `0.y.z` = desarrollo inicial

Mientras el número **MAJOR sea `0`**, la API puede cambiar en cualquier momento
(es lo que indica SemVer). El servidor está en `0.5.0`; el firmware ya en `3.x`
(estable).

## 9. Las fases del proyecto (`V8`, `V8.1`, `V9`) NO son versiones

Cuidado con la ambigüedad: el proyecto tiene **fases de arquitectura** en
mayúscula, que **no** son versiones de firmware:

| Fase | Significado | Versión de firmware donde se entregó |
|------|-------------|--------------------------------------|
| `V8` | Base de la plataforma configurable (registros, buses) | `3.8.0` |
| `V8.1` | SpiManager, 74HC165, MCP23S17, ADC | `3.10.0` |
| `V8.4` | StorageManager (LittleFS/SPIFFS/SD) | `3.10.0` |
| `V9` | Perfiles Modbus, capa CAN, gateway RS485 | `3.10.0` / `3.17.0` |
| `V10` | (futuro) | — |

!!! tip "Cómo distinguirlas"
    - **Mayúscula + sin prefijo `v`** → fase del proyecto (`V8`, `V9`).
    - **Minúscula + prefijo `v`** → etiqueta de release (`v3.29.0`).

## 10. Historial real de etiquetas (y sus inconsistencias)

Las etiquetas existentes en el repositorio, para referencia histórica:

| Etiqueta | Observación |
|----------|-------------|
| `V1.3.0`, `V1.4.0`, `V1.16.0` | **Mayúscula**; convención antigua |
| `V2.00`, `V2.30` | Mayúscula y con **cero relleno** (`V2.00`) — no es SemVer |
| `v3.0.0` … `v3.29.0` | Convención actual (minúscula, SemVer) |
| **`v3.8.0` faltante** | El CHANGELOG tiene la entrada 3.8.0 pero **no se creó el tag** |
| `server-v0.x`, `frontend-preview-v1.x` | Versionado por subsistema |

**Reglas adoptadas a partir de ahora:**

- ✅ Siempre `v` **minúscula** y SemVer completo (`v3.30.0`).
- ❌ No usar cero relleno (`2.00` → `2.0.0`).
- ❌ No saltear números: si existe la entrada en el CHANGELOG, **crear el tag**.
- ✅ Prefijo de subsistema solo para artefactos distintos (`server-v…`).

## 11. Flujo de release (paso a paso)

```text
1. Decidir MAJOR/MINOR/PATCH según §3
2. Editar include/core/Version.hpp  →  #define GH_FW_VERSION "X.Y.Z"
3. Actualizar CHANGELOG.md          →  ## [X.Y.Z] - AAAA-MM-DD
4. pio run                           →  verificar SUCCESS
5. Calcular SHA-256 del firmware.bin →  firmware_manifest.json
6. git commit -m "feat(...): ..."    →  Conventional Commits
7. git tag -a vX.Y.Z -m "Release vX.Y.Z: <resumen>"
8. git push origin main --tags
9. gh release create vX.Y.Z --title "vX.Y.Z — <título>" --notes "..." firmware.bin
10. Actualizar wiki del proyecto + repo Docs
```

## 12. Ejemplos prácticos

| Situación | Versión |
|-----------|---------|
| Se agregó el endpoint `/api/v1/modbus/gateway` | `3.17.0` (MINOR) |
| Se corrigió que el OTA no cerrara el socket | `3.17.1` (PATCH) |
| Se agregaron 10 features nuevas | `3.30.0` (MINOR — la cantidad no importa) |
| Se cambió el PCB a la revisión 2 | `3.29.0` + `rev1` (no cambia la versión de firmware) |
| Se migró la config automáticamente a schema 3 | `3.30.0` (MINOR — migra solo) |
| Se cambió el formato del JSON **sin** migración | `4.0.0` (MAJOR) |
| Se eliminó `/api/v1/` sin dejar `/api/v2/` | `4.0.0` (MAJOR) |
| Se reescribió todo el firmware, misma API | `3.30.0` (MINOR — nadie se rompe) |
| Beta del próximo release con nueva API | `3.30.0-beta.1` |
| Candidata final de esa beta | `3.30.0-rc.1` |
| Release final | `3.30.0` |
| Se actualizó solo la documentación | sin release |

## 13. Glosario de notación

| Símbolo | Significado |
|---------|-------------|
| `X.Y.Z` | `MAJOR.MINOR.PATCH` |
| `v` | Prefijo de etiqueta de Git/release (minúscula) |
| `V8`, `V9` | Fase del proyecto (mayúscula, no es versión) |
| `-alpha.N` | Pre-release experimental |
| `-beta.N` / `-bN` | Pre-release en pruebas |
| `-rc.N` | Release candidate |
| `+build.N`, `+sha.xxxx` | Metadatos de build (no afectan el orden) |
| `revN` | Revisión de hardware |
| `schema N` | Versión del esquema de configuración |
| `0.y.z` | Desarrollo inicial (API inestable) |

Ver también: [Reglas de trabajo](Reglas-de-trabajo.md) ·
[CHANGELOG](invernadero/CHANGELOG.md) · [Mejoras](invernadero/Mejoras.md) ·
[Evolución](invernadero/Evolucion.md).
