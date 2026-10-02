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
