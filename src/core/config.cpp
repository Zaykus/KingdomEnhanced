#include "config.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "il2cpp.h"

ConfigSettings g_config;
static char g_iniPath[MAX_PATH] = "KingdomEnhanced.ini";

static void GetIniPath() {
    char exe[MAX_PATH];
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    char* slash = strrchr(exe, '\\');
    if (slash) {
        strcpy(slash + 1, "KingdomEnhanced.ini");
        strcpy(g_iniPath, exe);
    }
}

void LoadConfig() {
    GetIniPath();
    char buf[64];

    // Language
    GetPrivateProfileStringA("General", "Language", "English", buf, sizeof(buf), g_iniPath);
    if (_stricmp(buf, "Chinese") == 0) {
        g_config.language = Lang::Chinese;
    } else if (_stricmp(buf, "Korean") == 0) {
        g_config.language = Lang::Korean;
    } else if (_stricmp(buf, "Spanish") == 0 || _stricmp(buf, "Espanol") == 0) {
        g_config.language = Lang::Spanish;
    } else if (_stricmp(buf, "German") == 0 || _stricmp(buf, "Deutsch") == 0) {
        g_config.language = Lang::German;
    } else if (_stricmp(buf, "French") == 0 || _stricmp(buf, "Francais") == 0) {
        g_config.language = Lang::French;
    } else {
        g_config.language = Lang::English;
    }
    SetLanguage(g_config.language);

    // Keybinds
    g_config.menuKey    = GetPrivateProfileIntA("Keybinds", "MenuKey", VK_F1, g_iniPath);
    g_config.mapKey     = GetPrivateProfileIntA("Keybinds", "MapKey", 'M', g_iniPath);
    g_config.staminaKey = GetPrivateProfileIntA("Keybinds", "StaminaKey", 'N', g_iniPath);

    // Modules
    g_config.enableMinimap    = GetPrivateProfileIntA("Modules", "EnableMinimap", 1, g_iniPath) != 0;
    g_config.enableStaminaBar = GetPrivateProfileIntA("Modules", "EnableStaminaBar", 1, g_iniPath) != 0;
    g_config.enableHotkeys    = GetPrivateProfileIntA("Modules", "EnableHotkeys", 1, g_iniPath) != 0;
    g_config.boldFont         = GetPrivateProfileIntA("Modules", "BoldFont", 0, g_iniPath) != 0;
    GetPrivateProfileStringA("Modules", "UIScale", "1.0", buf, sizeof(buf), g_iniPath);
    g_config.uiScale = (float)atof(buf);
    if (g_config.uiScale < 0.75f || g_config.uiScale > 3.0f) g_config.uiScale = 1.0f;

    GetPrivateProfileStringA("Modules", "MonitorAlpha", "0.55", buf, sizeof(buf), g_iniPath);
    g_config.monitorBgAlpha = (float)atof(buf);
    if (g_config.monitorBgAlpha < 0.1f || g_config.monitorBgAlpha > 1.0f) g_config.monitorBgAlpha = 0.55f;

    // Cheats
    g_config.godMode        = GetPrivateProfileIntA("Cheats", "GodMode", 0, g_iniPath) != 0;
    g_config.infiniteStamina = GetPrivateProfileIntA("Cheats", "InfiniteStamina", 0, g_iniPath) != 0;
    g_config.noToolCooldowns = GetPrivateProfileIntA("Cheats", "NoToolCooldowns", 0, g_iniPath) != 0;
    g_config.speedIndex     = GetPrivateProfileIntA("Cheats", "SpeedIndex", 0, g_iniPath);

    GetPrivateProfileStringA("Cheats", "SpeedMultiplier", "1.0", buf, sizeof(buf), g_iniPath);
    g_config.speedMultiplier = (float)atof(buf);
    if (g_config.speedMultiplier < 0.2f || g_config.speedMultiplier > 20.0f) g_config.speedMultiplier = 1.0f;

    GetPrivateProfileStringA("Cheats", "PlayerSize", "1.0", buf, sizeof(buf), g_iniPath);
    g_config.playerSize = (float)atof(buf);
    if (g_config.playerSize < 0.2f || g_config.playerSize > 5.0f) g_config.playerSize = 1.0f;

    g_config.spawnUnitCount = GetPrivateProfileIntA("Cheats", "SpawnUnitCount", 1, g_iniPath);
    if (g_config.spawnUnitCount < 1 || g_config.spawnUnitCount > 50) g_config.spawnUnitCount = 1;
    g_config.spawnAtCampfire = GetPrivateProfileIntA("Cheats", "SpawnAtCampfire", 1, g_iniPath) != 0;

    GetPrivateProfileStringA("Cheats", "CoinScale", "0.7", buf, sizeof(buf), g_iniPath);
    g_config.coinScale = (float)atof(buf);
    if (g_config.coinScale < 0.2f || g_config.coinScale > 2.0f) g_config.coinScale = 0.7f;

    // Construction
    g_config.hyperBuilders = GetPrivateProfileIntA("Construction", "HyperBuilders", 0, g_iniPath) != 0;
    GetPrivateProfileStringA("Construction", "BuilderSpeed", "1.0", buf, sizeof(buf), g_iniPath);
    g_config.builderSpeedMult = (float)atof(buf);
    if (g_config.builderSpeedMult < 0.1f || g_config.builderSpeedMult > 10.0f) g_config.builderSpeedMult = 1.0f;

    GetPrivateProfileStringA("Construction", "BuilderEfficiency", "1.0", buf, sizeof(buf), g_iniPath);
    g_config.builderEfficiency = (float)atof(buf);
    if (g_config.builderEfficiency < 0.001f || g_config.builderEfficiency > 5.0f) g_config.builderEfficiency = 1.0f;
    g_config.largerCamps = GetPrivateProfileIntA("Construction", "LargerCamps", 0, g_iniPath) != 0;

    // Lab & Hard
    g_config.bloodMoonDisabled    = GetPrivateProfileIntA("Lab", "BloodMoonDisabled", 0, g_iniPath) != 0;
    g_config.indestructibleWalls  = GetPrivateProfileIntA("Lab", "IndestructibleWalls", 0, g_iniPath) != 0;
    g_config.lockSummer           = GetPrivateProfileIntA("Lab", "LockSummer", 0, g_iniPath) != 0;
    g_config.clearWeather         = GetPrivateProfileIntA("Lab", "ClearWeather", 0, g_iniPath) != 0;
    g_config.coinsStayDry         = GetPrivateProfileIntA("Lab", "CoinsStayDry", 0, g_iniPath) != 0;
    g_config.archerFireBoost      = GetPrivateProfileIntA("Lab", "ArcherFireBoost", 0, g_iniPath) != 0;
    g_config.berserkerRage        = GetPrivateProfileIntA("Lab", "BerserkerRage", 0, g_iniPath) != 0;
    g_config.ninjaSpeedBoost      = GetPrivateProfileIntA("Lab", "NinjaSpeedBoost", 0, g_iniPath) != 0;
    g_config.eliteKnights         = GetPrivateProfileIntA("Lab", "EliteKnights", 0, g_iniPath) != 0;
    g_config.rapidCitizenHousing  = GetPrivateProfileIntA("Lab", "RapidCitizenHousing", 0, g_iniPath) != 0;
    g_config.farmOutputBoost      = GetPrivateProfileIntA("Lab", "FarmOutputBoost", 0, g_iniPath) != 0;
    g_config.towerFireBoost       = GetPrivateProfileIntA("Lab", "TowerFireBoost", 0, g_iniPath) != 0;
    g_config.animalSpawnBoost     = GetPrivateProfileIntA("Lab", "AnimalSpawnBoost", 0, g_iniPath) != 0;
    g_config.instantCastle        = GetPrivateProfileIntA("Lab", "InstantCastle", 0, g_iniPath) != 0;
    g_config.instantDaySkip       = GetPrivateProfileIntA("Lab", "InstantDaySkip", 0, g_iniPath) != 0;

    GetPrivateProfileStringA("Hard", "WaveMultiplier", "1.0", buf, sizeof(buf), g_iniPath);
    g_config.waveMultiplier = (float)atof(buf);
    if (g_config.waveMultiplier < 0.1f || g_config.waveMultiplier > 20.0f) g_config.waveMultiplier = 1.0f;

    GetPrivateProfileStringA("Hard", "EnemySpeedMultiplier", "1.0", buf, sizeof(buf), g_iniPath);
    g_config.enemySpeedMultiplier = (float)atof(buf);
    if (g_config.enemySpeedMultiplier < 0.1f || g_config.enemySpeedMultiplier > 10.0f) g_config.enemySpeedMultiplier = 1.0f;

    GetPrivateProfileStringA("Hard", "PortalSpawnRate", "1.0", buf, sizeof(buf), g_iniPath);
    g_config.portalSpawnRate = (float)atof(buf);
    if (g_config.portalSpawnRate < 0.1f || g_config.portalSpawnRate > 10.0f) g_config.portalSpawnRate = 1.0f;

    GetPrivateProfileStringA("Hard", "GreedQueenHPScale", "1.0", buf, sizeof(buf), g_iniPath);
    g_config.greedQueenHPScale = (float)atof(buf);
    if (g_config.greedQueenHPScale < 0.1f || g_config.greedQueenHPScale > 10.0f) g_config.greedQueenHPScale = 1.0f;

    GetPrivateProfileStringA("Hard", "DirectorThreatMult", "1.0", buf, sizeof(buf), g_iniPath);
    g_config.directorThreatMult = (float)atof(buf);
    if (g_config.directorThreatMult < 0.1f || g_config.directorThreatMult > 10.0f) g_config.directorThreatMult = 1.0f;

    g_config.noCrownStealing = GetPrivateProfileIntA("Hard", "NoCrownStealing", 0, g_iniPath) != 0;

    Log("Config loaded from %s", g_iniPath);
}

void SaveConfig() {
    GetIniPath();
    const char* langStr = "English";
    if (g_config.language == Lang::Chinese) langStr = "Chinese";
    else if (g_config.language == Lang::Korean) langStr = "Korean";
    else if (g_config.language == Lang::Spanish) langStr = "Spanish";
    else if (g_config.language == Lang::German) langStr = "German";
    else if (g_config.language == Lang::French) langStr = "French";

    WritePrivateProfileStringA("General", "Language", langStr, g_iniPath);

    char buf[64];
    #define WRITE_INT(sec, key, val) sprintf(buf, "%d", (int)(val)); WritePrivateProfileStringA(sec, key, buf, g_iniPath);
    #define WRITE_FLOAT(sec, key, val) sprintf(buf, "%.2f", (float)(val)); WritePrivateProfileStringA(sec, key, buf, g_iniPath);

    WRITE_INT("Keybinds", "MenuKey", g_config.menuKey);
    WRITE_INT("Keybinds", "MapKey", g_config.mapKey);
    WRITE_INT("Keybinds", "StaminaKey", g_config.staminaKey);

    WRITE_INT("Modules", "EnableMinimap", g_config.enableMinimap ? 1 : 0);
    WRITE_INT("Modules", "EnableStaminaBar", g_config.enableStaminaBar ? 1 : 0);
    WRITE_INT("Modules", "EnableHotkeys", g_config.enableHotkeys ? 1 : 0);
    WRITE_INT("Modules", "BoldFont", g_config.boldFont ? 1 : 0);
    WRITE_FLOAT("Modules", "UIScale", g_config.uiScale);
    WRITE_FLOAT("Modules", "MonitorAlpha", g_config.monitorBgAlpha);

    WRITE_INT("Cheats", "GodMode", g_config.godMode ? 1 : 0);
    WRITE_INT("Cheats", "InfiniteStamina", g_config.infiniteStamina ? 1 : 0);
    WRITE_INT("Cheats", "NoToolCooldowns", g_config.noToolCooldowns ? 1 : 0);
    WRITE_FLOAT("Cheats", "SpeedMultiplier", g_config.speedMultiplier);
    WRITE_FLOAT("Cheats", "PlayerSize", g_config.playerSize);
    WRITE_INT("Cheats", "SpawnUnitCount", g_config.spawnUnitCount);
    WRITE_INT("Cheats", "SpawnAtCampfire", g_config.spawnAtCampfire ? 1 : 0);
    WRITE_FLOAT("Cheats", "CoinScale", g_config.coinScale);

    WRITE_INT("Construction", "HyperBuilders", g_config.hyperBuilders ? 1 : 0);
    WRITE_FLOAT("Construction", "BuilderSpeed", g_config.builderSpeedMult);
    WRITE_FLOAT("Construction", "BuilderEfficiency", g_config.builderEfficiency);
    WRITE_INT("Construction", "LargerCamps", g_config.largerCamps ? 1 : 0);

    WRITE_INT("Lab", "BloodMoonDisabled", g_config.bloodMoonDisabled ? 1 : 0);
    WRITE_INT("Lab", "IndestructibleWalls", g_config.indestructibleWalls ? 1 : 0);
    WRITE_INT("Lab", "LockSummer", g_config.lockSummer ? 1 : 0);
    WRITE_INT("Lab", "ClearWeather", g_config.clearWeather ? 1 : 0);
    WRITE_INT("Lab", "CoinsStayDry", g_config.coinsStayDry ? 1 : 0);
    WRITE_INT("Lab", "ArcherFireBoost", g_config.archerFireBoost ? 1 : 0);
    WRITE_INT("Lab", "BerserkerRage", g_config.berserkerRage ? 1 : 0);
    WRITE_INT("Lab", "NinjaSpeedBoost", g_config.ninjaSpeedBoost ? 1 : 0);
    WRITE_INT("Lab", "EliteKnights", g_config.eliteKnights ? 1 : 0);
    WRITE_INT("Lab", "RapidCitizenHousing", g_config.rapidCitizenHousing ? 1 : 0);
    WRITE_INT("Lab", "FarmOutputBoost", g_config.farmOutputBoost ? 1 : 0);
    WRITE_INT("Lab", "TowerFireBoost", g_config.towerFireBoost ? 1 : 0);
    WRITE_INT("Lab", "AnimalSpawnBoost", g_config.animalSpawnBoost ? 1 : 0);
    WRITE_INT("Lab", "InstantCastle", g_config.instantCastle ? 1 : 0);
    WRITE_INT("Lab", "InstantDaySkip", g_config.instantDaySkip ? 1 : 0);

    WRITE_FLOAT("Hard", "WaveMultiplier", g_config.waveMultiplier);
    WRITE_FLOAT("Hard", "EnemySpeedMultiplier", g_config.enemySpeedMultiplier);
    WRITE_FLOAT("Hard", "PortalSpawnRate", g_config.portalSpawnRate);
    WRITE_FLOAT("Hard", "GreedQueenHPScale", g_config.greedQueenHPScale);
    WRITE_FLOAT("Hard", "DirectorThreatMult", g_config.directorThreatMult);
    WRITE_INT("Hard", "NoCrownStealing", g_config.noCrownStealing ? 1 : 0);

    #undef WRITE_INT
    #undef WRITE_FLOAT
    LogAction("Config", LogType::Success, "Saved settings to %s (Lang=%s, Bold=%s, Scale=%.2fx, Alpha=%.2f)", g_iniPath, langStr, g_config.boldFont ? "ON" : "OFF", g_config.uiScale, g_config.monitorBgAlpha);
}
