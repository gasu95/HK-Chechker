# HKChecker

[🇬🇧 English version](README.md)

> **Comprobador de completado de Hollow Knight para Nintendo Switch** — Analiza tus archivos de guardado y rastrea tu progreso hacia el 112% del juego.

---

## 🎮 Descripción general

HKChecker es una **aplicación homebrew para Nintendo Switch** que lee tus partidas de Hollow Knight directamente desde la memoria interna de la consola o desde copias de seguridad en la tarjeta SD (como JKSV) y muestra un desglose detallado de tu progreso. **Funciona completamente sin conexión a internet.**

Es un port y adaptación de la popular herramienta web [Hollow Knight Completion Check](https://github.io) de [ReznoRMichael](https://github.com).

### ✨ Características

- **8 pestañas completas**: `MAIN`, `ESENCIALES`, `DIARIO`, `COLECCIONABLES`, `ALIJOS GEO`, `SECRETOS`, `GODMASTER` y `ESTADÍSTICAS`.
- **Lectura automática de partidas**: detecta los 4 slots (`user1.dat` – `user4.dat`) directamente desde la memoria de la Switch o desde carpetas de respaldo en la SD.
- **Interfaz bilingüe** (Español / Inglés) con detección automática del idioma de la consola y cambio instantáneo con `-`.
- **Sistema anti-spoilers**: oculta nombres y ubicaciones de elementos pendientes hasta que los revelas individualmente o pulsas `Y`.
- **Base de datos completa**: 40 amuletos, 46 larvas, 15 raíces susurrantes, 43 reliquias, 20 huevos podridos, 13 cofres de Geo, 207 rocas de Geo, 158 entradas del diario, 44 estatuas de dioses y más.
- **Control híbrido**: soporte completo para botones, joysticks y pantalla táctil.

---

## 🚀 Instalación y uso

1. Descarga el archivo `HKChecker.nro` desde la sección de [Releases](https://github.com).
2. Cópialo en tu tarjeta SD dentro de la ruta:
   ```text
   sdmc:/switch/HKChecker/HKChecker.nro
   ```
3. **Importante (Modo Title Override):** Para que la aplicación pueda leer las partidas guardadas de la memoria interna de la Switch, abre el Homebrew Menu **manteniendo pulsado el botón `R` mientras inicias cualquier juego** (no desde el Álbum en Modo Applet).
4. Si abres la app desde el Álbum, aún podrás analizar archivos `.dat` extraídos previamente en tu tarjeta SD (por ejemplo, en `sdmc:/JKSV/`).

---

## 🎮 Controles

| Control | Acción en Explorador | Acción en Análisis |
| :--- | :--- | :--- |
| `Stick / Cruceta` | Mover selección | Navegar filas (Arriba/Abajo) y pestañas (Izq/Der) |
| `L` / `R` | `L`: Montar partidas internas de Switch | Cambiar entre las 8 pestañas |
| `A` | Abrir carpeta o analizar partida | Plegar/desplegar sección o revelar fila |
| `B` | Subir al directorio anterior | Volver al explorador de archivos |
| `X` | — | Alternar filtro (`TODOS` / `FALTANTES`) |
| `Y` | — | Mostrar / Ocultar todos los spoilers |
| `-` | Cambiar idioma (`ES` / `EN`) | Cambiar idioma (`ES` / `EN`) |
| `+` | Salir de la aplicación | Salir de la aplicación |
| `Táctil` | Seleccionar carpeta/archivo y navegar | Cambiar pestaña, pulsar botones o revelar filas |

---

## 🛠️ Compilación

### Requisitos previos

- **[devkitPro](https://devkitpro.org)** toolchain para Nintendo Switch (`libnx`, `devkitA64`)
- **Portlibs de Switch**: `switch-sdl2`, `switch-sdl2_image`, `switch-sdl2_ttf`
- **Make** (incluido con devkitPro)

### Compilar desde el código fuente

```bash
git clone https://github.com
cd HK-Checker
make
```

Esto generará `HKChecker.nro` en la raíz del proyecto, listo para transferir a tu Switch. Para limpiar y recompilar:

```bash
make clean
make
```

---

## 📁 Estructura del proyecto

```text
HK-Checker/
├── Makefile              # Configuración de compilación
├── icon.jpg              # Icono de la aplicación (256x256)
├── source/
│   ├── main.cpp          # Interfaz SDL2, navegación y renderizado
│   ├── hk_analyzer.hpp   # Lectura de guardados y evaluación de items
│   └── hk_web_db.hpp     # Base de datos completa (~400 items)
└── romfs/
    ├── fontello.ttf      # Fuente de iconos personalizada
    └── icons/            # Recursos gráficos de la UI (máscaras, geo, medallas, etc.)
```

---

## 📋 Créditos

Este proyecto se basa en el excelente trabajo de:

- **[ReznoRMichael](https://github.com)** — Aplicación web original [Hollow Knight Completion Check](https://github.io) y base de datos. La lógica de completado y estructura de items derivan de su proyecto.

Agradecimientos adicionales a:

- **[Team Switch / switchbrew](https://github.com)** — `libnx` y `devkitA64` para desarrollo homebrew en Switch.
- **[SDL2](https://libsdl.org)** — Biblioteca gráfica y de entrada.
- **[Hollow Knight Wiki (ES)](https://fandom.com)** — Referencia para las traducciones al español.

---

## ⚠️ Aviso

Este proyecto **no está afiliado con Team Cherry** ni con **Hollow Knight**. Todos los recursos gráficos y nombres del juego son propiedad de Team Cherry. Esta herramienta es únicamente para uso personal.

---

## 📜 Licencia

Este proyecto está bajo la [Licencia MIT](LICENSE).

La base de datos y la lógica de completado están basadas en [ReznoRMichael/hollow-knight-completion-check](https://github.com/hollow-knight-completion-check), usadas con atribución.
