# Contribución

## Ramas

- `main`: rama estable. Los cambios se integran mediante Pull Request.

## Estándar de commits

Se sigue [Conventional Commits](https://www.conventionalcommits.org/):

- `feat:` nuevas funcionalidades.
- `fix:` corrección de errores.
- `docs:` cambios de documentación.
- `refactor:` mejoras de código sin cambio de comportamiento.
- `chore:` tareas de mantenimiento o CI/CD.

## Construcción

```bash
pio run            # compila (firmware.bin en .pio/build/<env>/)
pio run -t upload  # graba por puerto serie
pio device monitor # monitor serie (115200)
```

## Estilo

- Un módulo por archivo (`.hpp` en `include/`, `.cpp` en `src/`).
- Comentarios explicativos en bloques clave (Doxygen).
- Separación de capas: config / sensors / actuators / control / network / api / storage / system / core.

## Versionado y release

- Seguir SemVer (`vMAJOR.MINOR.PATCH`).
- Mantener `firmware_manifest.json` (SHA-256 y versión) sincronizado con el binario liberado.
- Actualizar `CHANGELOG.md` en cada versión.
