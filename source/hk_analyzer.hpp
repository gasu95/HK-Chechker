#pragma once
#include <sys/types.h>
#include <zlib.h>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <unordered_set>
#include "hk_web_db.hpp"

namespace fs = std::filesystem;

enum class ItemState {
    Done = 0,
    Partial = 1,
    Missing = 2,
    Unavailable = 3
};

struct CheckItem {
    std::string nameES;
    std::string nameEN;
    std::string locES;
    std::string locEN;
    ItemState state;
    std::string badge;
    int godhomeRank = 0;
    bool bindNail = false, bindShell = false, bindCharms = false, bindSoul = false;
    bool individuallyRevealed = false;
    bool isHeader = false;
            bool isCollapsed = false;

    CheckItem() = default;
    CheckItem(std::string nES, std::string nEN, std::string lES, std::string lEN,
              ItemState st, std::string bdg, int rank = 0)
        : nameES(std::move(nES)), nameEN(std::move(nEN)),
          locES(std::move(lES)), locEN(std::move(lEN)),
          state(st), badge(std::move(bdg)), godhomeRank(rank) {}



    const std::string& name(bool isES) const { return isES ? nameES : nameEN; }
    const std::string& loc(bool isES) const { return isES ? locES : locEN; }
};

struct SaveAnalysis {
    bool valid = false;
    std::string errorES;
    std::string errorEN;
    std::string fileName;
        std::string saveVersion = "1.5.78";
    int geoRocksBroken = 0;
    int pantheonsDone = 0;
    int godsUnlocked = 0;
    int godsAttuned = 0;
    int godsAscended = 0;
    int godsRadiant = 0;


    float completionPercent = 0.0f;
    float trueCompletionPercent = 0.0f;
    int geo = 0;
    int shadeGeo = 0;
    int maxHealth = 5;
    int mpReserveMax = 0;
    int dreamOrbs = 0;
    int grubsCollected = 0;
    int nailDamage = 5;
    int charmSlots = 3;
    bool overcharmed = false;
    int paleOre = 0;
    int simpleKeys = 0;
    int journalCompleted = 0;
    int journalEncountered = 0;
    int journalTotal = 146;
    float playTime = 0.0f;
    bool isSteelSoul = false;

    int elderbugHintIdx = 0;

    std::vector<CheckItem> essentials112;
    std::vector<CheckItem> essentialsExtra;
    std::vector<CheckItem> achievementsHints;
    std::vector<CheckItem> hunterJournal;
    std::vector<CheckItem> collectibles;
    std::vector<CheckItem> geoCaches;
    std::vector<CheckItem> secrets;
    std::vector<CheckItem> godhome;
};

inline uint16_t readU16(const uint8_t* p) {
    return (uint16_t)(*p) | ((uint16_t)(*(p + 1)) << 8);
}

inline uint32_t readU32(const uint8_t* p) {
    return (uint32_t)(*p) |
           ((uint32_t)(*(p + 1)) << 8) |
           ((uint32_t)(*(p + 2)) << 16) |
           ((uint32_t)(*(p + 3)) << 24);
}

inline bool extractSaveFromZip(const std::string& zipPath,
                               std::string& outContent,
                               std::string& errES,
                               std::string& errEN) {
    std::ifstream file(zipPath, std::ios::binary);
    if (!file) {
        errES = "No se pudo abrir el archivo ZIP.";
        errEN = "Could not open ZIP file.";
        return false;
    }

    std::vector<uint8_t> data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    file.close();

    size_t pos = 0;
    std::string fallbackContent;
    const uint8_t* basePtr = data.data();

    while (pos + 30 <= data.size()) {
        if (readU32(basePtr + pos) != 0x04034b50) break;

        uint16_t method = readU16(basePtr + pos + 8);
        uint32_t compSize = readU32(basePtr + pos + 18);
        uint32_t uncompSize = readU32(basePtr + pos + 22);
        uint16_t nameLen = readU16(basePtr + pos + 26);
        uint16_t extraLen = readU16(basePtr + pos + 28);

        if (pos + 30 + nameLen + extraLen + compSize > data.size()) break;

        std::string entryName(
            reinterpret_cast<const char*>(basePtr + pos + 30), nameLen
        );
        const uint8_t* compData = basePtr + pos + 30 + nameLen + extraLen;

        bool isUserDat = (entryName.find("user") != std::string::npos &&
                          entryName.find(".dat") != std::string::npos &&
                          entryName.find(".bak") == std::string::npos);

        if (isUserDat) {
            std::string extracted(uncompSize, '\0');
            bool ok = false;

            if (method == 0) {
                std::memcpy(extracted.data(), compData, uncompSize);
                ok = true;
            } else if (method == 8) {
                z_stream strm{};
                strm.next_in = const_cast<Bytef*>(compData);
                strm.avail_in = compSize;
                strm.next_out = reinterpret_cast<Bytef*>(extracted.data());
                strm.avail_out = uncompSize;

                if (inflateInit2(&strm, -MAX_WBITS) == Z_OK) {
                    if (inflate(&strm, Z_FINISH) == Z_STREAM_END) ok = true;
                    inflateEnd(&strm);
                }
            }

            if (ok && !extracted.empty()) {
                if (entryName.find("user1.dat") != std::string::npos) {
                    outContent = std::move(extracted);
                    return true;
                }
                if (fallbackContent.empty()) fallbackContent = std::move(extracted);
            }
        }
        pos += 30 + nameLen + extraLen + compSize;
    }

    if (!fallbackContent.empty()) {
        outContent = std::move(fallbackContent);
        return true;
    }

    errES = "No hay ningun user*.dat valido dentro del ZIP.";
    errEN = "No valid user*.dat found inside ZIP.";
    return false;
}

inline size_t findKeyPos(const std::string& raw, const std::string& key) {
    std::string token = "\"" + key + "\"";
    size_t pos = raw.find(token);
    if (pos == std::string::npos) return std::string::npos;
    pos = raw.find(':', pos + token.size());
    if (pos == std::string::npos) return std::string::npos;
    pos++;
    while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == '\t' ||
           raw[pos] == '\r' || raw[pos] == '\n')) {
        pos++;
    }
    return pos;
}

inline bool getBool(const std::string& pd, const std::string& key) {
    size_t pos = findKeyPos(pd, key);
    if (pos == std::string::npos) return false;
    return pd.compare(pos, 4, "true") == 0;
}

inline int getInt(const std::string& pd, const std::string& key, int def = 0) {
    size_t pos = findKeyPos(pd, key);
    if (pos == std::string::npos) return def;
    char* endPtr = nullptr;
    long val = std::strtol(pd.c_str() + pos, &endPtr, 10);
    if (endPtr == pd.c_str() + pos) return def;
    return (int)val;
}

inline float getFloat(const std::string& pd, const std::string& key, float def = 0.0f) {
    size_t pos = findKeyPos(pd, key);
    if (pos == std::string::npos) return def;
    char* endPtr = nullptr;
    float val = std::strtof(pd.c_str() + pos, &endPtr);
    if (endPtr == pd.c_str() + pos) return def;
    return val;
}

inline bool evalRawEss(const RawEssEntry& e, const std::string& pd,
                       const std::unordered_set<std::string>& boolItems) {
    if (!e.scene.empty() && !e.id.empty()) {
        if (boolItems.count(e.scene + "|" + e.id) > 0) return true;
    }
    if (!e.key.empty() && getBool(pd, e.key)) return true;
    if (!e.id.empty() && getBool(pd, e.id)) return true;
    if (e.key == "hasAllMaps") return getBool(pd, "mapAllRooms");
    if (e.key == "hasQuill") return getBool(pd, "hasQuill");
    if (e.key == "openedCrossroadsStag") return getBool(pd, "openedCrossroads");
    if (e.key == "openedGreenpathStag") return getBool(pd, "openedGreenpath");
    if (e.key == "openedFungalWastesStag") return getBool(pd, "openedFungalWastes");
    if (e.key == "openedCityStag") return getBool(pd, "openedCity");
    if (e.key == "openedRuins2Stag") return getBool(pd, "openedRuins2");
    if (e.key == "openedRestingGroundsStag") return getBool(pd, "openedRestingGrounds");
    if (e.key == "openedDeepnestStag") return getBool(pd, "openedDeepnest");
    if (e.key == "openedHiddenStationStag") return getBool(pd, "openedHiddenStation");
    if (e.key == "openedGardensStag") return getBool(pd, "openedGardens");
    if (e.key == "openedStagNest") return getBool(pd, "stagPosition") || getInt(pd, "stationsOpened") >= 10;
    return false;
}

inline CheckItem makeHeader(const std::string& es, const std::string& en)
 {
    CheckItem c;
    c.nameES = es;
    c.nameEN = en;
    c.state = ItemState::Done;
    c.isHeader = true;
    return c;
}

inline ItemState stBool(bool cond)
 {
    return cond ? ItemState::Done : ItemState::Missing;
}

inline bool getObjBool(const std::string& pd, const std::string& objKey,
                       const std::string& field) {
    size_t pos = findKeyPos(pd, objKey);
    if (pos == std::string::npos) return false;
    size_t endObj = pd.find('}', pos);
    if (endObj == std::string::npos) return false;
    return getBool(pd.substr(pos, endObj - pos + 1), field);
}

inline int getStatueRank(const std::string& pd, const std::string& statueKey) {
    if (getObjBool(pd, statueKey, "completedTier3")) return 3;
    if (getObjBool(pd, statueKey, "completedTier2")) return 2;
    if (getObjBool(pd, statueKey, "completedTier1")) return 1;
    return 0;
}

inline std::string getLoc(const std::string& spoilerES, const std::string& spoiler) {
    return spoilerES.empty() ? spoiler : spoilerES;
}

inline void indexSceneData(const std::string& raw,
                           std::unordered_set<std::string>& outActivatedBools,
                           std::unordered_set<std::string>& outBrokenRocks) {
    size_t sdPos = raw.find("\"sceneData\"");
    if (sdPos == std::string::npos) return;

    size_t pos = sdPos;
    while ((pos = raw.find("\"id\"", pos)) != std::string::npos) {
        size_t objStart = raw.rfind('{', pos);
        size_t objEnd = raw.find('}', pos);
        if (objStart == std::string::npos || objEnd == std::string::npos) break;

        std::string obj = raw.substr(objStart, objEnd - objStart + 1);
        size_t idQ1 = obj.find('"', obj.find("\"id\"") + 4);
        size_t idQ2 = (idQ1 != std::string::npos) ? obj.find('"', idQ1 + 1) : std::string::npos;

        size_t scPos = obj.find("\"sceneName\"");
        size_t scQ1 = (scPos != std::string::npos) ? obj.find('"', scPos + 11) : std::string::npos;
        size_t scQ2 = (scQ1 != std::string::npos) ? obj.find('"', scQ1 + 1) : std::string::npos;

        if (idQ1 != std::string::npos && idQ2 != std::string::npos &&
            scQ1 != std::string::npos && scQ2 != std::string::npos) {
            std::string idVal = obj.substr(idQ1 + 1, idQ2 - idQ1 - 1);
            std::string scVal = obj.substr(scQ1 + 1, scQ2 - scQ1 - 1);
            std::string composite = scVal + "|" + idVal;

            if (obj.find("\"activated\":true") != std::string::npos ||
                obj.find("\"activated\": true") != std::string::npos) {
                outActivatedBools.insert(composite);
            }
            if (obj.find("\"hitsLeft\":0") != std::string::npos ||
                obj.find("\"hitsLeft\": 0") != std::string::npos) {
                outBrokenRocks.insert(composite);
            }
        }
        pos = objEnd + 1;
    }
}

inline SaveAnalysis analyzeHollowKnightSave(const std::string& filePath) {
    SaveAnalysis res;
    res.fileName = fs::path(filePath).filename().string();
    
    std::string ext = fs::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::string pd;
    if (ext == ".zip") {
        if (!extractSaveFromZip(filePath, pd, res.errorES, res.errorEN))
            return res;
    } else {
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile) {
            res.errorES = "No se pudo abrir el archivo.";
            res.errorEN = "Could not open file.";
            return res;
        }
        pd.assign(
            (std::istreambuf_iterator<char>(inFile)),
            std::istreambuf_iterator<char>()
        );
    }

    if (pd.find("playerData") == std::string::npos) {
        res.errorES = "No es un save JSON valido de Hollow Knight.";
        res.errorEN = "Not a valid Hollow Knight JSON save.";
        return res;
    }

            res.valid = true;
    size_t vPos = pd.find("\"version\"");
    if (vPos != std::string::npos) {
        size_t q1 = pd.find('"', vPos + 9);
        size_t q2 = (q1 != std::string::npos) ? pd.find('"', q1 + 1) : std::string::npos;
        if (q1 != std::string::npos && q2 != std::string::npos)
            res.saveVersion = pd.substr(q1 + 1, q2 - q1 - 1);
    }

    
    res.completionPercent = getFloat(pd, "completionPercentage");
    res.geo = getInt(pd, "geo");
    res.shadeGeo = getInt(pd, "geoPool", 0);
    res.maxHealth = getInt(pd, "maxHealth", 5);
    res.mpReserveMax = getInt(pd, "MPReserveMax", 0);
    res.dreamOrbs = getInt(pd, "dreamOrbs", 0);
    res.grubsCollected = getInt(pd, "grubsCollected", 0);
    res.nailDamage = getInt(pd, "nailDamage", 5);
    res.charmSlots = getInt(pd, "charmSlots", 3);
    res.overcharmed = getBool(pd, "overcharmed");
    res.paleOre = getInt(pd, "ore", 0);
    res.simpleKeys = getInt(pd, "simpleKeys", 0);
    res.journalCompleted = getInt(pd, "journalNotesCompleted", 0);
    res.journalEncountered = getInt(pd, "journalEntriesCompleted", 0);
    res.playTime = getFloat(pd, "playTime", 0.0f);
    res.isSteelSoul = (getInt(pd, "permadeathMode", 0) > 0);

    std::unordered_set<std::string> boolItems;
    std::unordered_set<std::string> brokenRocks;
    indexSceneData(pd, boolItems, brokenRocks);

    // Pista real de Elderbug extraída de WEB_ELDERBUG_HINTS
    int hintIdx = 0;
    if (!getBool(pd, "hasDash")) hintIdx = 2;
    else if (!getBool(pd, "hasWalljump")) hintIdx = 3;
    else if (!getBool(pd, "hasDreamNail")) hintIdx = 8;
    else if (!getBool(pd, "hasDoubleJump")) hintIdx = 10;
    else if (!getBool(pd, "hasShadowDash")) hintIdx = 12;
    else hintIdx = (int)WEB_ELDERBUG_HINTS.size() - 2;

    if (!WEB_ELDERBUG_HINTS.empty()) {
        int safeH = std::max(0, std::min(hintIdx, (int)WEB_ELDERBUG_HINTS.size() - 1));
        res.elderbugHintIdx = safeH;
    }

    // 1. Esenciales 112% (Jefes, Equipo, Aguijón, Hechizos, 25 Fragmentos, 40 Amuletos)
    int nailUp = getInt(pd, "nailSmithUpgrades", 0);
    int maskShards = (res.maxHealth - 5) * 4 + getInt(pd, "heartPieces");
    int vesselFrags = (res.mpReserveMax / 33) * 3 + getInt(pd, "vesselFragments");

    res.essentials112 = {
        makeHeader("--- JEFES PRINCIPALES Y DLC (17%) ---", "--- MAIN & DLC BOSSES (17%) ---"),
        {"[P1] Jefe: Falso Caballero", "[P1] Boss: False Knight", "Cruces Olvidados", "Forgotten Crossroads", stBool(getBool(pd, "killedFalseKnight")), "1%"},
        {"[P1] Jefe: Hornet Protectora", "[P1] Boss: Protectora Hornet", "Sendero Verde", "Greenpath", stBool(getBool(pd, "killedHornet")), "1%"},
        {"[P1] Jefe: Madre Gruz", "[P1] Boss: Gruz Mother", "Cruces Olvidados", "Forgotten Crossroads", stBool(getBool(pd, "killedBigFly")), "1%"},
        {"[P1] Jefe: Mawlek Incubador", "[P1] Boss: Brooding Mawlek", "Cruces Olvidados", "Forgotten Crossroads", stBool(getBool(pd, "killedMawlek")), "1%"},
        {"[P2] Jefe: Señores Mantis", "[P2] Boss: Mantis Lords", "Aldea Mantis", "Mantis Village", stBool(getBool(pd, "defeatedMantisLords")), "1%"},
        {"[P1] Jefe: Maestro de Almas", "[P1] Boss: Soul Master", "Santuario de Almas", "Soul Sanctum", stBool(getBool(pd, "killedMageLord")), "1%"},
        {"[P1] Jefe: Defensor del Estiércol", "[P1] Boss: Dung Defender", "Canales Reales", "Royal Waterways", stBool(getBool(pd, "killedDungDefender")), "1%"},
        {"[P2] Jefe: Receptáculo Roto", "[P2] Boss: Broken Vessel", "Cuenca Antigua", "Ancient Basin", stBool(getBool(pd, "killedInfectedKnight")), "1%"},
        {"[P2] Jefe: Nosk", "[P2] Boss: Nosk", "Nido Profundo", "Deepnest", stBool(getBool(pd, "killedMimicSpider")), "1%"},
        {"[P3] Jefe: Centinela Hornet", "[P3] Boss: Centinela Hornet", "Límite del Reino", "Kingdom's Edge", stBool(getBool(pd, "hornetOutskirtsDefeated")), "1%"},
        {"[P4] Jefe: Caballeros Vigías", "[P4] Boss: Watcher Knights", "Torre del Vigia", "Watcher's Spire", stBool(getBool(pd, "killedBlackKnight")), "1%"},
        {"[P3] Jefe: Uumuu", "[P3] Boss: Uumuu", "Archivos de la Maestra", "Teacher's Archives", stBool(getBool(pd, "killedMegaJellyfish")), "1%"},
        {"[P3] Jefe: El Coleccionista", "[P3] Boss: The Collector", "Torre del Amor", "Tower of Love", stBool(getBool(pd, "killedJarCollector")), "1%"},
        {"[P4] Jefe: Señor Desleal", "[P4] Boss: Traitor Lord", "Jardines de la Reina", "Queen's Gardens", stBool(getBool(pd, "killedTraitorLord")), "1%"},
        {"[P3] DLC: Maestro Compañía Grimm", "[P3] DLC: Troupe Master Grimm", "Bocasucia", "Dirtmouth", stBool(getBool(pd, "killedGrimm")), "1%"},
        {"DLC: Rey Pesadilla / Destierro", "DLC: Nightmare King / Banish", "Bocasucia / Acantilados", "Dirtmouth / Cliffs", stBool(getBool(pd, "killedNightmareGrimm") || getBool(pd, "destroyedNightmareLantern")), "1%"},
        {"[P3] DLC: Caballero Colmena", "[P3] DLC: Hive Knight", "La Colmena", "The Hive", stBool(getBool(pd, "killedHiveKnight")), "1%"},
        makeHeader("--- GUERREROS ONÍRICOS (7%) ---", "--- WARRIOR DREAMS (7%) ---"),
        {"[P2] Onirico: Xero", "[P2] Warrior Dream: Xero", "Tierras de Reposo", "Resting Grounds", stBool(getBool(pd, "killedGhostXero")), "1%"},
        {"[P1] Onirico: Gorb", "[P1] Warrior Dream: Gorb", "Acantilados Aulladores", "Howling Cliffs", stBool(getBool(pd, "killedGhostAladar")), "1%"},
        {"[P3] Onirico: Anciano Hu", "[P3] Warrior Dream: Elder Hu", "Páramos Fúngicos", "Fungal Wastes", stBool(getBool(pd, "killedGhostHu")), "1%"},
        {"[P2] Onirico: Marmu", "[P2] Warrior Dream: Marmu", "Jardines de la Reina", "Queen's Gardens", stBool(getBool(pd, "killedGhostMarmu")), "1%"},
        {"[P4] Onirico: Sin Ojos", "[P4] Warrior Dream: No Eyes", "Santuario de Piedra", "Stone Sanctuary", stBool(getBool(pd, "killedGhostNoEyes")), "1%"},
        {"[P3] Onirico: Galien", "[P3] Warrior Dream: Galien", "Nido Profundo", "Deepnest", stBool(getBool(pd, "killedGhostGalien")), "1%"},
        {"[P4] Onirico: Markoth", "[P4] Warrior Dream: Markoth", "Límite del Reino", "Kingdom's Edge", stBool(getBool(pd, "killedGhostMarkoth")), "1%"},
        makeHeader("--- COLISEO DE LOS INSENSATOS (3%) ---", "--- COLOSSEUM OF FOOLS (3%) ---"),
        {"Coliseo: Prueba Guerrero", "Colosseum: Trial of Warrior", "Coliseo de los Insensatos", "Colosseum of Fools", stBool(getBool(pd, "colosseumBronzeCompleted")), "1%"},
        {"Coliseo: Prueba Conquistador", "Colosseum: Trial of Conqueror", "Coliseo de los Insensatos", "Colosseum of Fools", stBool(getBool(pd, "colosseumSilverCompleted")), "1%"},
        {"Coliseo: Prueba Insensato", "Colosseum: Trial of Fool", "Coliseo de los Insensatos", "Colosseum of Fools", stBool(getBool(pd, "colosseumGoldCompleted")), "1%"},
        makeHeader("--- EQUIPO Y HABILIDADES (14%) ---", "--- EQUIPMENT & ABILITIES (14%) ---"),
        {"Equipo: Capa Ala de Polilla", "Equip: Mothwing Cloak", "Sendero Verde", "Greenpath", stBool(getBool(pd, "hasDash")), "2%"},
        {"Equipo: Garra de Mantis", "Equip: Mantis Claw", "Aldea Mantis", "Mantis Village", stBool(getBool(pd, "hasWalljump")), "2%"},
        {"Equipo: Corazón de Cristal", "Equip: Crystal Heart", "Cumbre de Cristal", "Crystal Peak", stBool(getBool(pd, "hasSuperDash")), "2%"},
        {"Equipo: Alas de Monarca", "Equip: Monarch Wings", "Cuenca Antigua", "Ancient Basin", stBool(getBool(pd, "hasDoubleJump")), "2%"},
        {"Equipo: Lágrima de Isma", "Equip: Isma's Tear", "Arboleda de Isma", "Isma's Grove", stBool(getBool(pd, "hasAcidArmour")), "2%"},
        {"Equipo: Capa Sombría", "Equip: Shade Cloak", "El Abismo", "The Abyss", stBool(getBool(pd, "hasShadowDash")), "2%"},
        {"Equipo: Marca del Rey", "Equip: King's Brand", "Límite del Reino", "Kingdom's Edge", stBool(getBool(pd, "hasKingsBrand")), "2%"},
        makeHeader("--- AGUIJÓN Y ARTES (7%) ---", "--- NAIL UPGRADES & ARTS (7%) ---"),
        {"Aguijón Afilado (+1)", "Sharpened Nail (+1)", "Forjaguijones (250 Geo)", "Nailsmith (250 Geo)", stBool(nailUp >= 1), "1%"},
        {"Aguijón Estilizado (+2)", "Channelled Nail (+2)", "Forjaguijones (800 Geo + 1 Ore)", "Nailsmith (800 Geo + 1 Ore)", stBool(nailUp >= 2), "1%"},
        {"Aguijón Espiral (+3)", "Coiled Nail (+3)", "Forjaguijones (2000 Geo + 2 Ore)", "Nailsmith (2000 Geo + 2 Ore)", stBool(nailUp >= 3), "1%"},
        {"Aguijón Puro (+4)", "Pure Nail (+4)", "Forjaguijones (4000 Geo + 3 Ore)", "Nailsmith (4000 Geo + 3 Ore)", stBool(nailUp >= 4), "1%"},
        {"Arte: Gran Corte", "Nail Art: Great Slash", "Maestro Sheo", "Nailmaster Sheo", stBool(getBool(pd, "hasUpwardSlash")), "1%"},
        {"Arte: Corte Ciclón", "Nail Art: Cyclone Slash", "Maestro Mato", "Nailmaster Mato", stBool(getBool(pd, "hasCyclone")), "1%"},
        {"Arte: Corte Veloz", "Nail Art: Dash Slash", "Maestro Oro (800 Geo)", "Nailmaster Oro (800 Geo)", stBool(getBool(pd, "hasDashSlash")), "1%"},
        makeHeader("--- HECHIZOS (6%) ---", "--- SPELLS (6%) ---"),
        {"Hechizo: Espíritu Vengativo", "Spell: Vengeful Spirit", "Montículo Ancestral", "Ancestral Mound", stBool(getInt(pd, "fireballLevel") >= 1), "1%"},
        {"Hechizo: Alma Sombría", "Spell: Shade Soul", "Santuario de Almas", "Soul Sanctum", stBool(getInt(pd, "fireballLevel") >= 2), "1%"},
        {"Hechizo: Salto Desolador", "Spell: Desolate Dive", "Santuario de Almas", "Soul Sanctum", stBool(getInt(pd, "quakeLevel") >= 1), "1%"},
        {"Hechizo: Oscuridad Descendente", "Spell: Descending Dark", "Túmulo Cristalizado", "Crystallized Mound", stBool(getInt(pd, "quakeLevel") >= 2), "1%"},
        {"Hechizo: Espectros Aulladores", "Spell: Howling Wraiths", "Túmulo Descuidado", "Overgrown Mound", stBool(getInt(pd, "screamLevel") >= 1), "1%"},        {"Hechizo: Chillido del Abismo", "Spell: Abyss Shriek",
         "El Abismo", "The Abyss",
         stBool(getInt(pd, "screamLevel") >= 2), "1%"},
        makeHeader("--- AGUIJÓN ONÍRICO Y SONADORES (7%) ---", "--- DREAM NAIL & DREAMERS (7%) ---"),
        {"Aguijón Onírico", "Dream Nail",
         "Tierras de Reposo", "Resting Grounds",
         stBool(getBool(pd, "hasDreamNail")), "1%"},
        {"Aguijón Onírico Despertado", "Awoken Dream Nail",
         "La Vidente (1800 Esencia)", "The Seer (1800 Essence)",
         stBool(getBool(pd, "dreamNailUpgraded")), "1%"},
        {"Ascensión de la Vidente", "Seer Ascension",
         "La Vidente (2400 Esencia)", "The Seer (2400 Essence)",
         stBool(getBool(pd, "mothDeparted")), "1%"},
        {"Afinador de Dioses", "Godtuner",
         "Foso de Basura", "Junk Pit",
         stBool(getBool(pd, "hasGodfinder")), "1%"},
        {"Sonador: Lurien", "Dreamer: Lurien",
         "Torre del Vigia", "Watcher's Spire",
         stBool(getBool(pd, "lurienDefeated")), "1%"},
        {"Sonador: Monomon", "Dreamer: Monomon",
         "Archivos de la Maestra", "Teacher's Archives",
         stBool(getBool(pd, "monomonDefeated")), "1%"},
        {"Sonador: Herrah", "Dreamer: Herrah",
         "Guarida de la Bestia", "Beast's Den",
         stBool(getBool(pd, "herrahDefeated")), "1%"}
    };

        res.essentials112.push_back(makeHeader("--- FRAGMENTOS DE MÁSCARA (16 = 4%) ---", "--- MASK SHARDS (16 = 4%) ---"));
    // Contar cuantos fragmentos de mundo estan activados en sceneData para deducir los de Sly/Vidente
    int worldShardsDone = 0;
    for (const auto& ms : WEB_MASK_SHARDS) {
        if (!ms.scene.empty() && boolItems.count(ms.scene + "|" + ms.id) > 0) worldShardsDone++;
    }
    int vendorShardsDone = std::max(0, maskShards - worldShardsDone);
    int vShardIdx = 0;
    for (const auto& ms : WEB_MASK_SHARDS) {
        bool done = false;
        if (!ms.scene.empty()) {
            done = (boolItems.count(ms.scene + "|" + ms.id) > 0);
        } else {
            vShardIdx++;
            done = (vendorShardsDone >= vShardIdx);
        }
        res.essentials112.push_back({ms.nameES, ms.name, getLoc(ms.spoilerES, ms.spoiler), ms.spoiler, stBool(done), "0.25%"});
    }

    res.essentials112.push_back(makeHeader("--- FRAGMENTOS DE VASIJA (9 = 3%) ---", "--- VESSEL FRAGMENTS (9 = 3%) ---"));
    int worldVesselsDone = 0;
    for (const auto& vf : WEB_VESSEL_FRAGS) {
        if (!vf.scene.empty() && boolItems.count(vf.scene + "|" + vf.id) > 0) worldVesselsDone++;
    }
    int vendorVesselsDone = std::max(0, vesselFrags - worldVesselsDone);
    int vVesIdx = 0;
    for (const auto& vf : WEB_VESSEL_FRAGS) {
        bool done = false;
        if (!vf.scene.empty()) {
            done = (boolItems.count(vf.scene + "|" + vf.id) > 0);
        } else {
            vVesIdx++;
            done = (vendorVesselsDone >= vVesIdx);
        }
        res.essentials112.push_back({vf.nameES, vf.name, getLoc(vf.spoilerES, vf.spoiler), vf.spoiler, stBool(done), "0.33%"});
    }

    res.essentials112.push_back(makeHeader("--- AMULETOS (40 = 40%) ---", "--- CHARMS (40 = 40%) ---"));
    for (size_t i = 0; i < WEB_CHARMS.size(); ++i) {
        const auto& ch = WEB_CHARMS[i];
        int cNum = (int)i + 1;
        bool hasC = getBool(pd, "gotCharm_" + std::to_string(cNum));
        ItemState st = stBool(hasC);
        if (!hasC && cNum == 38 && getBool(pd, "killedNightmareGrimm")) st = ItemState::Unavailable;
        if (!hasC && cNum == 40 && getBool(pd, "destroyedNightmareLantern")) st = ItemState::Unavailable;
        res.essentials112.push_back({ch.nameES, ch.name, getLoc(ch.spoilerES, ch.spoiler), ch.spoiler, st, "1%"});
    }

        // 2. Essentials % (4 subsecciones reales de la web)
        // Nota: reutilizamos essentials112 para MAIN (112%) y creamos las listas exactas de Essentials % y Achievements
    std::vector<CheckItem> essTabItems;
    auto pushEssTab = [&](const std::string& hES, const std::string& hEN,
                          const std::vector<RawEssEntry>& vec) {
        essTabItems.push_back(makeHeader(hES, hEN));
        for (const auto& item : vec) {
            bool done = evalRawEss(item, pd, boolItems);
            essTabItems.push_back({item.nameES, item.name, getLoc(item.spoilerES, item.spoiler), item.spoiler, stBool(done), "Ess."});
        }
    };
    pushEssTab("--- ESENCIALES %: COLECCIONABLES ---", "--- GAME COMPLETION % ESSENTIALS - COLLECTIBLES ---", WEB_ESS_COLL);
    pushEssTab("--- ESENCIALES %: ESTACIONES DE CIERVO ---", "--- GAME COMPLETION % ESSENTIALS - STAG STATIONS ---", WEB_ESS_STAGS);
    pushEssTab("--- ESENCIALES %: INTERACCIONES DEL MUNDO ---", "--- GAME COMPLETION % ESSENTIALS - WORLD INTERACTIONS ---", WEB_ESS_WORLD);
    pushEssTab("--- ESENCIALES %: JEFES ---", "--- GAME COMPLETION % ESSENTIALS - BOSSES ---", WEB_ESS_BOSSES);

    res.essentialsExtra = std::move(essTabItems);
    res.achievementsHints.clear();
    auto pushAchTab = [&](const std::string& hES, const std::string& hEN,
                          const std::vector<RawEssEntry>& vec) {
        res.achievementsHints.push_back(makeHeader(hES, hEN));
        for (const auto& item : vec) {
            bool done = evalRawEss(item, pd, boolItems);
            res.achievementsHints.push_back({item.nameES, item.name, getLoc(item.spoilerES, item.spoiler), item.spoiler, stBool(done), "Ach."});
        }
    };
    pushAchTab("--- LOGROS: COLECCIONABLES ---", "--- ACHIEVEMENTS ESSENTIALS - COLLECTIBLES ---", WEB_ACH_COLL);
    pushAchTab("--- LOGROS: MAPAS ---", "--- ACHIEVEMENTS ESSENTIALS - MAPS ---", WEB_ACH_MAPS);
    pushAchTab("--- LOGROS: INTERACCIONES DEL MUNDO ---", "--- ACHIEVEMENTS ESSENTIALS - WORLD INTERACTIONS ---", WEB_ACH_WORLD);
    pushAchTab("--- LOGROS: JEFES ---", "--- ACHIEVEMENTS ESSENTIALS - BOSSES ---", WEB_ACH_BOSSES);

    // 3. Diario del Cazador real desde WEB_JOURNAL_BASE y WEB_JOURNAL_OPT
    bool zoteNeglect = getBool(pd, "zoteDead") ||
                       (!getBool(pd, "zoteRescuedBuzzer") && getBool(pd, "hasWalljump"));
    res.hunterJournal.push_back(makeHeader("--- DIARIO DEL CAZADOR: BASE (146) ---", "--- HUNTER'S JOURNAL: BASE (146) ---"));
    for (const auto& je : WEB_JOURNAL_BASE) {
        if (je.key == "ShadeJournal") {
            res.hunterJournal.push_back({
                je.nameES, je.name, getLoc(je.spoilerES, je.spoiler), je.spoiler, ItemState::Done, "Base"
            });
            continue;
        }
        int killsLeft = getInt(pd, "kills" + je.key, -1);
        bool killed = getBool(pd, "killed" + je.key);
        ItemState st = ItemState::Missing;
        std::string badge = "Base";

        if ((je.key == "GreyPrince" ||
             je.key.find("Zoteling") != std::string::npos) && zoteNeglect) {
            st = ItemState::Unavailable;
        } else if (killsLeft == 0) {
            st = ItemState::Done;
        } else if (killed && killsLeft > 0) {
            st = ItemState::Partial;
            badge = "Faltan " + std::to_string(killsLeft);
        }
        res.hunterJournal.push_back({
            je.nameES, je.name, getLoc(je.spoilerES, je.spoiler), je.spoiler, st, badge
        });
    }
    res.hunterJournal.push_back(makeHeader("--- ENTRADAS OPCIONALES ---", "--- OPTIONAL JOURNAL ENTRIES ---"));
    for (const auto& je : WEB_JOURNAL_OPT) {
        int killsLeft = getInt(pd, "kills" + je.key, -1);
        bool killed = getBool(pd, "killed" + je.key);
        ItemState st = (killsLeft == 0 || killed)
            ? ItemState::Done : ItemState::Unavailable;
        res.hunterJournal.push_back({
            je.nameES, je.name, getLoc(je.spoilerES, je.spoiler), je.spoiler, st, "Opt."
        });
    }

    // 4. Coleccionables reales (46 Larvas + 15 Raices + Reliquias individuales)
    bool grub32Done = false;
    res.collectibles.push_back(makeHeader("--- LARVAS RESCATADAS (46) ---", "--- GRUBS (46) ---"));
    for (const auto& g : WEB_GRUBS) {
        bool done = boolItems.count(g.scene + "|" + g.id) > 0;
        if (g.key == "grub32" && done) grub32Done = true;
        if ((g.key == "grub33" || g.key == "grub34") && grub32Done) done = true;
        res.collectibles.push_back({
            g.nameES, g.name, getLoc(g.spoilerES, g.spoiler), g.spoiler, stBool(done), "Grub"
        });
    }
    res.collectibles.push_back(makeHeader("--- RAÍCES SUSURRANTES (15) ---", "--- WHISPERING ROOTS (15) ---"));
    for (const auto& r : WEB_ROOTS) {
        bool done = boolItems.count(r.scene + "|" + r.id) > 0;
        res.collectibles.push_back({
            r.nameES, r.name, getLoc(r.spoilerES, r.spoiler), r.spoiler, stBool(done), "Root"
        });
    }
    auto addRelics = [&](const std::vector<RawSceneEntry>& vec, const std::string& tag) {
        for (const auto& rel : vec) {
            bool done = boolItems.count(rel.scene + "|" + rel.id) > 0;
            ItemState st = stBool(done);
            if (!done && rel.key == "arcaneEgg4" && getBool(pd, "gotCharm_9"))
                st = ItemState::Unavailable;
            res.collectibles.push_back({
                rel.nameES, rel.name, getLoc(rel.spoilerES, rel.spoiler), rel.spoiler, st, tag
            });
        }
    };
    res.collectibles.push_back(makeHeader("--- RELIQUIAS: DIARIOS DEL ERRANTE (14) ---", "--- RELICS: WANDERER'S JOURNALS (14) ---"));
    addRelics(WEB_RELICS_JOURNALS, "Relic");
    res.collectibles.push_back(makeHeader("--- RELIQUIAS: SELLOS DE HALLOWNEST (17) ---", "--- RELICS: HALLOWNEST SEALS (17) ---"));
    addRelics(WEB_RELICS_SEALS, "Relic");
    res.collectibles.push_back(makeHeader("--- RELIQUIAS: ÍDOLOS DEL REY (8) ---", "--- RELICS: KING'S IDOLS (8) ---"));
    addRelics(WEB_RELICS_IDOLS, "Relic");
    res.collectibles.push_back(makeHeader("--- RELIQUIAS: HUEVOS ARCANOS (4) ---", "--- RELICS: ARCANE EGGS (4) ---"));
    addRelics(WEB_RELICS_EGGS, "Relic");
    res.collectibles.push_back(makeHeader("--- HUEVOS PODRIDOS ---", "--- RANCID EGGS ---"));
    for (const auto& re : WEB_RANCID_EGGS) {
        bool done = (!re.scene.empty() && boolItems.count(re.scene + "|" + re.id) > 0);
        res.collectibles.push_back({re.nameES, re.name, getLoc(re.spoilerES, re.spoiler), re.spoiler, stBool(done), "Egg"});
    }


    // 5. Alijos de Geo reales (Cofres + 205 Rocas individuales)
    res.geoCaches.push_back(makeHeader("--- COFRES DE GEO ---", "--- GEO CHESTS ---"));
    for (const auto& ch : WEB_GEO_CHESTS) {
        bool done = boolItems.count(ch.scene + "|" + ch.id) > 0;
        res.geoCaches.push_back({
            ch.nameES, ch.name, getLoc(ch.spoilerES, ch.spoiler), ch.spoiler, stBool(done), "Chest"
        });
    }
    res.geoCaches.push_back(makeHeader("--- ROCAS DE GEO (207) ---", "--- GEO ROCKS (207) ---"));
    for (const auto& rk : WEB_GEO_ROCKS) {
        bool done = brokenRocks.count(rk.scene + "|" + rk.id) > 0;
        res.geoCaches.push_back({
            rk.nameES, rk.name, getLoc(rk.spoilerES, rk.spoiler), rk.spoiler, stBool(done), "Rock"
        });
    }

    // 6. Secretos
    int mrM = getInt(pd, "mrMushroomState", 0);
    res.secrets = {
        {"Señor Seta", "Mr. Mushroom", "7 ubicaciones con Hongo Esporas",
         "7 locations with Spore Shroom", stBool(mrM >= 8),
         std::to_string(std::min(mrM, 7)) + "/7"},
        {"Sendero del Dolor", "Path of Pain", "Palacio Blanco", "White Palace",
         stBool(getBool(pd, "visitedPathOfPain")), "Secret"},
        {"Fabricante de máscaras desmascarado", "Mask Maker Unmasked",
         "Nido Profundo", "Deepnest",
         stBool(getBool(pd, "maskMakerUnmasked")), "Secret"},
        {"Sentido del Mundo", "World Sense", "Templo Huevo Negro",
         "Temple of the Black Egg",
         stBool(getBool(pd, "unlockedCompletionRate")), "100%"}
    };

        res.godhome.clear();
    res.godhome.push_back(makeHeader("--- PANTEONES (P1 - P5) ---", "--- PANTHEONS (P1 - P5) ---"));
    for (int p = 1; p <= 5; ++p) {
        std::string key = "bossDoorStateTier" + std::to_string(p);
        std::string pName = (p == 5) ? "Pantheon 5: Hallownest" : ("Pantheon " + std::to_string(p));
        CheckItem ci{pName, pName, "Godhome", "Godhome",
                     stBool(getObjBool(pd, key, "completed")), "P" + std::to_string(p),
                     getObjBool(pd, key, "allBindings") ? 3 : 1};
        ci.bindNail = getObjBool(pd, key, "boundNail");
        ci.bindShell = getObjBool(pd, key, "boundShell");
        ci.bindCharms = getObjBool(pd, key, "boundCharms");
        ci.bindSoul = getObjBool(pd, key, "boundSoul");
        res.godhome.push_back(ci);
    }
    res.godhome.push_back(makeHeader("--- SALÓN DE LOS DIOSES (44 ESTATUAS) ---", "--- HALL OF GODS (44 STATUES) ---"));
    for (const auto& hg : WEB_HALL_OF_GODS) {
        std::string stKey = "statueState" + hg.id;
        int rank = getStatueRank(pd, stKey);
        bool unlocked = getObjBool(pd, stKey, "isUnlocked") || rank > 0;
        ItemState st = (rank > 0) ? ItemState::Done : (unlocked ? ItemState::Partial : ItemState::Missing);
        res.godhome.push_back({hg.nameES, hg.name, "Hall of Gods", "Hall of Gods", st, "Boss", rank});
    }


        res.geoRocksBroken = getInt(pd, "geoRocksBroken", (int)brokenRocks.size());
    res.pantheonsDone = 0;
    for (int p = 1; p <= 5; ++p) {
        if (getObjBool(pd, "bossDoorStateTier" + std::to_string(p), "completed"))
            res.pantheonsDone++;
    }
    res.godsUnlocked = 0; res.godsAttuned = 0; res.godsAscended = 0; res.godsRadiant = 0;
    for (const auto& hg : WEB_HALL_OF_GODS) {
        std::string stKey = "statueState" + hg.id;
        int r = getStatueRank(pd, stKey);
        if (getObjBool(pd, stKey, "isUnlocked") || r > 0) res.godsUnlocked++;
        if (r >= 1) res.godsAttuned++;
        if (r >= 2) res.godsAscended++;
        if (r >= 3) res.godsRadiant++;
    }
    int doneCount = 0, totalCount = 0;

    auto countList = [&](const std::vector<CheckItem>& v) {
        for (const auto& it : v) {
            if (it.state != ItemState::Unavailable) {
                totalCount++;
                if (it.state == ItemState::Done) doneCount++;
            }
        }
    };
    countList(res.essentials112);
    countList(res.hunterJournal);
    countList(res.collectibles);
    countList(res.geoCaches);
    countList(res.secrets);
    res.trueCompletionPercent = totalCount > 0
        ? (100.0f * (float)doneCount / (float)totalCount) : 0.0f;

    return res;
}

