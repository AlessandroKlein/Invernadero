#pragma once
// Versiones del firmware, hardware y esquema de configuración (secciones 50/147).
// Centralizadas para que el manifest OTA, la API y el arranque sean consistentes.

#define GH_FW_VERSION            "3.28.0"
#define GH_HW_VERSION            "rev0"
#define GH_HW_PROFILE            "ESP32-GH-V1"
#define GH_CONFIG_SCHEMA_VERSION 2
#define GH_PROTOCOL_VERSION      1

// Bloqueo del mapa de pines para PCBs fabricadas (README §205):
//  - 0 = público: los usuarios pueden editar los pines desde la web.
//  - 1 = PCB fija: los pines quedan bloqueados (PUT /api/v1/pins → 403),
//        pero el catálogo de sensores/actuadores sigue configurable.
#define GH_PINS_LOCKED           0
