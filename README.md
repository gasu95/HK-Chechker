# HKChecker

[🇪🇸 Versión en español](README.es.md)

> **Hollow Knight Completion Checker for Nintendo Switch** — Analyze your save files and track your progress toward 112% game completion.

---

## 🎮 Overview

HKChecker is a **Nintendo Switch homebrew application** that reads Hollow Knight save files directly from your console's storage and displays a detailed breakdown of your game completion percentage. It works completely offline, with no internet connection required.

It is a port and enhancement of the popular web-based [Hollow Knight Completion Check](https://reznormichael.github.io/hollow-knight-completion-check/) by [ReznoRMichael](https://github.com/ReznoRMichael).

### ✨ Features

- **8 tabs** matching the web version: Main, Essentials %, Achievements, Journal, Collectibles, Geo Caches, Secrets, Godhome
- **Bilingual UI** (English/Spanish) with instant language switching via `-` button
- **Anti-spoiler system**: locations and hints hidden by default, reveal by touching rows
- **Elderbug hints** in both languages to guide your next steps
- **Complete database**: 40 charms, 46 grubs, 15 whispering roots, 43 relics, 20 rancid eggs, 13 geo chests, 207 geo rocks, 158 journal entries, 44 God Statues, and more
- **Touch, button, and joystick** support
- **Offline-first design**: reads save files directly from your Switch's internal storage and SD card

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

**No internet connection is required** — all processing happens locally on your Switch.

---

## ⚠️ Disclaimer

This project is **not affiliated with Team Cherry** or **Hollow Knight**. All game assets and names are property of Team Cherry. This tool is for personal use only.

---

## 📜 License

This project is licensed under the [MIT License](LICENSE).

The original web application database and completion logic are based on [ReznoRMichael/hollow-knight-completion-check](https://github.com/ReznoRMichael/hollow-knight-completion-check), used with permission and attribution.
