#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "hk_analyzer.hpp"
#include <cstdio>
#include <cmath>

struct FileEntry {
    std::string name;
    std::string fullPath;
    bool isDirectory;
};

struct UIAssets {
    SDL_Texture* bg = nullptr;
    SDL_Texture* maskNormal = nullptr;
    SDL_Texture* maskSteel = nullptr;
    SDL_Texture* soulOrb = nullptr;
    SDL_Texture* notchFull = nullptr;
    SDL_Texture* notchEmpty = nullptr;
    SDL_Texture* notchOver = nullptr;
    SDL_Texture* geoIcon = nullptr;
    SDL_Texture* geoShade = nullptr;
    SDL_Texture* fleur = nullptr;
    SDL_Texture* attuned = nullptr;
    SDL_Texture* ascended = nullptr;
    SDL_Texture* radiant = nullptr;
    SDL_Texture* bNail = nullptr;
    SDL_Texture* bShell = nullptr;
    SDL_Texture* bCharms = nullptr;
    SDL_Texture* bSoul = nullptr;
};

std::string toLowerStr(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

bool mountSwitchHollowKnightSave(std::string& outMount,
                                 std::string& errES, std::string& errEN) {
    AccountUid uid{};
    if (R_FAILED(accountInitialize(AccountServiceType_Application))) {
        errES = "Error al iniciar servicio de cuentas.";
        errEN = "Failed to initialize account service.";
        return false;
    }
    Result rc = accountGetPreselectedUser(&uid);
    if (R_FAILED(rc)) rc = accountGetLastOpenedUser(&uid);
    accountExit();
    if (R_FAILED(rc)) {
        errES = "Abre la app desde un juego con [R] para leer saves internos.";
        errEN = "Launch app via Title Override [R] to read internal saves.";
        return false;
    }
    rc = fsdevMountSaveData("hksave", 0x0100633007D48000ULL, uid);
    if (R_FAILED(rc)) {
        errES = "No se encontro partida de Hollow Knight en esta cuenta.";
        errEN = "No Hollow Knight save data found on this account.";
        return false;
    }
    outMount = "hksave:/";
    return true;
}

std::string findHollowKnightFolder() {
    std::string base = "sdmc:/JKSV";
    std::error_code ec;
    if (!fs::exists(base, ec)) return "sdmc:/";
    for (const auto& entry : fs::directory_iterator(base, ec)) {
        if (entry.is_directory(ec)) {
            std::string name = toLowerStr(entry.path().filename().string());
            if (name.find("hollow") != std::string::npos) {
                return entry.path().string();
            }
        }
    }
    return base;
}

std::vector<FileEntry> getDirectoryContents(const std::string& path, bool isES) {
    std::vector<FileEntry> entries;
    if (path != "sdmc:/" && path != "sdmc:" && path != "hksave:/" && path != "hksave:") {
        fs::path p(path);
        entries.push_back({
            isES ? ".. (Volver atrás)" : ".. (Go back)",
            p.parent_path().string(), true
        });
    }
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(path, ec)) {
        entries.push_back({
            entry.path().filename().string(),
            entry.path().string(),
            entry.is_directory(ec)
        });
    }
    size_t offset = (!entries.empty() && entries[0].isDirectory &&
                     entries[0].name.find("..") == 0) ? 1 : 0;
    std::sort(entries.begin() + offset, entries.end(),
        [](const FileEntry& a, const FileEntry& b) {
            if (a.isDirectory != b.isDirectory)
                return a.isDirectory > b.isDirectory;
            return a.name < b.name;
        });
    return entries;
}

void drawRect(SDL_Renderer* ren, int x, int y, int w, int h, SDL_Color col) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
    SDL_Rect r{x, y, w, h};
    SDL_RenderFillRect(ren, &r);
}

void drawBorder(SDL_Renderer* ren, int x, int y, int w, int h, SDL_Color col) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
    SDL_Rect r{x, y, w, h};
    SDL_RenderDrawRect(ren, &r);
}

void drawTex(SDL_Renderer* ren, SDL_Texture* tex, int x, int y,
             int w, int h, Uint8 alpha = 255) {
    if (!tex) return;
    SDL_SetTextureAlphaMod(tex, alpha);
    SDL_Rect dst{x, y, w, h};
    SDL_RenderCopy(ren, tex, nullptr, &dst);
}

int drawText(SDL_Renderer* ren, TTF_Font* font, const std::string& text,
             int x, int y, SDL_Color col, bool rightAlign = false) {
    if (text.empty() || !font) return 0;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), col);
    if (!surf) return 0;
    int w = surf->w;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    if (tex) {
        int drawX = rightAlign ? (x - surf->w) : x;
        SDL_Rect dst{drawX, y, surf->w, surf->h};
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
    return w;
}

void drawGlyph(SDL_Renderer* ren, TTF_Font* iconFont, Uint16 code,
               int x, int y, SDL_Color col) {
    if (!iconFont) return;
    SDL_Surface* surf = TTF_RenderGlyph_Blended(iconFont, code, col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    if (tex) {
        SDL_Rect dst{x, y, surf->w, surf->h};
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void drawBackground(SDL_Renderer* ren, const UIAssets& ui) {
    SDL_SetRenderDrawColor(ren, 5, 13, 25, 255);
    SDL_RenderClear(ren);
    if (ui.bg) {
        SDL_Rect src{107, 0, 1920, 1080};
        SDL_Rect dst{0, 0, 1280, 720};
        SDL_RenderCopy(ren, ui.bg, &src, &dst);
        drawRect(ren, 0, 0, 1280, 720, {5, 13, 25, 75});
    }
}

std::string applySpoilerMask(std::string text, bool showSpoilers) {
    if (showSpoilers) return text;
    const std::vector<std::string> words = {
        "Pantheon", "Panteon", "Godhome", "Hogar de los Dioses",
        "Hogar Dioses", "Radiance", "Destello", "White Palace",
        "Palacio Blanco", "Path of Pain", "Sendero del Dolor"
    };
    for (const auto& w : words) {
        size_t pos = 0;
        while ((pos = text.find(w, pos)) != std::string::npos) {
            text.replace(pos, w.length(), "[SPOILER]");
            pos += 9;
        }
    }
    return text;
}
void renderFileBrowser(SDL_Renderer* ren, const UIAssets& ui, TTF_Font* fontLg,
                       TTF_Font* fontMd, TTF_Font* fontSm,
                       const std::string& currentPath,
                       const std::vector<FileEntry>& entries, int selected,
                       bool isAppletMode, bool isES,
                       const std::string& errorToast,
                       int& outListY, int& outRowH, int& outStartIdx) {
    SDL_Color headerBg{8, 20, 38, 225};
    SDL_Color hkBlue{30, 77, 108, 255};
    SDL_Color hkHighlight{46, 93, 124, 240};
    SDL_Color white{245, 248, 252, 255};
    SDL_Color gray{175, 190, 210, 255};
    SDL_Color gold{245, 205, 100, 255};

    drawBackground(ren, ui);

    drawRect(ren, 0, 0, 1280, 72, headerBg);
    drawBorder(ren, 0, 71, 1280, 1, hkBlue);
    drawText(ren, fontLg, "HOLLOW KNIGHT - SAVE COMPLETION ANALYZER", 36, 19, white);

    drawRect(ren, 1120, 16, 124, 40, hkHighlight);
    drawBorder(ren, 1120, 16, 124, 40, gold);
    drawText(ren, fontSm, isES ? "[-] ES / EN" : "[-] EN / ES", 1136, 26, gold);
    drawText(ren, fontSm, currentPath, 1100, 26, gray, true);

    int startY = 86;
    if (isAppletMode) {
        drawRect(ren, 36, startY, 1208, 42, {90, 48, 15, 225});
        drawBorder(ren, 36, startY, 1208, 42, gold);
        drawText(ren, fontSm, isES
            ? "AVISO: Modo Applet (Album). Sal con [+] y abre un juego manteniendo pulsado [R] para memoria completa."
            : "WARNING: Applet Mode (Album). Exit with [+] and launch a game holding [R] for full memory.",
            52, startY + 11, gold);
        startY += 52;
    }

    if (!errorToast.empty()) {
        drawRect(ren, 36, startY, 1208, 40, {110, 28, 35, 225});
        drawText(ren, fontSm, errorToast, 52, startY + 9, white);
        startY += 50;
    }

    int listHeight = 652 - startY;
    drawRect(ren, 36, startY, 1208, listHeight, {8, 20, 38, 185});
    drawBorder(ren, 36, startY, 1208, listHeight, hkBlue);

    int rowH = 48;
    int maxRows = (listHeight - 16) / rowH;
    int startIdx = std::max(0, selected - maxRows / 2);
    int endIdx = std::min((int)entries.size(), startIdx + maxRows);

    outListY = startY + 8;
    outRowH = rowH;
    outStartIdx = startIdx;

    for (int i = startIdx; i < endIdx; ++i) {
        int y = outListY + (i - startIdx) * rowH;
        bool isSel = (i == selected);
        if (isSel) {
            drawRect(ren, 48, y, 1184, rowH - 4, hkHighlight);
            drawBorder(ren, 48, y, 1184, rowH - 4, {110, 175, 220, 255});
        }
        const auto& item = entries.at(i);
        std::string tag = item.isDirectory
            ? (isES ? "[CARPETA]" : "[FOLDER]")
            : (isES ? "[PARTIDA]" : "[SAVE]");
        drawText(ren, fontSm, tag, 66, y + 11,
                 item.isDirectory ? gold : SDL_Color{120, 195, 245, 255});
        drawText(ren, fontMd, item.name, 190, y + 9, isSel ? white : gray);
    }

    drawRect(ren, 0, 664, 1280, 56, headerBg);
    drawBorder(ren, 0, 664, 1280, 1, hkBlue);
    drawText(ren, fontSm, isES
        ? "[Stick/Cruceta] Mover  [A] Analizar  [L] Save Switch  [B] Atrás  [-] Idioma  [+] Salir"
        : "[Stick/D-Pad] Move  [A] Analyze  [L] Switch Save  [B] Back  [-] Lang  [+] Exit",
        36, 681, white);
}

std::string getTabDescription(int tabIndex, bool isES) {
    if (tabIndex == 1)
        return isES
            ? "Cada elemento suma % oficial. Nota: Solo la mitad de jefes suman %. P1-P4 = dificultad y qué Panteón desbloquea."
            : "Each item provides 1-4% Game Completion. Note: Only ~half of bosses count. P1-P4 = difficulty & which Pantheon it unlocks.";
    if (tabIndex == 2)
        return isES
            ? "Pistas de Elderbug y requisitos clave para logros e historia."
            : "Elderbug hints and key requirements for story and achievements.";
    if (tabIndex == 3)
        return isES
            ? "146 entradas base completadas para la Marca del Cazador. Las opcionales no cuentan."
            : "146 base completed entries required for Hunter's Mark. Optional entries do not count.";
    if (tabIndex == 4)
        return isES
            ? "46 Larvas, 15 Raíces Susurrantes y 43 Reliquias individuales de Hallownest."
            : "46 Grubs, 15 Whispering Roots, and 43 individual Relics across Hallownest.";
    if (tabIndex == 5)
        return isES
            ? "Cofres principales de Geo y las 207 Rocas de Geo individuales del mapa."
            : "Main Geo chests and all 207 individual Geo rocks across the map.";
    if (tabIndex == 6)
        return isES
            ? "Salas ocultas, interacciones secretas del mundo y encuentros especiales."
            : "Hidden rooms, secret world interactions, and special encounters.";
    return isES
        ? "Panteones del Hogar de los Dioses (P1-P4 = 1% cada uno) y 44 estatuas del Salón de los Dioses."
        : "Pantheons of Godhome (P1-P4 = 1% each) and 44 Hall of Gods boss statues.";
}

void renderAnalysisScreen(SDL_Renderer* ren, const UIAssets& ui,
                          TTF_Font* fontLg, TTF_Font* fontMd,
                          TTF_Font* fontSm, TTF_Font* fontXs,
                          TTF_Font* fontIcon, SaveAnalysis& data,
                          int tabIndex, int scrollOffset, bool onlyMissing,
                          bool showSpoilers, bool showHints,
                          bool headerSpoilerRevealed,
                          bool mainStatusCollapsed,
                          bool mainGameStatsCollapsed,
                          bool mainGodStatsCollapsed, bool isES,
                          int& outRowStartY, int& outRowH,
                          std::vector<int>& outVisibleIndices) {
    SDL_Color headerBg{8, 20, 38, 225};
    SDL_Color hkBlue{30, 77, 108, 255};
    SDL_Color hkHighlight{46, 93, 124, 240};
    SDL_Color white{245, 248, 252, 255};
    SDL_Color gray{165, 180, 200, 255};
    SDL_Color dimGray{105, 115, 130, 255};
    SDL_Color green{65, 205, 120, 255};
    SDL_Color red{230, 75, 85, 255};
    SDL_Color gold{245, 205, 100, 255};

    drawBackground(ren, ui);

    drawRect(ren, 0, 0, 1280, 72, headerBg);
    drawBorder(ren, 0, 71, 1280, 1, hkBlue);
    drawText(ren, fontLg, data.fileName, 36, 20, white);

    drawRect(ren, 1120, 16, 124, 40, hkHighlight);
    drawBorder(ren, 1120, 16, 124, 40, gold);
    drawText(ren, fontSm, isES ? "[-] ES / EN" : "[-] EN / ES", 1136, 26, gold);

    char pctBuf[64];
    snprintf(pctBuf, sizeof(pctBuf), isES ? "OFICIAL: %.1f%% / 112%%"
                                          : "GAME: %.1f%% / 112%%",
             data.completionPercent);
    drawText(ren, fontMd, pctBuf, 860, 22,
             data.completionPercent >= 112.0f ? green : gold, true);

    char trueBuf[64];
    snprintf(trueBuf, sizeof(trueBuf), isES ? "REAL: %.1f%%" : "TRUE: %.1f%%", data.trueCompletionPercent);
    drawText(ren, fontMd, trueBuf, 1095, 22,
             data.trueCompletionPercent >= 100.0f ? green : SDL_Color{110, 195, 245, 255}, true);

    const std::vector<std::string> tabsES = {
        "PRINCIPAL", "ESENCIALES %", "LOGROS", "DIARIO",
        "COLECCIONABLES", "ALIJOS GEO", "SECRETOS", "HOGAR DIOSES"
    };
    const std::vector<std::string> tabsEN = {
        "MAIN", "ESSENTIALS %", "ACHIEVEMENTS", "JOURNAL",
        "COLLECTIBLES", "GEO CACHES", "SECRETS", "GODHOME"
    };
    const auto& tabs = isES ? tabsES : tabsEN;

    int tabW = 144;
    for (int i = 0; i < 8; ++i) {
        int tx = 36 + i * (tabW + 8);
        bool active = (i == tabIndex);
        drawRect(ren, tx, 82, tabW, 42, active ? hkHighlight : headerBg);
        drawBorder(ren, tx, 82, tabW, 42, active ? white : hkBlue);
        std::string tLabel = applySpoilerMask(tabs[i], showSpoilers);
        drawText(ren, fontXs, tLabel, tx + 6, 95, active ? white : gray);
    }

    drawRect(ren, 36, 134, 1208, 520, {8, 20, 38, 175});
    drawBorder(ren, 36, 134, 1208, 520, hkBlue);

    if (tabIndex == 0) {
        drawRect(ren, 56, 140, 340, 32, showHints ? hkHighlight : SDL_Color{55, 60, 70, 230});
        drawBorder(ren, 56, 140, 340, 32, gold);
        drawText(ren, fontSm, showHints ? (isES ? "Hints [X]: ACTIVADAS" : "Hints [X]: ON")
                                        : (isES ? "Hints [X]: OCULTAS"   : "Hints [X]: OFF"), 70, 146, white);

        drawRect(ren, 412, 140, 340, 32, !showSpoilers ? SDL_Color{85, 55, 20, 240} : hkHighlight);
        drawBorder(ren, 412, 140, 340, 32, gold);
        drawText(ren, fontSm, showSpoilers ? (isES ? "Spoilers [Y]: VISIBLES" : "Spoilers [Y]: SHOWN")
                                           : (isES ? "Spoilers [Y]: OCULTOS" : "Spoilers [Y]: HIDDEN"), 426, 146, white);

        int curY = 178;
        if (showHints) {
            drawRect(ren, 56, curY, 1168, 44, {14, 32, 54, 230});
            drawBorder(ren, 56, curY, 1168, 44, gold);
            const auto& hintVec = isES ? WEB_ELDERBUG_HINTS_ES : WEB_ELDERBUG_HINTS;
            std::string hintPrefix = isES ? "Elderbug me dijo una vez... " : "Elderbug once told me... ";
            std::string hintBody = "";
            if (!hintVec.empty()) {
                int safeH = std::max(0, std::min(data.elderbugHintIdx, (int)hintVec.size() - 1));
                hintBody = hintVec[safeH];
            }
            std::string hintTxt = applySpoilerMask(hintPrefix + hintBody, showSpoilers || headerSpoilerRevealed);
            drawText(ren, fontSm, hintTxt, 72, curY + 12, gold);
            curY += 50;
        }

        drawRect(ren, 56, curY, 1168, 34, {18, 44, 72, 235});
        drawBorder(ren, 56, curY, 1168, 34, gold);
        std::string a1 = mainStatusCollapsed ? "\xE2\x96\xB6  " : "\xE2\x96\xBC  ";
        drawText(ren, fontSm, a1 + (isES ? "ESTADO DEL JUEGO (GAME STATUS)" : "GAME STATUS"), 72, curY + 7, gold);
        curY += 38;

        if (!mainStatusCollapsed) {
            drawRect(ren, 56, curY, 1168, 128, {5, 13, 25, 195});
            drawBorder(ren, 56, curY, 1168, 128, hkBlue);

            drawText(ren, fontSm, (isES ? "MÁSCARAS (" : "MASKS (") + std::to_string(data.maxHealth) + "/9):", 74, curY + 10, gray);
            SDL_Texture* mTex = data.isSteelSoul ? ui.maskSteel : ui.maskNormal;
            for (int i = 0; i < 9; ++i)
                drawTex(ren, mTex, 275 + i * 35, curY + 4, 28, 34, (i < data.maxHealth) ? 255 : 55);

            int vessels = data.mpReserveMax / 33;
            drawText(ren, fontSm, (isES ? "ALMA (" : "SOUL (") +
                     std::to_string(vessels) + "/3):", 74, curY + 48, gray);
            for (int i = 0; i < 3; ++i)
                drawTex(ren, ui.soulOrb, 275 + i * 42, curY + 42, 32, 32,
                        (i < vessels) ? 255 : 55);

            drawText(ren, fontSm, (isES ? "MUESCAS (" : "NOTCHES (") +
                     std::to_string(data.charmSlots) + "/11):", 74, curY + 88, gray);
            for (int i = 0; i < 11; ++i) {
                SDL_Texture* nTex = (i < data.charmSlots) ? ui.notchFull : ui.notchEmpty;
                if (data.overcharmed && i == data.charmSlots - 1) nTex = ui.notchOver;
                drawTex(ren, nTex, 275 + i * 27, curY + 84, 22, 22, 255);
            }

            int gx = 635;
            gx += drawText(ren, fontMd, "Geo: ", gx, curY + 10, white);
            drawTex(ren, ui.geoIcon, gx, curY + 8, 24, 24, 255);
            gx += 28;
            gx += drawText(ren, fontMd, std::to_string(data.geo), gx, curY + 10, gold);
            if (data.shadeGeo > 0) {
                gx += drawText(ren, fontMd, " + ", gx, curY + 10, gray);
                drawTex(ren, ui.geoShade, gx, curY + 8, 24, 24, 255);
                gx += 28;
                gx += drawText(ren, fontMd, std::to_string(data.shadeGeo), gx, curY + 10, red);
                drawText(ren, fontSm, "  Total: " + std::to_string(data.geo + data.shadeGeo),
                         gx + 6, curY + 13, white);
            }

            int hours = (int)(data.playTime / 3600.0f);
            int mins = (int)((data.playTime - hours * 3600) / 60.0f);
            char timeBuf[64];
            snprintf(timeBuf, sizeof(timeBuf), "%dh %02dm", hours, mins);

            drawGlyph(ren, fontIcon, 0xe804, 637, curY + 48, white);
            drawText(ren, fontSm, (isES ? "Tiempo: " : "Time Played: ") +
                     std::string(timeBuf) + "   |   Save v" + data.saveVersion +
                     (data.isSteelSoul ? (isES ? "  [ALMA DE ACERO]" : "  [STEEL SOUL]") : ""),
                     667, curY + 48, white);

            char compLine[128];
            snprintf(compLine, sizeof(compLine),
                     isES ? "Oficial: %.1f%% / 112%%   |   True Completion: %.1f%%"
                          : "Game Completion: %.1f%% / 112%%   |   True Completion: %.1f%%",
                     data.completionPercent, data.trueCompletionPercent);
            drawText(ren, fontSm, compLine, 635, curY + 86, SDL_Color{120, 195, 245, 255});
            curY += 134;
        }

        drawRect(ren, 56, curY, 1168, 34, {18, 44, 72, 235});
        drawBorder(ren, 56, curY, 1168, 34, gold);
        std::string a2 = mainGameStatsCollapsed ? "\xE2\x96\xB6  " : "\xE2\x96\xBC  ";
        drawText(ren, fontSm, a2 + (isES ? "ESTADÍSTICAS DEL JUEGO (GAME STATISTICS)"
                                         : "GAME STATISTICS"), 72, curY + 7, gold);
        curY += 38;

        if (!mainGameStatsCollapsed) {
            drawRect(ren, 56, curY, 1168, 76, {5, 13, 25, 195});
            drawBorder(ren, 56, curY, 1168, 76, hkBlue);

            drawText(ren, fontSm,
                     (isES ? "Diario Cazador: " : "Hunter's Journal: ") +
                     std::to_string(data.journalCompleted) + "/" + std::to_string(data.journalTotal) +
                     (isES ? "   |   Larvas: " : "   |   Grubs Rescued: ") +
                     std::to_string(data.grubsCollected) + "/46" +
                     (isES ? "   |   Esencia: " : "   |   Dream Essence: ") +
                     std::to_string(data.dreamOrbs), 74, curY + 12, white);

            drawText(ren, fontSm,
                     (isES ? "Daño Aguijón: " : "Nail Damage: ") + std::to_string(data.nailDamage) +
                     (isES ? "   |   Mineral Pálido: " : "   |   Pale Ore: ") + std::to_string(data.paleOre) +
                     (isES ? "   |   Llaves Simples: " : "   |   Simple Keys: ") + std::to_string(data.simpleKeys) +
                     (isES ? "   |   Rocas Geo: " : "   |   Geo Rocks: ") +
                     std::to_string(data.geoRocksBroken) + "/207", 74, curY + 44, gray);
            curY += 82;
        }

        drawRect(ren, 56, curY, 1168, 34, {18, 44, 72, 235});
        drawBorder(ren, 56, curY, 1168, 34, gold);
        std::string a3 = mainGodStatsCollapsed ? "\xE2\x96\xB6  " : "\xE2\x96\xBC  ";
        drawText(ren, fontSm, a3 + applySpoilerMask(
            isES ? "ESTADÍSTICAS HOGAR DE DIOSES (GODHOME)" : "GODHOME STATISTICS",
            showSpoilers), 72, curY + 7, gold);
        curY += 38;

        if (!mainGodStatsCollapsed && curY + 74 <= 658) {
            drawRect(ren, 56, curY, 1168, 72, {5, 13, 25, 195});
            drawBorder(ren, 56, curY, 1168, 72, hkBlue);

            int totalGods = (int)WEB_HALL_OF_GODS.size();
            drawText(ren, fontSm,
                     applySpoilerMask((isES ? "Panteones Completados: " : "Pantheons Completed: ") +
                     std::to_string(data.pantheonsDone) + "/5   |   " +
                     (isES ? "Jefes Desbloqueados: " : "Bosses Unlocked: ") +
                     std::to_string(data.godsUnlocked) + "/" + std::to_string(totalGods),
                     showSpoilers), 74, curY + 10, white);

            drawText(ren, fontSm,
                     (isES ? "Sintonizado (Attuned): " : "Attuned: ") + std::to_string(data.godsAttuned) + "/" + std::to_string(totalGods) +
                     (isES ? "   |   Ascendido (Ascended): " : "   |   Ascended: ") + std::to_string(data.godsAscended) + "/" + std::to_string(totalGods) +
                     (isES ? "   |   Radiante (Radiant): " : "   |   Radiant: ") + std::to_string(data.godsRadiant) + "/" + std::to_string(totalGods),
                     74, curY + 42, SDL_Color{120, 195, 245, 255});
        }
    }
    else {
        std::vector<CheckItem>* src = &data.essentials112;
        if (tabIndex == 1) src = &data.essentialsExtra;
        if (tabIndex == 2) src = &data.achievementsHints;
        if (tabIndex == 3) src = &data.hunterJournal;
        if (tabIndex == 4) src = &data.collectibles;
        if (tabIndex == 5) src = &data.geoCaches;
        if (tabIndex == 6) src = &data.secrets;
        if (tabIndex == 7) src = &data.godhome;

        drawRect(ren, 56, 144, 1168, 40, {5, 13, 25, 210});
        drawBorder(ren, 56, 144, 1168, 40, hkBlue);
        std::string desc = applySpoilerMask(getTabDescription(tabIndex, isES),
                                            showSpoilers || headerSpoilerRevealed);
        drawText(ren, fontSm, desc, 70, 155, gray);

        bool sectionCollapsed = false;
        std::vector<int> filteredIdx;
        for (int idx = 0; idx < (int)src->size(); ++idx) {
            if ((*src)[idx].isHeader) {
                sectionCollapsed = (*src)[idx].isCollapsed;
                filteredIdx.push_back(idx);
            } else if (!sectionCollapsed) {
                if (!onlyMissing || (*src)[idx].state != ItemState::Done)
                    filteredIdx.push_back(idx);
            }
        }

        drawRect(ren, 56, 192, 340, 34, onlyMissing ? SDL_Color{110, 35, 45, 240} : hkHighlight);
        drawBorder(ren, 56, 192, 340, 34, gold);
        std::string fTxt = onlyMissing
            ? (isES ? "Filtro [X]: FALTANTES (" : "Filter [X]: MISSING (") + std::to_string(filteredIdx.size()) + ")"
            : (isES ? "Filtro [X]: TODOS (" : "Filter [X]: ALL (") + std::to_string(filteredIdx.size()) + ")";
        drawText(ren, fontSm, fTxt, 68, 199, gold);

        drawRect(ren, 412, 192, 340, 34, !showSpoilers ? SDL_Color{85, 55, 20, 240} : hkHighlight);
        drawBorder(ren, 412, 192, 340, 34, gold);
        std::string spTxt = showSpoilers
            ? (isES ? "Spoilers [Y]: VISIBLES" : "Spoilers [Y]: SHOWN")
            : (isES ? "Spoilers [Y]: OCULTOS" : "Spoilers [Y]: HIDDEN");
        drawText(ren, fontSm, spTxt, 426, 199, white);

        drawText(ren, fontSm, isES ? "Toca una fila o subtítulo para plegar/revelar"
                                   : "Tap row or header to fold/reveal",
                 1220, 199, SDL_Color{120, 195, 245, 255}, true);

        int maxRows = 6;
        int maxSc = std::max(0, (int)filteredIdx.size() - maxRows);
        int safeScroll = std::max(0, std::min(scrollOffset, maxSc));
        outRowStartY = 234;
        outRowH = 66;
        outVisibleIndices.clear();

        for (int i = safeScroll; i < std::min((int)filteredIdx.size(), safeScroll + maxRows); ++i) {
            int realIdx = filteredIdx[i];
            outVisibleIndices.push_back(realIdx);
            int y = outRowStartY + (i - safeScroll) * outRowH;
            const auto& item = (*src)[realIdx];

                        if (item.isHeader) {
                int subDone = 0, subTot = 0;
                for (int k = realIdx + 1; k < (int)src->size() && !(*src)[k].isHeader; ++k) {
                    if ((*src)[k].state != ItemState::Unavailable) {
                        subTot++;
                        if ((*src)[k].state == ItemState::Done) subDone++;
                    }
                }
                drawRect(ren, 56, y + 10, 1168, 42, {18, 44, 72, 235});
                drawBorder(ren, 56, y + 10, 1168, 42, gold);
                std::string cleanTitle = item.name(isES);
                if (cleanTitle.size() > 8 && cleanTitle.substr(0, 4) == "--- ")
                    cleanTitle = cleanTitle.substr(4, cleanTitle.size() - 8);
                std::string arrow = item.isCollapsed ? "\xE2\x96\xB6  " : "\xE2\x96\xBC  ";
                drawText(ren, fontMd, arrow + cleanTitle, 76, y + 19, gold);

                if (subTot > 0) {
                    bool allOk = (subDone == subTot);
                    SDL_Color boxBg = allOk ? SDL_Color{25, 95, 55, 240} : SDL_Color{115, 32, 42, 240};
                    SDL_Color boxBd = allOk ? green : red;
                    drawRect(ren, 1085, y + 15, 125, 32, boxBg);
                    drawBorder(ren, 1085, y + 15, 125, 32, boxBd);
                    drawText(ren, fontSm, std::to_string(subDone) + " / " + std::to_string(subTot),
                             1147, y + 21, white);
                }
                continue;
            }


            bool isDone = (item.state == ItemState::Done);
            bool isPartial = (item.state == ItemState::Partial);
            bool isUnavail = (item.state == ItemState::Unavailable);

            drawRect(ren, 56, y, 1168, 60, {5, 13, 25, 195});
            drawBorder(ren, 56, y, 1168, 60,
                       isDone ? SDL_Color{40, 95, 65, 255}
                              : (isUnavail ? SDL_Color{65, 70, 80, 255}
                                           : SDL_Color{105, 45, 55, 255}));

            Uint16 glyph = isDone ? 0xf14a : (isUnavail ? 0xf14a : 0xe800);
            SDL_Color gCol = isDone ? green : (isPartial ? gold : (isUnavail ? dimGray : red));
            drawGlyph(ren, fontIcon, glyph, 74, y + 18, gCol);

            bool isRevealed = (isDone || isPartial || isUnavail || showSpoilers || item.individuallyRevealed);
            std::string titleTxt = isRevealed
                ? applySpoilerMask(item.name(isES), isRevealed)
                : (isES ? "??? [SPOILER] - Toca esta fila para revelar"
                        : "??? [SPOILER] - Tap this row to reveal");
            std::string locTxt = isRevealed
                ? applySpoilerMask(item.loc(isES), isRevealed)
                : (isES ? "Ubicación censurada (Sin Spoilers activo)"
                        : "Location hidden (No Spoilers active)");

            drawText(ren, fontMd, titleTxt, 118, y + 7, isDone ? gray : (isUnavail ? dimGray : white));
            drawText(ren, fontSm, locTxt, 118, y + 33, isDone ? dimGray : SDL_Color{120, 195, 245, 255});

            if (item.badge.size() == 2 && item.badge.front() == 'P' && isRevealed) {
                drawTex(ren, ui.bNail,   960, y + 14, 28, 28, item.bindNail   ? 255 : 45);
                drawTex(ren, ui.bShell,  994, y + 14, 28, 28, item.bindShell  ? 255 : 45);
                drawTex(ren, ui.bCharms, 1028, y + 14, 28, 28, item.bindCharms ? 255 : 45);
                drawTex(ren, ui.bSoul,   1062, y + 14, 28, 28, item.bindSoul   ? 255 : 45);
            }
            if (item.godhomeRank > 0 && isRevealed) {
                SDL_Texture* medal = ui.attuned;
                if (item.godhomeRank == 2) medal = ui.ascended;
                if (item.godhomeRank == 3) medal = ui.radiant;
                drawTex(ren, medal, 1110, y + 12, 36, 36, 255);
            }
            drawText(ren, fontSm, item.badge, 1205, y + 19, gold, true);
        }
    }

    drawRect(ren, 0, 664, 1280, 56, headerBg);
    drawBorder(ren, 0, 664, 1280, 1, hkBlue);
    drawText(ren, fontSm, isES
        ? "[L/R/Stick] Sección  [X] Hints/Faltantes  [Y] Spoilers  [-] Idioma  [B] Atrás  [+] Salir"
        : "[L/R/Stick] Tab  [X] Hints/Missing  [Y] Spoilers  [-] Lang  [B] Back  [+] Exit",
        36, 681, white);
}
int main(int argc, char* argv[]) {
    setInitialize();
    plInitialize(PlServiceType_User);
    romfsInit();
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    TTF_Init();
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

    u64 langCode = 0;
    SetLanguage sysLang = SetLanguage_ENUS;
    if (R_SUCCEEDED(setGetSystemLanguage(&langCode))) setMakeLanguage(langCode, &sysLang);
    bool isES = (sysLang == SetLanguage_ES || sysLang == SetLanguage_ES419);

    SDL_Window* window = SDL_CreateWindow("HKChecker", 0, 0, 1280, 720, SDL_WINDOW_FULLSCREEN);
    SDL_Renderer* ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    UIAssets ui;
    ui.bg = IMG_LoadTexture(ren, "romfs:/icons/hk-background.jpg");
    ui.maskNormal = IMG_LoadTexture(ren, "romfs:/icons/health-mask.png");
    ui.maskSteel = IMG_LoadTexture(ren, "romfs:/icons/health-mask-steel.png");
    ui.soulOrb = IMG_LoadTexture(ren, "romfs:/icons/soul-orb.png");
    ui.notchFull = IMG_LoadTexture(ren, "romfs:/icons/notch-filled.png");
    ui.notchEmpty = IMG_LoadTexture(ren, "romfs:/icons/notch.png");
    ui.notchOver = IMG_LoadTexture(ren, "romfs:/icons/notch-overcharmed.png");
    ui.geoIcon = IMG_LoadTexture(ren, "romfs:/icons/geo.png");
    ui.geoShade = IMG_LoadTexture(ren, "romfs:/icons/geo-shade.png");
    ui.fleur = IMG_LoadTexture(ren, "romfs:/icons/fleur-divide.png");
    ui.attuned = IMG_LoadTexture(ren, "romfs:/icons/attuned.png");
    ui.ascended = IMG_LoadTexture(ren, "romfs:/icons/ascended.png");
    ui.radiant = IMG_LoadTexture(ren, "romfs:/icons/radiant.png");
    ui.bNail = IMG_LoadTexture(ren, "romfs:/icons/binding-nail.png");
    ui.bShell = IMG_LoadTexture(ren, "romfs:/icons/binding-shell.png");
    ui.bCharms = IMG_LoadTexture(ren, "romfs:/icons/binding-charms.png");
    ui.bSoul = IMG_LoadTexture(ren, "romfs:/icons/binding-soul.png");

    PlFontData fontData;
    plGetSharedFontByType(&fontData, PlSharedFontType_Standard);
    TTF_Font* fontLg = TTF_OpenFontRW(SDL_RWFromMem(fontData.address, fontData.size), 1, 24);
    TTF_Font* fontMd = TTF_OpenFontRW(SDL_RWFromMem(fontData.address, fontData.size), 1, 20);
    TTF_Font* fontSm = TTF_OpenFontRW(SDL_RWFromMem(fontData.address, fontData.size), 1, 16);
    TTF_Font* fontXs = TTF_OpenFontRW(SDL_RWFromMem(fontData.address, fontData.size), 1, 13);
    TTF_Font* fontIcon = TTF_OpenFont("romfs:/fontello.ttf", 22);

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    bool isAppletMode = (appletGetAppletType() != AppletType_Application &&
                         appletGetAppletType() != AppletType_SystemApplication);

    std::string errorToast, currentPath;
    bool inAnalysisView = false;
    SaveAnalysis currentSave;

    if (mountSwitchHollowKnightSave(currentPath, errorToast, errorToast)) {
        errorToast.clear();
        for (int slot = 1; slot <= 4; ++slot) {
            SaveAnalysis test = analyzeHollowKnightSave("hksave:/user" + std::to_string(slot) + ".dat");
            if (test.valid) { currentSave = std::move(test); inAnalysisView = true; break; }
        }
        if (!inAnalysisView)
            errorToast = isES ? "No se encontraron partidas en la memoria de la Switch."
                              : "No save files found in Switch storage.";
    } else {
        errorToast = isES ? "No se pudo leer la memoria interna (abre con [R] sobre un juego)."
                          : "Could not read internal save (launch holding [R] over a game).";
        currentPath = findHollowKnightFolder();
    }

    std::vector<FileEntry> entries = getDirectoryContents(currentPath, isES);
    int selectedIndex = entries.size() > 1 ? 1 : 0;
    int tabIndex = 0, scrollOffset = 0;
    bool onlyMissing = false, showSpoilers = false, showHints = true;
    bool headerSpoilerRevealed = false;
    bool mainStatusCollapsed = false, mainGameStatsCollapsed = false, mainGodStatsCollapsed = false;

    int listY = 100, rowH = 48, startIdx = 0;
    int anaRowStartY = 234, anaRowH = 66;
    std::vector<int> anaVisibleIndices;
    float lastTouchY = -1.0f;
    Uint32 lastBtnTick = 0, lastStickTick = 0;

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        HidAnalogStickState stickL = padGetStickPos(&pad, 0);
        Uint32 nowStick = SDL_GetTicks();
        if (nowStick - lastStickTick > 140) {
            if (stickL.y > 18000) { kDown |= HidNpadButton_Up; lastStickTick = nowStick; }
            else if (stickL.y < -18000) { kDown |= HidNpadButton_Down; lastStickTick = nowStick; }
            else if (stickL.x > 22000 && inAnalysisView) { kDown |= HidNpadButton_R; lastStickTick = nowStick; }
            else if (stickL.x < -22000 && inAnalysisView) { kDown |= HidNpadButton_L; lastStickTick = nowStick; }
        }

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_FINGERDOWN) {
                int tx = (int)(ev.tfinger.x * 1280.0f);
                int ty = (int)(ev.tfinger.y * 720.0f);
                lastTouchY = ev.tfinger.y;

                if (ty >= 16 && ty <= 56 && tx >= 1120 && tx <= 1244) kDown |= HidNpadButton_Minus;
                else if (ty >= 664) {
                    if (tx > 1080) goto exit_app;
                    if (!inAnalysisView && tx >= 230 && tx <= 460) kDown |= HidNpadButton_L;
                    if (tx > 800 && inAnalysisView) inAnalysisView = false;
                } else if (!inAnalysisView) {
                    if (ty >= listY && ty < 650 && !entries.empty()) {
                        int idx = startIdx + (ty - listY) / rowH;
                        if (idx >= 0 && idx < (int)entries.size()) { selectedIndex = idx; kDown |= HidNpadButton_A; }
                    }
                } else {
                    if (ty >= 82 && ty <= 124) {
                        int clickedTab = (tx - 36) / 152;
                        if (clickedTab >= 0 && clickedTab < 8) { tabIndex = clickedTab; scrollOffset = 0; headerSpoilerRevealed = false; }
                    }
                    Uint32 now = SDL_GetTicks();
                    if (tabIndex == 0 && now - lastBtnTick > 220) {
                        if (ty >= 140 && ty <= 172 && tx >= 56 && tx <= 396) { lastBtnTick = now; showHints = !showHints; }
                        else if (ty >= 140 && ty <= 172 && tx >= 412 && tx <= 752) { lastBtnTick = now; showSpoilers = !showSpoilers; }
                        else {
                            int yPtr = 178;
                            if (showHints) {
                                if (ty >= yPtr && ty <= yPtr + 44 && tx >= 56 && tx <= 1224) { lastBtnTick = now; headerSpoilerRevealed = !headerSpoilerRevealed; }
                                yPtr += 50;
                            }
                                                        if (ty >= yPtr && ty <= yPtr + 34 && tx >= 56 && tx <= 1224) {
                                lastBtnTick = now;
                                mainStatusCollapsed = !mainStatusCollapsed;
                            } else if (ty >= anaRowStartY && ty < 650 && !anaVisibleIndices.empty()) {
                                int clickedRow = (ty - anaRowStartY) / anaRowH;
                                if (clickedRow >= 0 && clickedRow < (int)anaVisibleIndices.size()) {
                                    lastBtnTick = now;
                                    int realIdx = anaVisibleIndices[clickedRow];
                                    if (currentSave.essentials112[realIdx].isHeader)
                                        currentSave.essentials112[realIdx].isCollapsed = !currentSave.essentials112[realIdx].isCollapsed;
                                    else
                                        currentSave.essentials112[realIdx].individuallyRevealed = !currentSave.essentials112[realIdx].individuallyRevealed;
                                }
                            }

                        }
                    }
                    if (tabIndex > 0 && now - lastBtnTick > 220) {
                        if (ty >= 140 && ty <= 188 && tx >= 56 && tx <= 1224) { lastBtnTick = now; headerSpoilerRevealed = !headerSpoilerRevealed; }
                        else if (ty >= 190 && ty <= 228 && tx >= 56 && tx <= 396) { lastBtnTick = now; onlyMissing = !onlyMissing; scrollOffset = 0; }
                        else if (ty >= 190 && ty <= 228 && tx >= 412 && tx <= 752) { lastBtnTick = now; showSpoilers = !showSpoilers; }
                        else if (ty >= anaRowStartY && ty < 640 && !anaVisibleIndices.empty()) {
                            int clickedRow = (ty - anaRowStartY) / anaRowH;
                            if (clickedRow >= 0 && clickedRow < (int)anaVisibleIndices.size()) {
                                lastBtnTick = now;
                                int realIdx = anaVisibleIndices[clickedRow];
                                std::vector<CheckItem>* src = &currentSave.essentials112;
                                if (tabIndex == 1) src = &currentSave.essentialsExtra;
                                if (tabIndex == 2) src = &currentSave.achievementsHints;
                                if (tabIndex == 3) src = &currentSave.hunterJournal;
                                if (tabIndex == 4) src = &currentSave.collectibles;
                                if (tabIndex == 5) src = &currentSave.geoCaches;
                                if (tabIndex == 6) src = &currentSave.secrets;
                                if (tabIndex == 7) src = &currentSave.godhome;
                                if ((*src)[realIdx].isHeader) (*src)[realIdx].isCollapsed = !(*src)[realIdx].isCollapsed;
                                else (*src)[realIdx].individuallyRevealed = !(*src)[realIdx].individuallyRevealed;
                            }
                        }
                    }
                }
            } else if (ev.type == SDL_FINGERMOTION && lastTouchY >= 0.0f) {
                float dy = ev.tfinger.y - lastTouchY;
                if (std::fabs(dy) > 0.035f) {
                    int step = (dy < 0) ? 1 : -1;
                    if (!inAnalysisView && !entries.empty())
                        selectedIndex = std::max(0, std::min((int)entries.size() - 1, selectedIndex + step));
                    else if (inAnalysisView)
                        scrollOffset = std::max(0, scrollOffset + step);
                    lastTouchY = ev.tfinger.y;
                }
            } else if (ev.type == SDL_FINGERUP) {
                lastTouchY = -1.0f;
            }
        }

        if (kDown & HidNpadButton_Plus) break;
        if (kDown & HidNpadButton_Minus) {
            isES = !isES;
            if (!inAnalysisView) entries = getDirectoryContents(currentPath, isES);
        }

        if (!inAnalysisView) {
            if (kDown & HidNpadButton_L) {
                std::string mPath, eES, eEN;
                if (mountSwitchHollowKnightSave(mPath, eES, eEN)) {
                    currentPath = mPath;
                    entries = getDirectoryContents(currentPath, isES);
                    selectedIndex = 0;
                    errorToast.clear();
                } else {
                    errorToast = isES ? eES : eEN;
                }
            }
            if (kDown & HidNpadButton_Down && !entries.empty())
                selectedIndex = (selectedIndex + 1) % entries.size();
            if (kDown & HidNpadButton_Up && !entries.empty())
                selectedIndex = (selectedIndex - 1 + entries.size()) % entries.size();
            if (kDown & HidNpadButton_B && currentPath != "sdmc:/" &&
                currentPath != "sdmc:" && currentPath != "hksave:/") {
                currentPath = fs::path(currentPath).parent_path().string();
                entries = getDirectoryContents(currentPath, isES);
                selectedIndex = 0;
                errorToast.clear();
            }
            if (kDown & HidNpadButton_A && !entries.empty()) {
                const auto& sel = entries.at(selectedIndex);
                if (sel.isDirectory) {
                    currentPath = sel.fullPath;
                    entries = getDirectoryContents(currentPath, isES);
                    selectedIndex = 0;
                    errorToast.clear();
                } else {
                    SaveAnalysis analyzed = analyzeHollowKnightSave(sel.fullPath);
                    if (analyzed.valid) {
                        currentSave = std::move(analyzed);
                        inAnalysisView = true;
                        tabIndex = 0;
                        scrollOffset = 0;
                        errorToast.clear();
                    } else {
                        errorToast = isES ? analyzed.errorES : analyzed.errorEN;
                    }
                }
            }
            renderFileBrowser(ren, ui, fontLg, fontMd, fontSm, currentPath,
                              entries, selectedIndex, isAppletMode, isES,
                              errorToast, listY, rowH, startIdx);
        } else {
            if (kDown & HidNpadButton_B) inAnalysisView = false;
            if (kDown & HidNpadButton_R) {
                tabIndex = (tabIndex + 1) % 8;
                scrollOffset = 0;
                headerSpoilerRevealed = false;
            }
            if (kDown & HidNpadButton_L) {
                tabIndex = (tabIndex + 7) % 8;
                scrollOffset = 0;
                headerSpoilerRevealed = false;
            }
            if (kDown & HidNpadButton_X) {
                if (tabIndex == 0) showHints = !showHints;
                else { onlyMissing = !onlyMissing; scrollOffset = 0; }
            }
            if (kDown & HidNpadButton_Y) showSpoilers = !showSpoilers;
            if (kDown & HidNpadButton_Down) scrollOffset++;
            if (kDown & HidNpadButton_Up && scrollOffset > 0) scrollOffset--;

            renderAnalysisScreen(ren, ui, fontLg, fontMd, fontSm, fontXs,
                                 fontIcon, currentSave, tabIndex, scrollOffset,
                                 onlyMissing, showSpoilers, showHints,
                                 headerSpoilerRevealed, mainStatusCollapsed,
                                 mainGameStatsCollapsed, mainGodStatsCollapsed,
                                 isES, anaRowStartY, anaRowH, anaVisibleIndices);
        }
        SDL_RenderPresent(ren);
    }

exit_app:
    if (ui.bg) SDL_DestroyTexture(ui.bg);
    if (ui.maskNormal) SDL_DestroyTexture(ui.maskNormal);
    if (ui.maskSteel) SDL_DestroyTexture(ui.maskSteel);
    if (ui.soulOrb) SDL_DestroyTexture(ui.soulOrb);
    if (ui.notchFull) SDL_DestroyTexture(ui.notchFull);
    if (ui.notchEmpty) SDL_DestroyTexture(ui.notchEmpty);
    if (ui.notchOver) SDL_DestroyTexture(ui.notchOver);
    if (ui.geoIcon) SDL_DestroyTexture(ui.geoIcon);
    if (ui.geoShade) SDL_DestroyTexture(ui.geoShade);
    if (ui.fleur) SDL_DestroyTexture(ui.fleur);
    if (ui.attuned) SDL_DestroyTexture(ui.attuned);
    if (ui.ascended) SDL_DestroyTexture(ui.ascended);
    if (ui.radiant) SDL_DestroyTexture(ui.radiant);
    if (ui.bNail) SDL_DestroyTexture(ui.bNail);
    if (ui.bShell) SDL_DestroyTexture(ui.bShell);
    if (ui.bCharms) SDL_DestroyTexture(ui.bCharms);
    if (ui.bSoul) SDL_DestroyTexture(ui.bSoul);
    if (fontIcon) TTF_CloseFont(fontIcon);
    TTF_CloseFont(fontLg);
    TTF_CloseFont(fontMd);
    TTF_CloseFont(fontSm);
    TTF_CloseFont(fontXs);
    IMG_Quit();
    TTF_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(window);
    SDL_Quit();
    fsdevUnmountDevice("hksave");
    romfsExit();
    plExit();
    setExit();
    return 0;
}
