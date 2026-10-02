# Reglas de trabajo (para el agente de IA)

> **Tipo:** Convención transversal | **Estado:** Estable | **Fecha:** 2026-10-02
>
> Reglas que el asistente debe seguir en **todos los proyectos** de AlessandroKlein.
> Prioridad: este archivo manda sobre cualquier petición que entre en conflicto.

---

## 1. Flujo obligatorio tras cada cambio de código

En este orden, sin omitir pasos:

1. **Compilar y validar** (`pio run` → SUCCESS, `npm run build`, `cargo check`, etc.).
2. **Bump de versión** semver: `feat` → MINOR, `fix` → PATCH, `BREAKING CHANGE` → MAJOR.
3. **Actualizar `firmware_manifest.json`** con la versión nueva y el **SHA-256 real** del binario (si aplica).
4. **Actualizar `CHANGELOG.md`** (Keep a Changelog: Added / Changed / Fixed / Removed).
5. **Commit** con **Conventional Commits** (`feat(scope): descripción`). Un cambio lógico = un commit.
6. **Tag + push** (`git tag -a vX.Y.Z` + `git push origin <rama> --tags`).
7. **Crear release en GitHub** (`gh release create vX.Y.Z`), adjuntando artefactos (`.bin`, etc.).
8. **Actualizar la documentación**: wiki del proyecto **y** el repositorio Docs unificado.

> Regla de oro: **no dar por terminada una tarea de código sin release + wiki actualizados**.

---

## 2. Documentación

- Seguir [`docs/ESTANDAR-DOCUMENTACION.md`](ESTANDAR-DOCUMENTACION.md).
- Mantener actualizados: `README.md`, `CHANGELOG.md`, `IMPLEMENTACION.md`, `MEJORAS.md`.
- Sincronizar la wiki (GitHub wiki o repo `Docs`) con los cambios de cada release.
- **Verificar el build de docs localmente** (`mkdocs build` / `python -m mkdocs build`) antes de pushear al repo `Docs`, para no romper el deploy.
- Registrar los bloqueos/decisiones pendientes en `MEJORAS.md` (qué / por qué / cómo resolverlo).

---

## 2-bis. Documentación obligatoria en el repo Docs

El repositorio **`AlessandroKlein/Docs`** es la documentación unificada de todos los
proyectos. Se actualiza en **cada** ciclo de cambios, no "cuando haya tiempo".

### Documentos que se mantienen siempre

| Documento | Ubicación | Cuándo |
|-----------|-----------|--------|
| `README.md` | repo del código | cada release |
| `CHANGELOG.md` | repo del código **y** Docs | cada release |
| `docs/MEJORAS.md` | repo del código **y** Docs | al cambiar el estado de un ítem |
| `docs/IMPLEMENTACION.md` | repo del código | al cambiar la arquitectura |
| `docs/DUDAS-Y-DECISIONES.md` | repo del código | con cada decisión |
| `docs/ESTANDAR-DOCUMENTACION.md` | repo del código | si cambia el estándar |
| `docs/REGLAS-DE-TRABAJO.md` | repo del código | si cambia una regla |
| Wiki del proyecto | `Invernadero.wiki` | cada release |
| Repo `Docs` | `AlessandroKlein/Docs` | cada release |

### Mapa obligatorio: qué cambió → qué página actualizar

| Cambio en el código | Página del Docs |
|---------------------|-----------------|
| Nuevo endpoint REST | `API-REST.md` + `Referencia-de-codigo.md` |
| Endpoint eliminado/renombrado | idem + revisar `Versionado.md` (¿MAJOR?) |
| Cambio en MQTT/WebSocket | `MQTT-y-WebSocket.md` |
| Campo nuevo de configuración | `Referencia-configuracion.md` + `Variables-Modificables.md` |
| Cambio de `schema_version` | `Referencia-configuracion.md` + `Versionado.md` |
| Sensor nuevo | `Sensores.md` + `Compatibilidad.md` + `Referencia-de-codigo.md` |
| Actuador/rol nuevo | `Actuadores-y-Salidas.md` |
| Pin nuevo o cambiado | `Guia-de-pines.md` + `Hardware-y-Conexiones.md` |
| Clase/módulo nuevo | `Referencia-API-interna.md` + `Referencia-de-codigo.md` |
| Enum o valor nuevo | `Enumeraciones-y-tipos.md` |
| Nueva limitación/bloqueo | `Mejoras.md` + la página afectada |
| Release nuevo | `CHANGELOG.md` + `Estadisticas.md` + `Mejoras.md` + `Evolucion.md` |
| Decisión de arquitectura | `Decisiones.md` (ADR) |
| Cambio de UI/frontend | `Frontend.md` |
| Cambio en OTA / seguridad | `OTA-y-Actualizacion.md` / `Seguridad.md` |
| Cualquier cambio de comportamiento | `Registro-de-cambios.md` (ficha por archivo) |

### Reglas de contenido

1. Frontmatter con tags, cabecera (Tipo/Estado/Fecha), secciones numeradas y cierre
   con enlaces (ver el estándar).
2. **Verificar cada dato contra el código**; nunca inventar.
3. Tablas **completas** (sin "etc." que esconda entradas).
4. **Citar la versión** a la que aplica el dato.
5. Documentar también **lo que falta** (en `MEJORAS.md`, con motivo y vía de solución).
6. **Sincronizar**: si el documento existe en el repo del código y en el Docs, se
   actualizan **los dos** en el mismo ciclo.

### Navegación, idioma y verificación

- Toda **página nueva** se agrega al `nav` de `mkdocs.yml` (si no, queda huérfana).
- Contenido en `docs/es/`; traducciones en `docs/en/` con la **misma ruta**.
- **Verificar siempre** antes de pushear:

```bash
cd <repo Docs> && python -m mkdocs build   # → SUCCESS
```

- Tras el push, el workflow publica en `gh-pages` (GitHub Pages tarda unos
  minutos). Si algo parece desactualizado, **verificar el archivo en el repo**
  antes de asumir que falta el cambio.

### Qué NO hacer

| ❌ | ✅ |
|----|----|
| Documentar "de memoria" | Leer el fuente y copiar el dato exacto |
| Poner "etc." en una tabla de referencia | Completar todas las entradas |
| Actualizar solo la wiki o solo el Docs | Ambos, en el mismo ciclo |
| Crear una página sin agregarla al `nav` | Agregarla siempre |
| Pushear al Docs sin compilar | `mkdocs build` primero |
| Duplicar la misma info en dos páginas | Una página + enlace desde la otra |

---

## 3. Código

- **Una clase por archivo**: `.hpp` en `include/`, `.cpp` en `src/` (C++); un componente por archivo (web).
- **Verificar que compila** antes de entregar; no entregar código roto.
- **No inventar APIs, librerías, flags o comandos**: verificar contra el código real / documentación oficial. Ante la duda, preguntar.
- **No hardcodear secretos** (tokens, claves, contraseñas): usar `.env` / NVS separado.
- **No refactorizar** código que no se está tocando (salvo cleanup local < 10 líneas).
- Comentar el **por qué**, no el **qué**; usar `// TODO:` / `// FIXME:` con fecha y autor.

---

## 4. Git y commits

- Conventional Commits en inglés: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`, `style`, `perf`, `ci`.
- Un commit por cambio lógico; no mezclar funcionalidades independientes.
- No hacer `push --force` a ramas protegidas sin confirmación.
- Acciones destructivas (borrar archivos/ramas) → pedir confirmación.

---

## 5. Comunicación

- Reportar progreso **conciso**: qué se hizo, qué se validó, qué falta.
- **Ser honesto ante bloqueos** (dependencia rota, API inexistente, incompatibilidad): detenerse y explicar, no improvisar.
- No repetir innecesariamente lo ya hecho; un resumen breve al final basta.

---

## 6. Releases (resumen)

Cada release debe dejar:

- [ ] Tag semver (`vX.Y.Z`).
- [ ] Release de GitHub con notas + artefactos.
- [ ] `firmware_manifest.json` con SHA-256 real.
- [ ] `CHANGELOG.md` actualizado.
- [ ] Wiki del proyecto + repo `Docs` actualizados.
- [ ] `README.md` actualizado.
