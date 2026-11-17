# HKChecker

[🇪🇸 Versión en español](README.es.md)

> **Hollow Knight Completion Checker for Nintendo Switch** — Analiza tus partidas y rastrea tu progreso hacia el 112% del juego.

---

## 🎮 Overview

**HKChecker** es una aplicación homebrew para Nintendo Switch que lee archivos de guardado de *Hollow Knight* (directamente del almacenamiento de la consola, respaldos de JKSV o archivos `.dat`) de forma totalmente offline. Es una adaptación de la herramienta web [Hollow Knight Completion Check](https://github.io) de [ReznoRMichael](https://github.com).

### ✨ Características principales
- **8 pestañas completas**: `MAIN`, `ESSENTIALS`, `JOURNAL`, `COLLECTIBLES`, `GEO CACHES`, `SECRETS`, `GODMASTER` y `STATISTICS`.
- **Detección automática** de los 4 espacios de guardado (`user1.dat` – `user4.dat`).
- **Interfaz bilingüe** (Inglés / Español) con selector rápido (`-`) y **sistema anti-spoilers** para ocultar elementos no descubiertos.
- **Base de datos completa** (~400 elementos: amuletos, larvas, raíces, reliquias, estatuas de dioses, etc.) y **controles híbridos** (botones, sticks y pantalla táctil).

---

## 🚀 Instalación y Uso

1. Descarga `HKChecker.nro` desde la sección de [Releases](https://github.com).
2. Colócalo en tu tarjeta SD en: `sdmc:/switch/HKChecker/HKChecker.nro`.
3. **Modo de anulación de título (Importante):** Para permitir que la app lea los datos de guardado internos de la consola, abre el Menú Homebrew **manteniendo presionado `R` al iniciar cualquier juego** (no lo abras desde el modo Applet / Álbum).
4. El modo Applet solo permite analizar archivos `.dat` exportados previamente (ej. vía JKSV en `sdmc:/JKSV/`).

---

## 🎮 Controles

| Control | Navegador de Archivos | Vista de Análisis |
| :--- | :--- | :--- |
| `Stick / D-Pad` | Mover selección | Navegar filas (Arriba/Abajo) y pestañas (Izquierda/Derecha) |
| `L` / `R` | `L`: Montar datos internos de Switch | Cambiar entre las 8 pestañas |
| `A` | Abrir carpeta o analizar archivo | Plegar/desplegar sección o revelar fila seleccionada |
| `B` | Ir al directorio padre | Volver al explorador de archivos |
| `X` | — | Cambiar filtro (`ALL` / `MISSING`) |
| `Y` | — | Mostrar / Ocultar todos los spoilers |
| `Touch` | Seleccionar y desplazar | Cambiar pestañas o revelar filas |
| `-` / `+` | Cambiar idioma (`EN`/`ES`) / Salir | Cambiar idioma (`EN`/`ES`) / Salir |

---

## 🛠️ Compilación y Estructura

Requiere la cadena de herramientas [devkitPro](https://devkitpro.org) (`libnx`, `devkitA64`), librerías SDL2 (`switch-sdl2`, `switch-sdl2_image`, `switch-sdl2_ttf`) y `make`:

```bash
git clone https://github.com
cd HK-Checker
make
```

Estructura principal del repositorio:
- `source/main.cpp` (UI SDL2 y navegación), `source/hk_analyzer.hpp` (evaluación de partidas), `source/hk_web_db.hpp` (base de datos) y recursos gráficos/fuentes en `romfs/`.

---

## 📋 Créditos y Licencia

Basado en el trabajo original de **[ReznoRMichael](https://github.com)** ([Hollow Knight Completion Check](https://github.io)), con soporte de `libnx` ([Team Switch](https://github.com)), SDL2 y la Wiki de Hollow Knight (ES). 

*Distribuido bajo la [Licencia MIT](LICENSE). No afiliado con Team Cherry.*
