# HKChecker

> **Hollow Knight Completion Checker for Nintendo Switch** — Analyze your save files and track your progress toward 112% game completion.

---

## 🎮 Overview

HKChecker is a **Nintendo Switch homebrew application** that reads Hollow Knight save files and displays a detailed breakdown of your game completion percentage. It is a port and enhancement of the popular web-based [Hollow Knight Completion Check](https://reznormichael.github.io/hollow-knight-completion-check/) by [ReznoRMichael](https://github.com/ReznoRMichael).

### ✨ Features

- **8 tabs** matching the web version: Main, Essentials %, Achievements, Journal, Collectibles, Geo Caches, Secrets, Godhome
- **Bilingual UI** (English/Spanish) with instant language switching via `-` button
- **Anti-spoiler system**: locations and hints hidden by default, reveal by touching rows
- **Elderbug hints** in both languages to guide your next steps
- **Complete database**: 40 charms, 46 grubs, 15 whispering roots, 43 relics, 20 rancid eggs, 13 geo chests, 207 geo rocks, 158 journal entries, 44 God Statues, and more
- **Touch, button, and joystick** support

---

## 📋 Credits

This project is based on the excellent work of:

- **[ReznoRMichael](https://github.com/ReznoRMichael)** — Original [Hollow Knight Completion Check](https://reznormichael.github.io/hollow-knight-completion-check/) web application and database. The complete item database, completion logic, and hint system are derived from this project.

Additional thanks to:

- **[Team Switch](https://github.com/switchbrew/switchbrew)** — libnx and devkitA64 for Switch homebrew development
- **[SDL2](https://www.libsdl.org/)** — Cross-platform multimedia library
- **[Hollow Knight Wiki (ES)](https://hollowknight.fandom.com/es/wiki/Hollow_Knight_Wiki)** — Spanish translations reference

---

## 🛠️ Building

### Prerequisites

- **[devkitPro](https://devkitpro.org/)** toolchain for Nintendo Switch (libnx, devkitA64)
- **SDL2, SDL2_image, SDL2_ttf** portlibs for Switch
- **Make** (included with devkitPro)

On Windows, install the [devkitPro installer](https://devkitpro.org/wiki/Getting_Started) which includes all Switch dependencies.

### Compile

```bash
cd HKChecker
make
```

This produces `HKChecker.nro` in the project root, ready to transfer to your Switch.

To clean and rebuild:

```bash
make clean
make
```

---

## 📁 Project Structure

```
HKChecker/
├── Makefile              # Build configuration
├── icon.jpg              # App icon 256x256
├── source/
│   ├── main.cpp          # UI, navigation, rendering
│   ├── hk_analyzer.hpp   # Save file parsing, item evaluation
│   └── hk_web_db.hpp     # Complete item database (~400 items)
└── romfs/
    ├── fontello.ttf      # Custom font
    └── icons/            # UI icons (masks, geo, soul, etc.)
```

---

## 🎮 Controls

| Control | Action |
|---------|--------|
| `Stick/D-pad` | Navigate rows |
| `L/R` or `Stick` | Switch tabs |
| `A` | Analyze save |
| `B` | Go back |
| `-` | Toggle language (ES/EN) |
| `X` | Toggle filter (Missing/All) |
| `Y` | Toggle spoilers |
| `Touch` | Tap rows to reveal spoilers |

---

## 📖 How it works

The app scans the Switch's internal storage and SD card for Hollow Knight save files (`user1.dat`). Once loaded, it decompresses the ZIP archive, parses the JSON save data, and evaluates each item against the complete database to determine what you've collected and what's still missing.

---

## ⚠️ Disclaimer

This project is **not affiliated with Team Cherry** or **Hollow Knight**. All game assets and names are property of Team Cherry. This tool is for personal use only.

---

## 📜 License

This project is licensed under the [MIT License](LICENSE).

The original web application database and completion logic are based on [ReznoRMichael/hollow-knight-completion-check](https://github.com/ReznoRMichael/hollow-knight-completion-check), used with permission and attribution.

---

---

# HKChecker

> **Comprobador de completitud de Hollow Knight para Nintendo Switch** — Analiza tus archivos de guardado y rastrea tu progreso hacia el 112% de completitud del juego.

---

## 🎮 Descripción general

HKChecker es una **aplicación homebrew para Nintendo Switch** que lee archivos de guardado de Hollow Knight y muestra un desglose detallado del porcentaje de completitud del juego. Es un puerto y mejora de la popular herramienta web [Hollow Knight Completion Check](https://reznormichael.github.io/hollow-knight-completion-check/) de [ReznoRMichael](https://github.com/ReznoRMichael).

### ✨ Características

- **8 pestañas** que coinciden con la versión web: Principal, Esenciales %, Logros, Diario, Coleccionables, Alijos Geo, Secretos, Hogar Dioses
- **Interfaz bilingüe** (inglés/español) con cambio instantáneo de idioma con el botón `-`
- **Sistema anti-spoiler**: ubicaciones y pistas ocultas por defecto, revela tocando las filas
- **Pistas de Elderbug** en ambos idiomas para guiar tus próximos pasos
- **Base de datos completa**: 40 amuletos, 46 larvas, 15 raíces susurrantes, 43 reliquias, 20 huevos podridos, 13 cofres Geo, 207 rocas Geo, 158 entradas del diario, 44 estatuas de dioses y más
- **Soporte táctil, botones y joystick**

---

## 📋 Créditos

Este proyecto se basa en el excelente trabajo de:

- **[ReznoRMichael](https://github.com/ReznoRMichael)** — Aplicación web original [Hollow Knight Completion Check](https://reznormichael.github.io/hollow-knight-completion-check/) y base de datos. La base de datos completa de items, la lógica de completitud y el sistema de pistas se derivan de este proyecto.

Agradecimientos adicionales a:

- **[Team Switch](https://github.com/switchbrew/switchbrew)** — libnx y devkitA64 para desarrollo homebrew en Switch
- **[SDL2](https://www.libsdl.org/)** — Biblioteca multimedia multiplataforma
- **[Hollow Knight Wiki (ES)](https://hollowknight.fandom.com/es/wiki/Hollow_Knight_Wiki)** — Referencia de traducciones al español

---

## 🛠️ Compilación

### Requisitos previos

- **[devkitPro](https://devkitpro.org/)** toolchain para Nintendo Switch (libnx, devkitA64)
- **SDL2, SDL2_image, SDL2_ttf** portlibs para Switch
- **Make** (incluido con devkitPro)

En Windows, instala el [instalador de devkitPro](https://devkitpro.org/wiki/Getting_Started) que incluye todas las dependencias para Switch.

### Compilar

```bash
cd HKChecker
make
```

Esto genera `HKChecker.nro` en la raíz del proyecto, listo para transferir a tu Switch.

Para limpiar y recompilar:

```bash
make clean
make
```

---

## 📁 Estructura del proyecto

```
HKChecker/
├── Makefile              # Configuración de compilación
├── icon.jpg              # Icono de la aplicación 256x256
├── source/
│   ├── main.cpp          # Interfaz, navegación, renderizado
│   ├── hk_analyzer.hpp   # Análisis de guardados, evaluación de items
│   └── hk_web_db.hpp     # Base de datos completa (~400 items)
└── romfs/
    ├── fontello.ttf      # Fuente personalizada
    └── icons/            # Iconos de la interfaz (máscaras, geo, alma, etc.)
```

---

## 🎮 Controles

| Control | Acción |
|---------|--------|
| `Stick/Cruceta` | Navegar filas |
| `L/R` o `Stick` | Cambiar pestañas |
| `A` | Analizar guardado |
| `B` | Volver atrás |
| `-` | Cambiar idioma (ES/EN) |
| `X` | Cambiar filtro (Faltantes/Todos) |
| `Y` | Alternar spoilers |
| `Táctil` | Toca filas para revelar spoilers |

---

## 📖 Cómo funciona

La aplicación escanea el almacenamiento interno y la tarjeta SD de la Switch buscando archivos de guardado de Hollow Knight (`user1.dat`). Una vez cargado, descomprime el archivo ZIP, analiza los datos JSON del guardado y evalúa cada item contra la base de datos completa para determinar lo que has recolectado y lo que aún falta.

---

## ⚠️ Aviso

Este proyecto **no está afiliado con Team Cherry** ni con **Hollow Knight**. Todos los activos y nombres del juego son propiedad de Team Cherry. Esta herramienta es solo para uso personal.

---

## 📜 Licencia

Este proyecto está licenciado bajo la [Licencia MIT](LICENSE).

La base de datos de la aplicación web original y la lógica de completitud se basan en [ReznoRMichael/hollow-knight-completion-check](https://github.com/ReznoRMichael/hollow-knight-completion-check), usados con permiso y atribución.
