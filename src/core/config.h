#pragma once
#include <windows.h>
#include "i18n.h"

struct ConfigSettings {
    Lang language = Lang::English;
    int menuKey = VK_F1;
    int mapKey = 'M';
    int staminaKey = 'N';

    bool enableMinimap = true;
    bool enableStaminaBar = true;
    bool enableHotkeys = true;
    bool boldFont = false;
    float uiScale = 1.0f; // Slider 1.0x - 2.5x (4K / Display Scaling)
    float monitorBgAlpha = 0.55f; // Translucent HUD alpha (0.2 - 1.0)

    // Movement & Player
    bool godMode = false;
    bool infiniteStamina = false;
    float speedMultiplier = 1.0f; // Slider 0.5x - 10.0x
    int speedIndex = 0;           // Quick cycle index
    float playerSize = 1.0f;      // Slider 0.2x - 3.0x

    // Economy & Cheats
    float coinScale = 0.7f;
    float bagScale = 1.0f;
    bool noToolCooldowns = false;
    int spawnUnitCount = 1;       // Slider 1 - 50
    bool spawnAtCampfire = true;  // Toggle: Campfire vs Monarch
    float coinIncomeMult = 1.0f;  // Passive coin income rate
    float bagDropMult = 1.0f;     // Bag drop rate multiplier

    // Artemis Bow
    int artemisArrowCount = 10;
    float artemisRange = 1.0f;
    float artemisDamage = 1.0f;

    // Construction & Builders
    bool hyperBuilders = false;
    float builderSpeedMult = 1.0f;
    float builderEfficiency = 1.0f;
    bool largerCamps = false;

    // World / Lab
    bool bloodMoonDisabled = false;
    bool indestructibleWalls = false;
    bool lockSummer = false;
    bool clearWeather = false;
    bool coinsStayDry = false;

    // Lab - Unit Boosts
    bool archerFireBoost = false;
    bool berserkerRage = false;
    bool ninjaSpeedBoost = false;
    bool eliteKnights = false;
    bool rapidCitizenHousing = false;

    // Lab - World Boosts
    bool farmOutputBoost = false;
    bool towerFireBoost = false;
    bool animalSpawnBoost = false;
    bool instantCastle = false;
    bool instantDaySkip = false;
    int recruitCap = 0;            // 0 = game default
    float treeRegrowthMult = 1.0f;

    // Lab - Ballista
    bool ballistaBoost = false;
    float ballistaReloadMult = 1.0f;
    float ballistaFlightMult = 1.0f;

    // Lab - Catapult
    bool catapultBoost = false;
    float catapultReloadMult = 1.0f;
    float catapultFlightMult = 1.0f;

    // Lab - Steed
    bool chargeDmgBoost = false;
    float buffAuraDuration = 1.0f;

    // Difficulty (Hard Mode)
    float waveMultiplier = 1.0f;
    float enemySpeedMultiplier = 1.0f;
    float portalSpawnRate = 1.0f;  // Fixed: was wrongly bound to builderSpeedMult
    float greedQueenHPScale = 1.0f;
    float directorThreatMult = 1.0f;
    bool noCrownStealing = false;
};

extern ConfigSettings g_config;

void LoadConfig();
void SaveConfig();
