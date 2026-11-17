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
    std::vector<CheckItem> essentialsAchievements;
    std::vector<CheckItem> hunterJournal;
    std::vector<CheckItem> collectibles;
    std::vector<CheckItem> geoCaches;
    std::vector<CheckItem> secrets;
    std::vector<CheckItem> godhome;
    std::vector<CheckItem> statistics;
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
        makeHeader("--- JEFES (14%) ---", "--- BOSSES (14%) ---"),
        {"[P1] Jefe: Falso Caballero", "[P1] Boss: False Knight", "Cruces Olvidados", "Forgotten Crossroads", stBool(getBool(pd, "killedFalseKnight")), "1%"},
        {"[P1] Jefe: Hornet Protectora", "[P1] Boss: Protector Hornet", "Sendero Verde", "Greenpath", stBool(getBool(pd, "killedHornet")), "1%"},
        {"[P1] Jefe: Madre Gruz", "[P1] Boss: Mother Gruz", "Cruces Olvidados", "Forgotten Crossroads", stBool(getBool(pd, "killedBigFly")), "1%"},
        {"[P1] Jefe: Mawlek Incubador", "[P1] Boss: Brooding Mawlek", "Cruces Olvidados", "Forgotten Crossroads", stBool(getBool(pd, "killedMawlek")), "1%"},
        {"[P2] Jefe: Señores Mantis", "[P2] Boss: Mantis Lords", "Aldea Mantis", "Mantis Village", stBool(getBool(pd, "defeatedMantisLords")), "1%"},
        {"[P1] Jefe: Maestro de Almas", "[P1] Boss: Soul Master", "Santuario de Almas", "Soul Sanctum", stBool(getBool(pd, "killedMageLord")), "1%"},
        {"[P1] Jefe: Defensor del Estiércol", "[P1] Boss: Dung Defender", "Canales Reales", "Royal Waterways", stBool(getBool(pd, "killedDungDefender")), "1%"},
        {"[P2] Jefe: Receptáculo Roto", "[P2] Boss: Broken Vessel", "Cuenca Antigua", "Ancient Basin", stBool(getBool(pd, "killedInfectedKnight")), "1%"},
        {"[P2] Jefe: Nosk", "[P2] Boss: Nosk", "Nido Profundo", "Deepnest", stBool(getBool(pd, "killedMimicSpider")), "1%"},
        {"[P3] Jefe: Centinela Hornet", "[P3] Boss: Hornet Sentinela", "Límite del Reino", "Kingdom's Edge", stBool(getBool(pd, "hornetOutskirtsDefeated")), "1%"},
        {"[P4] Jefe: Caballeros Vigías", "[P4] Boss: Watcher Knights", "Torre del Vigia", "Watcher's Spire", stBool(getBool(pd, "killedBlackKnight")), "1%"},
        {"[P3] Jefe: Uumuu", "[P3] Boss: Uumuu", "Archivos de la Maestra", "Teacher's Archives", stBool(getBool(pd, "killedMegaJellyfish")), "1%"},
        {"[P3] Jefe: El Coleccionista", "[P3] Boss: The Collector", "Torre del Amor", "Tower of Love", stBool(getBool(pd, "killedJarCollector")), "1%"},
        {"[P4] Jefe: Señor Desleal", "[P4] Boss: Traitor Lord", "Jardines de la Reina", "Queen's Gardens", stBool(getBool(pd, "killedTraitorLord")), "1%"},
        makeHeader("--- AMULETOS (40%) ---", "--- CHARMS (40%) ---"),
        makeHeader("--- EQUIPO (14%) ---", "--- EQUIPMENT (14%) ---"),
        {"Equipo: Capa Ala de Polilla", "Equip: Mothwing Cloak", "Sendero Verde", "Greenpath", stBool(getBool(pd, "hasDash")), "2%"},
        {"Equipo: Garra de Mantis", "Equip: Mantis Claw", "Aldea Mantis", "Mantis Village", stBool(getBool(pd, "hasWalljump")), "2%"},
        {"Equipo: Corazón de Cristal", "Equip: Crystal Heart", "Cumbre de Cristal", "Crystal Peak", stBool(getBool(pd, "hasSuperDash")), "2%"},
        {"Equipo: Alas de Monarca", "Equip: Monarch Wings", "Cuenca Antigua", "Ancient Basin", stBool(getBool(pd, "hasDoubleJump")), "2%"},
        {"Equipo: Lágrima de Isma", "Equip: Isma's Tear", "Arboleda de Isma", "Isma's Grove", stBool(getBool(pd, "hasAcidArmour")), "2%"},
        {"Equipo: Capa Sombría", "Equip: Shade Cloak", "El Abismo", "The Abyss", stBool(getBool(pd, "hasShadowDash")), "2%"},
        {"Equipo: Marca del Rey", "Equip: King's Brand", "Límite del Reino", "Kingdom's Edge", stBool(getBool(pd, "hasKingsBrand")), "2%"},
        makeHeader("--- MEJORAS DE AGUIJÓN (4%) ---", "--- NAIL UPGRADES (4%) ---"),
        {"Aguijón Afilado (+1)", "Sharpened Nail (+1)", "Forjaguijones (250 Geo)", "Nailsmith (250 Geo)", stBool(nailUp >= 1), "1%"},
        {"Aguijón Estilizado (+2)", "Channelled Nail (+2)", "Forjaguijones (800 Geo + 1 Ore)", "Nailsmith (800 Geo + 1 Ore)", stBool(nailUp >= 2), "1%"},
        {"Aguijón Espiral (+3)", "Coiled Nail (+3)", "Forjaguijones (2000 Geo + 2 Ore)", "Nailsmith (2000 Geo + 2 Ore)", stBool(nailUp >= 3), "1%"},
        {"Aguijón Puro (+4)", "Pure Nail (+4)", "Forjaguijones (4000 Geo + 3 Ore)", "Nailsmith (4000 Geo + 3 Ore)", stBool(nailUp >= 4), "1%"},
        makeHeader("--- ARTES DE AGUIJÓN (3%) ---", "--- NAIL ARTS (3%) ---"),
        {"Arte: Gran Corte", "Nail Art: Great Slash", "Maestro Sheo", "Nailmaster Sheo", stBool(getBool(pd, "hasUpwardSlash")), "1%"},
        {"Arte: Corte Ciclón", "Nail Art: Cyclone Slash", "Maestro Mato", "Nailmaster Mato", stBool(getBool(pd, "hasCyclone")), "1%"},
        {"Arte: Corte Veloz", "Nail Art: Dash Slash", "Maestro Oro (800 Geo)", "Nailmaster Oro (800 Geo)", stBool(getBool(pd, "hasDashSlash")), "1%"},
        makeHeader("--- HECHIZOS (6%) ---", "--- SPELLS (6%) ---"),
        {"Hechizo: Espíritu Vengativo", "Spell: Vengeful Spirit", "Montículo Ancestral", "Ancestral Mound", stBool(getInt(pd, "fireballLevel") >= 1), "1%"},
        {"Hechizo: Alma Sombría", "Spell: Shade Soul", "Santuario de Almas", "Soul Sanctum", stBool(getInt(pd, "fireballLevel") >= 2), "1%"},
        {"Hechizo: Salto Desolador", "Spell: Desolate Dive", "Santuario de Almas", "Soul Sanctum", stBool(getInt(pd, "quakeLevel") >= 1), "1%"},
        {"Hechizo: Oscuridad Descendente", "Spell: Descending Dark", "Túmulo Cristalizado", "Crystallized Mound", stBool(getInt(pd, "quakeLevel") >= 2), "1%"},
        {"Hechizo: Espectros Aulladores", "Spell: Howling Wraiths", "Túmulo Descuidado", "Overgrown Mound", stBool(getInt(pd, "screamLevel") >= 1), "1%"},
        {"Hechizo: Chillido del Abismo", "Spell: Abyss Shriek", "El Abismo", "The Abyss", stBool(getInt(pd, "screamLevel") >= 2), "1%"},
        makeHeader("--- FRAGMENTOS DE MÁSCARA (4%) ---", "--- MASK SHARDS (4%) ---"),
        makeHeader("--- FRAGMENTOS DE VASIJA (3%) ---", "--- VESSEL FRAGMENTS (3%) ---"),
        makeHeader("--- AGUIJÓN ONÍRICO Y ESENCIA (3%) ---", "--- DREAM NAIL & ESSENCE (3%) ---"),
        {"Aguijón Onírico", "Dream Nail", "Tierras de Reposo", "Resting Grounds", stBool(getBool(pd, "hasDreamNail")), "1%"},
        {"Aguijón Onírico Despertado", "Awoken Dream Nail", "La Vidente (1800 Esencia)", "The Seer (1800 Essence)", stBool(getBool(pd, "dreamNailUpgraded")), "1%"},
        {"Ascensión de la Vidente", "Seer Ascension", "La Vidente (2400 Esencia)", "The Seer (2400 Essence)", stBool(getBool(pd, "mothDeparted")), "1%"},
        makeHeader("--- GUERREROS ONÍRICOS (7%) ---", "--- WARRIOR DREAMS (7%) ---"),
        {"[P2] Onirico: Xero", "[P2] Warrior Dream: Xero", "Tierras de Reposo", "Resting Grounds", stBool(getBool(pd, "killedGhostXero")), "1%"},
        {"[P1] Onirico: Gorb", "[P1] Warrior Dream: Gorb", "Acantilados Aulladores", "Howling Cliffs", stBool(getBool(pd, "killedGhostAladar")), "1%"},
        {"[P3] Onirico: Anciano Hu", "[P3] Warrior Dream: Elder Hu", "Páramos Fúngicos", "Fungal Wastes", stBool(getBool(pd, "killedGhostHu")), "1%"},
        {"[P2] Onirico: Marmu", "[P2] Warrior Dream: Marmu", "Jardines de la Reina", "Queen's Gardens", stBool(getBool(pd, "killedGhostMarmu")), "1%"},
        {"[P4] Onirico: Sin Ojos", "[P4] Warrior Dream: No Eyes", "Santuario de Piedra", "Stone Sanctuary", stBool(getBool(pd, "killedGhostNoEyes")), "1%"},
        {"[P3] Onirico: Galien", "[P3] Warrior Dream: Galien", "Nido Profundo", "Deepnest", stBool(getBool(pd, "killedGhostGalien")), "1%"},
        {"[P4] Onirico: Markoth", "[P4] Warrior Dream: Markoth", "Límite del Reino", "Kingdom's Edge", stBool(getBool(pd, "killedGhostMarkoth")), "1%"},
        makeHeader("--- SONADORES (3%) ---", "--- DREAMERS (3%) ---"),
        {"Sonador: Lurien", "Dreamer: Lurien", "Torre del Vigia", "Watcher's Spire", stBool(getBool(pd, "lurienDefeated")), "1%"},
        {"Sonador: Monomon", "Dreamer: Monomon", "Archivos de la Maestra", "Teacher's Archives", stBool(getBool(pd, "monomonDefeated")), "1%"},
        {"Sonador: Herrah", "Dreamer: Herrah", "Guarida de la Bestia", "Beast's Den", stBool(getBool(pd, "herrahDefeated")), "1%"},
        makeHeader("--- COLISEO DE LOS INSENSATOS (3%) ---", "--- COLOSSEUM OF FOOLS (3%) ---"),
        {"Coliseo: Prueba Guerrero", "Colosseum: Trial of Warrior", "Coliseo de los Insensatos", "Colosseum of Fools", stBool(getBool(pd, "colosseumBronzeCompleted")), "1%"},
        {"Coliseo: Prueba Conquistador", "Colosseum: Trial of Conqueror", "Coliseo de los Insensatos", "Colosseum of Fools", stBool(getBool(pd, "colosseumSilverCompleted")), "1%"},
        {"Coliseo: Prueba Insensato", "Colosseum: Trial of Fool", "Coliseo de los Insensatos", "Colosseum of Fools", stBool(getBool(pd, "colosseumGoldCompleted")), "1%"},
        makeHeader("--- COMPAÑÍA GRIMM (2%) ---", "--- GRIMM TROUPE CONTENT PACK (2%) ---"),
        {"[P3] Maestro Compañía Grimm", "[P3] Troupe Master Grimm", "Bocasucia", "Dirtmouth", stBool(getBool(pd, "killedGrimm")), "1%"},
        {"Rey Pesadilla Grimm", "Nightmare King Grimm", "Bocasucia", "Dirtmouth", stBool(getBool(pd, "killedNightmareGrimm")), "1%"},
        makeHeader("--- PAQUETE DE CONTENIDO VIDA (1%) ---", "--- LIFEBLOOD CONTENT PACK (1%) ---"),
        {"Caballero Colmena", "Hive Knight", "La Colmena", "The Hive", stBool(getBool(pd, "killedHiveKnight")), "1%"},
        makeHeader("--- PAQUETE DE CONTENIDO GODMASTER (5%) ---", "--- GODMASTER CONTENT PACK (5%) ---"),
        {"Afinador de Dioses", "Godtuner", "Pila de Basura", "Junk Pit", stBool(getBool(pd, "hasGodfinder")), "1%"},
        {"P1 – Panteón del Maestro", "P1 – Pantheon of the Master", "Hogar de Dioses", "Godhome", stBool(getObjBool(pd, "bossDoorStateTier1", "unlocked")), "1%"},
        {"P2 – Panteón del Artista", "P2 – Pantheon of the Artist", "Hogar de Dioses", "Godhome", stBool(getObjBool(pd, "bossDoorStateTier2", "unlocked")), "1%"},
        {"P3 – Panteón del Sabio", "P3 – Pantheon of the Sage", "Hogar de Dioses", "Godhome", stBool(getObjBool(pd, "bossDoorStateTier3", "unlocked")), "1%"},
        {"P4 – Panteón del Caballero", "P4 – Pantheon of the Knight", "Hogar de Dioses", "Godhome", stBool(getObjBool(pd, "bossDoorStateTier4", "unlocked")), "1%"}
    };

    // Insert charms after the CHARMS header (index 15 in the initializer list)
    // Index 0: JEFES header, 1-14: 14 bosses, 15: AMULETOS header
    // (Godfinder Content Pack added after Lifeblood, but CHARMS header stays at 15)
    int charmsHeaderIdx = 15;
    for (size_t i = 0; i < WEB_CHARMS.size(); ++i) {
        const auto& ch = WEB_CHARMS[i];
        int cNum = (int)i + 1;
        bool hasC = getBool(pd, "gotCharm_" + std::to_string(cNum));
        ItemState st = stBool(hasC);
        if (!hasC && cNum == 38 && getBool(pd, "killedNightmareGrimm")) st = ItemState::Unavailable;
        if (!hasC && cNum == 40 && getBool(pd, "destroyedNightmareLantern")) st = ItemState::Unavailable;
        res.essentials112.insert(res.essentials112.begin() + charmsHeaderIdx + 1 + i, {ch.nameES, ch.name, getLoc(ch.spoilerES, ch.spoiler), ch.spoiler, st, "1%"});
    }

    // Insert mask shards after the MASK SHARDS header (index 40 in the initializer list, adjusted for charms inserted)
    // Original index 40 + 40 charms inserted = 80
    int maskShardsHeaderIdx = 40 + (int)WEB_CHARMS.size();
    // Contar cuantos fragmentos de mundo estan activados en sceneData para deducir los de Sly/Vidente
    int worldShardsDone = 0;
    for (const auto& ms : WEB_MASK_SHARDS) {
        if (!ms.scene.empty() && boolItems.count(ms.scene + "|" + ms.id) > 0) worldShardsDone++;
    }
    int vendorShardsDone = std::max(0, maskShards - worldShardsDone);
    int vShardIdx = 0;
    for (size_t mi = 0; mi < WEB_MASK_SHARDS.size(); ++mi) {
        const auto& ms = WEB_MASK_SHARDS[mi];
        bool done = false;
        if (!ms.scene.empty()) {
            done = (boolItems.count(ms.scene + "|" + ms.id) > 0);
        } else {
            vShardIdx++;
            done = (vendorShardsDone >= vShardIdx);
        }
        res.essentials112.insert(res.essentials112.begin() + maskShardsHeaderIdx + 1 + mi, {ms.nameES, ms.name, getLoc(ms.spoilerES, ms.spoiler), ms.spoiler, stBool(done), "0.25%"});
    }

    // Insert vessel fragments after the VESSEL FRAGMENTS header (index 41 in the initializer list, adjusted for charms + mask shards inserted)
    // Original index 41 + 40 charms + 16 mask shards = 97
    int vesselFragsHeaderIdx = 41 + (int)WEB_CHARMS.size() + (int)WEB_MASK_SHARDS.size();
    int worldVesselsDone = 0;
    for (const auto& vf : WEB_VESSEL_FRAGS) {
        if (!vf.scene.empty() && boolItems.count(vf.scene + "|" + vf.id) > 0) worldVesselsDone++;
    }
    int vendorVesselsDone = std::max(0, vesselFrags - worldVesselsDone);
    int vVesIdx = 0;
    for (size_t vi = 0; vi < WEB_VESSEL_FRAGS.size(); ++vi) {
        const auto& vf = WEB_VESSEL_FRAGS[vi];
        bool done = false;
        if (!vf.scene.empty()) {
            done = (boolItems.count(vf.scene + "|" + vf.id) > 0);
        } else {
            vVesIdx++;
            done = (vendorVesselsDone >= vVesIdx);
        }
        res.essentials112.insert(res.essentials112.begin() + vesselFragsHeaderIdx + 1 + vi, {vf.nameES, vf.name, getLoc(vf.spoilerES, vf.spoiler), vf.spoiler, stBool(done), "0.33%"});
    }

    // 2. Essentials + Achievements combinados (8 subsecciones en un solo vector)
    auto pushCombinedTab = [&](const std::string& hES, const std::string& hEN,
                               const std::vector<RawEssEntry>& vec, const std::string& badge) {
        res.essentialsAchievements.push_back(makeHeader(hES, hEN));
        for (const auto& item : vec) {
            bool done = evalRawEss(item, pd, boolItems);
            res.essentialsAchievements.push_back({item.nameES, item.name, getLoc(item.spoilerES, item.spoiler), item.spoiler, stBool(done), badge});
        }
    };
    pushCombinedTab("--- ESENCIALES %: COLECCIONABLES ---", "--- GAME COMPLETION % ESSENTIALS - COLLECTIBLES ---", WEB_ESS_COLL, "Ess.");
    pushCombinedTab("--- ESENCIALES %: ESTACIONES DE CIERVO ---", "--- GAME COMPLETION % ESSENTIALS - STAG STATIONS ---", WEB_ESS_STAGS, "Ess.");
    pushCombinedTab("--- ESENCIALES %: INTERACCIONES DEL MUNDO ---", "--- GAME COMPLETION % ESSENTIALS - WORLD INTERACTIONS ---", WEB_ESS_WORLD, "Ess.");
    pushCombinedTab("--- ESENCIALES %: JEFES ---", "--- GAME COMPLETION % ESSENTIALS - BOSSES ---", WEB_ESS_BOSSES, "Ess.");
    pushCombinedTab("--- LOGROS: COLECCIONABLES ---", "--- ACHIEVEMENTS ESSENTIALS - COLLECTIBLES ---", WEB_ACH_COLL, "Ach.");
    pushCombinedTab("--- LOGROS: MAPAS ---", "--- ACHIEVEMENTS ESSENTIALS - MAPS ---", WEB_ACH_MAPS, "Ach.");
    pushCombinedTab("--- LOGROS: INTERACCIONES DEL MUNDO ---", "--- ACHIEVEMENTS ESSENTIALS - WORLD INTERACTIONS ---", WEB_ACH_WORLD, "Ach.");
    pushCombinedTab("--- LOGROS: JEFES ---", "--- ACHIEVEMENTS ESSENTIALS - BOSSES ---", WEB_ACH_BOSSES, "Ach.");

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

    // 4. Coleccionables reales (Hendiduras para amuletos + 46 Larvas + 15 Raices + Reliquias + Huevos podridos + Objetos)
    bool grub32Done = false;
    res.collectibles.push_back(makeHeader("--- HENDIDURAS PARA AMULETOS (8) ---", "--- CHARM NOTCHES (8) ---"));
    res.collectibles.push_back({"Muesca de amuleto #1", "Charm Notch #1", "Páramos Fúngicos: Sala de Fungiogros", "Fungal Wastes: Shroom Ogres room", stBool(getBool(pd, "notchShroomOgres")), "Notch"});
    res.collectibles.push_back({"Muesca de amuleto #2", "Charm Notch #2", "Salubra: 120 Geo + 5 Amuletos", "Salubra: 120 Geo + 5 Charms", stBool(getBool(pd, "salubraNotch1")), "Notch"});
    res.collectibles.push_back({"Muesca de amuleto #3", "Charm Notch #3", "Salubra: 500 Geo + 10 Amuletos", "Salubra: 500 Geo + 10 Charms", stBool(getBool(pd, "salubraNotch2")), "Notch"});
    res.collectibles.push_back({"Muesca de amuleto #4", "Charm Notch #4", "Salubra: 900 Geo + 18 Amuletos", "Salubra: 900 Geo + 18 Charms", stBool(getBool(pd, "salubraNotch3")), "Notch"});
    res.collectibles.push_back({"Muesca de amuleto #5", "Charm Notch #5", "Salubra: 1400 Geo + 25 Amuletos", "Salubra: 1400 Geo + 25 Charms", stBool(getBool(pd, "salubraNotch4")), "Notch"});
    res.collectibles.push_back({"Muesca de amuleto #6", "Charm Notch #6", "Coliseo: Prueba del Guerrero", "Colosseum: Trial of Warrior", stBool(getBool(pd, "colosseumBronzeCompleted")), "Notch"});
    res.collectibles.push_back({"Muesca de amuleto #7", "Charm Notch #7", "Cañón de Niebla: Sala de huevos explosivos", "Fog Canyon: Explosive eggs room", stBool(getBool(pd, "notchFogCanyon")), "Notch"});
    res.collectibles.push_back({"Muesca de amuleto #8", "Charm Notch #8", "Bocasucia: Líder Grimm", "Dirtmouth: Troupe Leader Grimm", stBool(getBool(pd, "gotGrimmNotch")), "Notch"});
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
    res.collectibles.push_back(makeHeader("--- OBJETOS ---", "--- ITEMS ---"));
    res.collectibles.push_back({"Pluma", "Quill", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasQuill")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Banco", "Map Pin: Bench", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasPinBench")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Vendedor", "Map Pin: Vendor", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasPinShop")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Capullo de Saviavida", "Map Pin: Lifeblood Cocoon", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasPinCocoon")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Aguas termales", "Map Pin: Hot Springs", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasPinSpa")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Estación de Ciervo", "Map Pin: Stag Station", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasPinStag")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Tumba del Guerrero", "Map Pin: Warrior's Grave", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasPinGhost")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Raíz Susurrante", "Map Pin: Whispering Root", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasPinDreamPlant")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Tranvía", "Map Pin: Tram", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasPinTram")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Marcador de Concha", "Map Pin: Shell Marker", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasMarker_r")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Marcador de Escarabajo", "Map Pin: Scarab Marker", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasMarker_b")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Marcador de Moneda", "Map Pin: Token Marker", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasMarker_y")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Marcador Resplandeciente", "Map Pin: Gleaming Marker", "Cruces Olvidados: Tienda de Iselda", "Forgotten Crossroads: Iselda's Shop", stBool(getBool(pd, "hasMarker_w")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Sonadores", "Map Pin: Dreamers", "Descanso: Inspeccionar el santuario", "Resting Grounds: Inspect the shrine", stBool(getBool(pd, "hasPinGuardian")), "Item"});
    res.collectibles.push_back({"Pin de mapa: Templo del Huevo Negro", "Map Pin: Temple of the Black Egg", "Ciudad de Lágrimas: Inspeccionar fuente", "City of Tears: Inspect fountain", stBool(getBool(pd, "hasPinBlackEgg")), "Item"});
    res.collectibles.push_back({"Mapa del coleccionista", "Collector's Map", "Borde del Reino: Torre del Amor", "Kingdom's Edge: Tower of Love", stBool(getBool(pd, "hasPinGrub")), "Item"});
    res.collectibles.push_back({"Blasón de la Ciudad", "City Crest", "Cruces Olvidados: Recompensa Falso Caballero", "Forgotten Crossroads: False Knight Reward", stBool(getBool(pd, "cityCrest")), "Item"});
    res.collectibles.push_back({"Portal Onírico", "Dreamgate", "Vidente: 900 Esencia", "Seer: 900 Essence", stBool(getBool(pd, "hasDreamGate")), "Item"});
    res.collectibles.push_back({"Codicia irrompible", "Unbreakable Greed", "Divino: Codicia frágil + 9000 Geo", "Divine: Fragile Greed + 9000 Geo", stBool(getBool(pd, "fragileGreed_unbreakable")), "Item"});
    res.collectibles.push_back({"Corazón irrompible", "Unbreakable Heart", "Divino: Corazón frágil + 12000 Geo", "Divine: Fragile Heart + 12000 Geo", stBool(getBool(pd, "fragileHealth_unbreakable")), "Item"});
    res.collectibles.push_back({"Fuerza irrompible", "Unbreakable Strength", "Divino: Fuerza frágil + 15000 Geo", "Divine: Fragile Strength + 15000 Geo", stBool(getBool(pd, "fragileStrength_unbreakable")), "Item"});
    res.collectibles.push_back({"Linterna de lumélula", "Lumafly Lantern", "Cruces Olvidados", "Forgotten Crossroads", stBool(getBool(pd, "hasLantern")), "Item"});
    res.collectibles.push_back({"Linterna de Pesadilla", "Nightmare Lantern", "Acantilados Aulladores", "Howling Cliffs", stBool(getBool(pd, "nightmareLanternLit")), "Item"});
    res.collectibles.push_back({"Afinador de Dioses", "Godtuner", "Pila de Basura", "Junk Pit", stBool(getBool(pd, "hasGodfinder")), "Item"});


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

    // 6. Secretos (3 subsecciones: Interacciones del mundo, Salas secretas, Notas de Cornifer)
    int mrM = getInt(pd, "mrMushroomState", 0);
    res.secrets.clear();
    res.secrets.push_back(makeHeader("--- INTERACCIONES DEL MUNDO ---", "--- WORLD INTERACTIONS ---"));
    res.secrets.push_back({"Sentido del Mundo", "World Sense", "Templo del Huevo Negro", "Temple of the Black Egg", stBool(getBool(pd, "unlockedCompletionRate")), "Int."});
    res.secrets.push_back({"Insectos charlando", "Gossipping Bugs", "Cruces Olvidados: Aguas termales", "Forgotten Crossroads: Hot Springs", stBool(getBool(pd, "spaBugsEncountered")), "Int."});
    res.secrets.push_back({"Cueva de Jiji", "Confessor Jiji's Cave", "Bocasucia: Llave simple", "Dirtmouth: Simple Key", stBool(getBool(pd, "jijiDoorUnlocked")), "Int."});
    res.secrets.push_back({"Casa del Placer", "Pleasure House", "Ciudad de Lágrimas: Lado derecho", "City of Tears: Right side", stBool(getBool(pd, "bathHouseOpened")), "Int."});
    res.secrets.push_back({"Sumidero de Canales", "Waterways Manhole", "Ciudad de Lágrimas: Debajo de Lemm", "City of Tears: Below Lemm", stBool(getBool(pd, "openedWaterwaysManhole")), "Int."});
    res.secrets.push_back({"Claro de los Espíritus", "Spirits' Glade", "Tierras de Reposo: 200 Esencia", "Resting Grounds: 200 Essence", stBool(getBool(pd, "gladeDoorOpened")), "Int."});
    res.secrets.push_back({"Puerta de la Ciudad abierta", "City of Tears Gate", "Páramos Fúngicos: Blasón de la Ciudad", "Fungal Wastes: City Crest", stBool(getBool(pd, "openedCityGate")), "Int."});
    res.secrets.push_back({"Atajo al Santuario de Almas", "Soul Sanctum Shortcut", "Ciudad de Lágrimas: Sala del ascensor", "City of Tears: Cornifer Lift Room", stBool(getBool(pd, "soulSanctumShortcut")), "Int."});
    res.secrets.push_back({"Puerta de Canales abierta", "Waterways Gate", "Canales Reales: Sala de Cornifer", "Royal Waterways: Cornifer Room", stBool(getBool(pd, "waterwaysGate")), "Int."});
    res.secrets.push_back({"Candelabro caído", "Chandelier Dropped", "Ciudad de Lágrimas: Sala Caballeros Vigía", "City of Tears: Watcher Knights Room", stBool(getBool(pd, "watcherChandelier")), "Int."});
    res.secrets.push_back({"Aguas termales ocultas", "Hidden Hot Spring", "Coliseo: Romper pared del banco", "Colosseum: Break wall right of Bench", stBool(getBool(pd, "colosseumHiddenHotSpring")), "Int."});
    res.secrets.push_back({"Huevo del Nido de ciervo", "Stag Nest Egg", "Acantilados Aulladores: Nido de ciervo", "Howling Cliffs: Top of Stag Nest", stBool(getBool(pd, "stagEggInspected")), "Int."});
    res.secrets.push_back({"Puente del Nido Profundo", "Deepnest Entry Bridge", "Páramos Fúngicos: Izq. de Hongo Esporas", "Fungal Wastes: Left of Spore Shroom", stBool(getBool(pd, "deepnestBridgeCollapsed")), "Int."});
    res.secrets.push_back({"Hacedor de Máscaras", "Mask Maker Unmasked", "Nido Profundo: Salto Desolador", "Deepnest: Use Desolate Dive", stBool(getBool(pd, "maskmakerUnmasked1")), "Int."});
    res.secrets.push_back({"Cuenta bancaria abierta", "Bank Account Opened", "Cañón de Niebla: Millibelle", "Fog Canyon: Millibelle", stBool(getBool(pd, "bankerAccountPurchased")), "Int."});
    res.secrets.push_back({"Banquera desaparecida", "Banker Disappeared", "Cañón de Niebla: Millibelle", "Fog Canyon: Millibelle", stBool(getBool(pd, "millibelleLeft")), "Int."});
    res.secrets.push_back({"Puesto de la banquera", "Banker Stand Investigated", "Cañón de Niebla: Millibelle", "Fog Canyon: Millibelle", stBool(getBool(pd, "millibelleCheckedStand")), "Int."});
    res.secrets.push_back({"Millibelle encontrada", "Found & Talked to Millibelle", "Ciudad de Lágrimas: Aguas termales", "City of Tears: Hot Springs", stBool(getBool(pd, "bankerSpaMet")), "Int."});
    res.secrets.push_back({"Geo recuperado", "Reclaimed All Geo", "Ciudad de Lágrimas: Millibelle la ladrona", "City of Tears: Millibelle the Thief", stBool(getBool(pd, "millibelleReclaimedAllGeo")), "Int."});
    res.secrets.push_back({"Flor Delicada: Bicho anciano", "Delicate Flower: Elderbug", "Entregar desde Tumba del Hijo traidor", "Deliver from Traitor's Child Grave", stBool(getBool(pd, "elderbugGaveFlower")), "Int."});
    res.secrets.push_back({"Flor Delicada: Buscadora de Dioses", "Delicate Flower: Godseeker", "Después de 2 Panteones", "After completing 2 Pantheons", stBool(getBool(pd, "givenGodseekerFlower")), "Int."});
    res.secrets.push_back({"Flor Delicada: Maestro Oro", "Delicate Flower: Nailmaster Oro", "Entregar desde Tumba del Hijo traidor", "Deliver from Traitor's Child Grave", stBool(getBool(pd, "givenOroFlower")), "Int."});
    res.secrets.push_back({"Flor Delicada: Dama Blanca", "Delicate Flower: White Lady", "Entregar desde Tumba del Hijo traidor", "Deliver from Traitor's Child Grave", stBool(getBool(pd, "givenWhiteLadyFlower")), "Int."});
    res.secrets.push_back({"Flor Delicada: Emilitia", "Delicate Flower: Emilitia", "Entregar desde Tumba del Hijo traidor", "Deliver from Traitor's Child Grave", stBool(getBool(pd, "givenEmilitiaFlower")), "Int."});
    res.secrets.push_back({"Señor Seta", "Mr. Mushroom", "7 ubicaciones con Hongo Esporas", "7 locations with Spore Shroom", stBool(mrM >= 8), std::to_string(std::min(mrM, 7)) + "/7"});
    res.secrets.push_back(makeHeader("--- SALAS SECRETAS ---", "--- SECRET ROOMS ---"));
    res.secrets.push_back({"Tienda de Grimm: Sala secreta", "Grimm's Tent: Secret Room", "Bocasucia: Tienda de Grimm", "Dirtmouth: Grimm's Tent", stBool(getBool(pd, "grimmTentSecretRoom")), "Secret"});
    res.secrets.push_back({"Torre del Amor: Sala secreta", "Tower of Love: Secret Room", "Ciudad de Lágrimas: Cerca del Mapa", "City of Tears: Near Collector's Map", stBool(getBool(pd, "towerOfLoveSecretRoom")), "Secret"});
    res.secrets.push_back({"Guarida de Tejedoras: Sala secreta #1", "Weaver's Den: Secret Room #1", "Nido Profundo: Guarida de Tejedoras", "Deepnest: Weaver's Den", stBool(getBool(pd, "weaversDenSecretRoom1")), "Secret"});
    res.secrets.push_back({"Guarida de Tejedoras: Sala secreta #2", "Weaver's Den: Secret Room #2", "Nido Profundo: Guarida de Tejedoras", "Deepnest: Weaver's Den", stBool(getBool(pd, "weaversDenSecretRoom2")), "Secret"});
    res.secrets.push_back({"Palacio Blanco: Entrada Sendero del Dolor", "White Palace: Path of Pain Entrance", "Palacio Blanco: Romper pared", "White Palace: Break left wall", stBool(getBool(pd, "pathOfPainEntrance")), "Secret"});
    res.secrets.push_back({"Sendero del Dolor", "Path of Pain", "Palacio Blanco: Área secreta", "White Palace: Main Secret Area", stBool(getBool(pd, "killsBindingSeal")), "Secret"});
    res.secrets.push_back({"Palacio Blanco: Sala secreta #1", "White Palace: Secret Room #1", "Palacio Blanco: Taller del Rey Pálido", "White Palace: Pale King's Workshop", stBool(getBool(pd, "whitePalaceSecretRoomVisited")), "Secret"});
    res.secrets.push_back({"Palacio Blanco: Sala secreta #2", "White Palace: Secret Room #2", "Palacio Blanco: Sala de la Dama Blanca", "White Palace: White Lady's Room", stBool(getBool(pd, "whiteLadyRoom")), "Secret"});
    res.secrets.push_back({"Palacio Blanco: Sala secreta #3", "White Palace: Secret Room #3", "Palacio Blanco: Sala del Trono", "White Palace: Throne Room", stBool(getBool(pd, "throneRoomLoreTablet")), "Secret"});
    res.secrets.push_back(makeHeader("--- NOTAS DE CORNIFER (10) ---", "--- CORNIFER'S NOTES (10) ---"));
    res.secrets.push_back({"Nota #1: Cruces Olvidados", "Note #1: Forgotten Crossroads", "Debajo de la sala de Madre Gruz", "Below Big Gruzzer Room", stBool(getBool(pd, "corniferNote1")), "Note"});
    res.secrets.push_back({"Nota #2: Sendero Verde", "Note #2: Greenpath", "Sala debajo de la entrada de Cruces", "Room below Crossroads entrance", stBool(getBool(pd, "corniferNote2")), "Note"});
    res.secrets.push_back({"Nota #3: Páramos Fúngicos", "Note #3: Fungal Wastes", "Derecha de Estación de la Reina", "Right of Queen's Station", stBool(getBool(pd, "corniferNote3")), "Note"});
    res.secrets.push_back({"Nota #4: Acantilados Aulladores", "Note #4: Howling Cliffs", "Gran área principal a la izquierda", "Big Main Area on the left", stBool(getBool(pd, "corniferNote4")), "Note"});
    res.secrets.push_back({"Nota #5: Ciudad de Lágrimas", "Note #5: City of Tears", "Banco del peaje izq. del Santuario", "Toll Bench left of Soul Sanctum", stBool(getBool(pd, "corniferNote5")), "Note"});
    res.secrets.push_back({"Nota #6: Cumbre de Cristal", "Note #6: Crystal Peak", "Izquierda del Banco central", "Left of Central Bench", stBool(getBool(pd, "corniferNote6")), "Note"});
    res.secrets.push_back({"Nota #7: Tierras de Reposo", "Note #7: Resting Grounds", "Cerca de la Estación de ciervo", "Near the Stag Station", stBool(getBool(pd, "corniferNote7")), "Note"});
    res.secrets.push_back({"Nota #8: Canales Reales", "Note #8: Royal Waterways", "Cerca de la salida a Páramos", "Near Fungal Wastes exit", stBool(getBool(pd, "corniferNote8")), "Note"});
    res.secrets.push_back({"Nota #9: Cuenca Antigua", "Note #9: Ancient Basin", "Debajo del Tranvía y la fuente", "Below the Tram and Fountain", stBool(getBool(pd, "corniferNote9")), "Note"});
    res.secrets.push_back({"Nota #10: Límite del Reino", "Note #10: Kingdom's Edge", "Debajo de la entrada de Estación del Rey", "Below the King's Station entrance", stBool(getBool(pd, "corniferNote10")), "Note"});

    // 7. Godmaster (P1-P5 con vínculos + Salón de los Dioses)
    res.godhome.clear();
    const std::vector<std::string> pNamesES = {"", "P1 - Panteón del Maestro", "P2 - Panteón del Artista", "P3 - Panteón del Sabio", "P4 - Panteón del Caballero", "P5 - Panteón de Hallownest"};
    const std::vector<std::string> pNamesEN = {"", "P1 - Pantheon of the Master", "P2 - Pantheon of the Artist", "P3 - Pantheon of the Sage", "P4 - Pantheon of the Knight", "P5 - Pantheon of Hallownest"};
    for (int p = 1; p <= 5; ++p) {
        res.godhome.push_back(makeHeader("--- " + pNamesES[p] + " ---", "--- " + pNamesEN[p] + " ---"));
        std::string key = "bossDoorStateTier" + std::to_string(p);
        bool unlocked = getObjBool(pd, key, "unlocked");
        bool completed = getObjBool(pd, key, "completed");
        bool boundNail = getObjBool(pd, key, "boundNail");
        bool boundShell = getObjBool(pd, key, "boundShell");
        bool boundCharms = getObjBool(pd, key, "boundCharms");
        bool boundSoul = getObjBool(pd, key, "boundSoul");
        bool allBindings = getObjBool(pd, key, "allBindings");
        bool noHits = getObjBool(pd, key, "noHits");
        res.godhome.push_back({pNamesES[p] + ": Desbloqueado", pNamesEN[p] + ": Unlocked", "Godhome", "Godhome", stBool(unlocked), "P" + std::to_string(p)});
        res.godhome.push_back({pNamesES[p] + ": Completado", pNamesEN[p] + ": Completed", "Godhome", "Godhome", stBool(completed), "P" + std::to_string(p)});
        CheckItem nailCI{pNamesES[p] + ": Vínculo Aguijón", pNamesEN[p] + ": Binding Nail", "Godhome", "Godhome", stBool(boundNail), "P" + std::to_string(p)};
        nailCI.bindNail = true;
        res.godhome.push_back(nailCI);
        CheckItem shellCI{pNamesES[p] + ": Vínculo Cáscara", pNamesEN[p] + ": Binding Shell", "Godhome", "Godhome", stBool(boundShell), "P" + std::to_string(p)};
        shellCI.bindShell = true;
        res.godhome.push_back(shellCI);
        CheckItem charmsCI{pNamesES[p] + ": Vínculo Amuletos", pNamesEN[p] + ": Binding Charms", "Godhome", "Godhome", stBool(boundCharms), "P" + std::to_string(p)};
        charmsCI.bindCharms = true;
        res.godhome.push_back(charmsCI);
        CheckItem soulCI{pNamesES[p] + ": Vínculo Alma", pNamesEN[p] + ": Binding Soul", "Godhome", "Godhome", stBool(boundSoul), "P" + std::to_string(p)};
        soulCI.bindSoul = true;
        res.godhome.push_back(soulCI);
        res.godhome.push_back({pNamesES[p] + ": Todos los vínculos", pNamesEN[p] + ": All Bindings", "Godhome", "Godhome", stBool(allBindings), "P" + std::to_string(p)});
        res.godhome.push_back({pNamesES[p] + ": Sin daño", pNamesEN[p] + ": No Damage", "Godhome", "Godhome", stBool(noHits), "P" + std::to_string(p)});
    }
    res.godhome.push_back(makeHeader("--- SALÓN DE LOS DIOSES (176 ÍTEMS) ---", "--- HALL OF GODS (176 ITEMS) ---"));
    for (const auto& hg : WEB_HALL_OF_GODS) {
        std::string stKey = "statueState" + hg.id;
        bool isUnlocked = getObjBool(pd, stKey, "isUnlocked");
        bool completedTier1 = getObjBool(pd, stKey, "completedTier1");
        bool completedTier2 = getObjBool(pd, stKey, "completedTier2");
        bool completedTier3 = getObjBool(pd, stKey, "completedTier3");
        res.godhome.push_back({hg.nameES + ": Desbloqueado", hg.name + ": Unlocked", "Salón de los Dioses", "Hall of Gods", stBool(isUnlocked), "God"});
        res.godhome.push_back({hg.nameES + ": Sintonizado", hg.name + ": Attuned", "Salón de los Dioses", "Hall of Gods", stBool(completedTier1), "God"});
        res.godhome.push_back({hg.nameES + ": Ascendido", hg.name + ": Ascended", "Salón de los Dioses", "Hall of Gods", stBool(completedTier2), "God"});
        res.godhome.push_back({hg.nameES + ": Radiante", hg.name + ": Radiant", "Salón de los Dioses", "Hall of Gods", stBool(completedTier3), "God"});
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
    countList(res.essentialsAchievements);
    countList(res.hunterJournal);
    countList(res.collectibles);
    countList(res.geoCaches);
    countList(res.secrets);
    countList(res.godhome);
    res.trueCompletionPercent = totalCount > 0
        ? (100.0f * (float)doneCount / (float)totalCount) : 0.0f;


    // 8. Statistics (18 estadísticas reales del save file, según hk-database.js)
    res.statistics.clear();
    res.statistics.push_back(makeHeader("--- ESTADÍSTICAS DEL JUEGO ---", "--- GAME STATISTICS ---"));
    res.statistics.push_back({"Criaturas encontradas", "Creatures Encountered", "Diario del Cazador", "Hunter's Journal", ItemState::Done, std::to_string(getInt(pd, "journalEntriesCompleted", 0))});
    res.statistics.push_back({"Notas del Cazador completadas", "Hunter Notes Completed", "Diario del Cazador", "Hunter's Journal", ItemState::Done, std::to_string(getInt(pd, "journalNotesCompleted", 0))});
    res.statistics.push_back({"Daño base del aguijón", "Base Nail Damage", "Forjaguijones", "Nailsmith", ItemState::Done, std::to_string(getInt(pd, "nailDamage", 5))});
    res.statistics.push_back({"Geo en la Sombra", "Shade Geo", "Sombra", "Shade", ItemState::Done, std::to_string(getInt(pd, "geoPool", 0))});
    res.statistics.push_back({"Diarios de errante vendidos", "Wanderer's Journals Sold", "Banquera", "Banker", ItemState::Done, std::to_string(getInt(pd, "soldTrinket1", 0))});
    res.statistics.push_back({"Sellos de Hallownest vendidos", "Hallownest Seals Sold", "Banquera", "Banker", ItemState::Done, std::to_string(getInt(pd, "soldTrinket2", 0))});
    res.statistics.push_back({"Ídolos del rey vendidos", "King's Idols Sold", "Banquera", "Banker", ItemState::Done, std::to_string(getInt(pd, "soldTrinket3", 0))});
    res.statistics.push_back({"Huevos arcanos vendidos", "Arcane Eggs Sold", "Banquera", "Banker", ItemState::Done, std::to_string(getInt(pd, "soldTrinket4", 0))});
    res.statistics.push_back({"Geo total por reliquias vendidas", "Total Geo from Sold Relics", "Banquera", "Banker", ItemState::Done, std::to_string(getInt(pd, "relicsSoldTotalGeo", 0))});
    res.statistics.push_back({"Mineral pálido", "Pale Ore", "Recolectado", "Collected", ItemState::Done, std::to_string(getInt(pd, "ore", 0))});
    res.statistics.push_back({"Llaves simples", "Simple Keys", "Recolectado", "Collected", ItemState::Done, std::to_string(getInt(pd, "simpleKeys", 0))});
    res.statistics.push_back({"Huevos pútridos", "Rancid Eggs", "Recolectado", "Collected", ItemState::Done, std::to_string(getInt(pd, "rancidEggs", 0))});
    res.statistics.push_back({"Huevos pútridos vendidos", "Rancid Eggs Sold", "Jiji", "Jiji", ItemState::Done, std::to_string(getInt(pd, "jinnEggsSold", 0))});
    res.statistics.push_back({"Flores delicadas destruidas", "Delicate Flowers Destroyed", "Xun", "Xun", ItemState::Done, std::to_string(getInt(pd, "xunFlowerBrokeTimes", 0))});
    res.statistics.push_back({"Interactuables", "Interactables", "Exploración", "Exploration", ItemState::Done, std::to_string(getInt(pd, "itemsDiscovered", 0))});
    res.statistics.push_back({"Saldo en el banco", "Bank Account Balance", "Banquera", "Banker", ItemState::Done, std::to_string(getInt(pd, "bankerBalance", 0))});
    res.statistics.push_back({"Veces derrotado: Defensor Blanco", "White Defender Times Defeated", "Hogar de Dioses", "Godhome", ItemState::Done, std::to_string(getInt(pd, "whiteDefenderDefeats", 0)) + " / 5"});
    res.statistics.push_back({"Veces derrotado: Príncipe Gris Zote", "Grey Prince Zote Times Defeated", "Hogar de Dioses", "Godhome", ItemState::Done, std::to_string(getInt(pd, "greyPrinceDefeats", 0)) + " / 10"});

    return res;
}

