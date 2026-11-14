# HKChecker

[🇬🇧 English version](README.md)

> **Comprobador de completitud de Hollow Knight para Nintendo Switch** — Analiza tus archivos de guardado y rastrea tu progreso hacia el 112% de completitud del juego.

---

## 🎮 Descripción general

HKChecker es una **aplicación homebrew para Nintendo Switch** que lee archivos de guardado de Hollow Knight directamente desde el almacenamiento de tu consola y muestra un desglose detallado del porcentaje de completitud del juego. **Funciona completamente sin conexión a internet**, no necesita acceso a red.

Es un port de la popular herramienta web [Hollow Knight Completion Check](https://reznormichael.github.io/hollow-knight-completion-check/) de [ReznoRMichael](https://github.com/ReznoRMichael).

### ✨ Características

- **8 pestañas** que coinciden con la versión web: Principal, Esenciales %, Logros, Diario, Coleccionables, Alijos Geo, Secretos, Hogar Dioses
- **Interfaz bilingüe** (inglés/español) con cambio instantáneo de idioma con el botón `-`
- **Sistema anti-spoiler**: ubicaciones y pistas ocultas por defecto, revela tocando las filas
- **Pistas de Elderbug** en ambos idiomas para guiar tus próximos pasos
- **Base de datos completa**: 40 amuletos, 46 larvas, 15 raíces susurrantes, 43 reliquias, 20 huevos podridos, 13 cofres Geo, 207 rocas Geo, 158 entradas del diario, 44 estatuas de dioses y más
- **Soporte táctil, botones y joystick**
- **Diseño sin conexión**: lee los archivos de guardado directamente desde el almacenamiento interno y la tarjeta SD de tu Switch

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

**No se requiere conexión a internet** — todo el procesamiento se realiza localmente en tu Switch.

---

## ⚠️ Aviso

Este proyecto **no está afiliado con Team Cherry** ni con **Hollow Knight**. Todos los activos y nombres del juego son propiedad de Team Cherry. Esta herramienta es solo para uso personal.

---

## 📜 Licencia

Este proyecto está licenciado bajo la [Licencia MIT](LICENSE).

La base de datos de la aplicación web original y la lógica de completitud se basan en [ReznoRMichael/hollow-knight-completion-check](https://github.com/ReznoRMichael/hollow-knight-completion-check), usados con permiso y atribución.
