#include "hard.h"
#include "config.h"
#include "il2cpp.h"
#include <cstdio>
#include <cmath>

// 1. Crown Stealer Protection
typedef void (*t_PickupLoot)(void* thisPtr, void* loot, const void* mi);
static t_PickupLoot o_PickupLoot = nullptr;

static void hk_PickupLoot(void* thisPtr, void* loot, const void* mi) {
    if (g_config.noCrownStealing) {
        // Prevent CrownStealer from picking up monarch's crown
        return;
    }
    if (o_PickupLoot) o_PickupLoot(thisPtr, loot, mi);
}

// 2. EnemyManager Wave Spawning & Enemy Multipliers
// Verified exact method signature from Assembly-CSharp:
// Void AddEnemies(EnemyType type, AnimationCurve curve, Int32 targetDay, Single multiplier, List`1 list, String& log)
typedef void (*t_AddEnemies)(void* thisPtr, int type, void* curve, int targetDay, float multiplier, void* list, void** log, const void* mi);
static t_AddEnemies o_AddEnemies = nullptr;

// Verified exact method signature from Assembly-CSharp:
// Boolean SpawnWave(Wave wave, Int32 targetDay, Int32 multiplierDay, Int32 daysOnCurrentIsland, Side side, Boolean checkOtherSideIfNoneFound)
typedef bool (*t_SpawnWave)(void* thisPtr, void* wave, int targetDay, int multiplierDay, int daysOnIsland, int side, bool checkOther, const void* mi);
static t_SpawnWave o_SpawnWave = nullptr;

static size_t off_diffMult = 0;
static size_t off_diffBoss = 0;
static size_t off_diffCrush = 0;

static void hk_AddEnemies(void* thisPtr, int type, void* curve, int targetDay, float multiplier, void* list, void** log, const void* mi) {
    if (g_config.waveMultiplier > 0.05f) {
        multiplier *= g_config.waveMultiplier;
    }
    if (o_AddEnemies) {
        o_AddEnemies(thisPtr, type, curve, targetDay, multiplier, list, log, mi);
    }
}

static bool hk_SpawnWave(void* thisPtr, void* wave, int targetDay, int multiplierDay, int daysOnIsland, int side, bool checkOther, const void* mi) {
    if (thisPtr) {
        float threatFactor = g_config.directorThreatMult > 0.05f ? g_config.directorThreatMult : 1.0f;
        float waveFactor = g_config.waveMultiplier > 0.05f ? g_config.waveMultiplier : 1.0f;
        float combinedThreat = threatFactor * waveFactor;

        if (off_diffMult)  *(float*)((char*)thisPtr + off_diffMult)  = combinedThreat;
        if (off_diffBoss)  *(float*)((char*)thisPtr + off_diffBoss)  = combinedThreat;
        if (off_diffCrush) *(float*)((char*)thisPtr + off_diffCrush) = combinedThreat;

        if (waveFactor > 1.0f) {
            multiplierDay = (int)(multiplierDay * waveFactor + 0.5f);
        }
    }

    if (o_SpawnWave) {
        return o_SpawnWave(thisPtr, wave, targetDay, multiplierDay, daysOnIsland, side, checkOther, mi);
    }
    return false;
}

// 3. Portal Spawn Rate
static size_t off_portalInterval = 0;
static size_t off_portalDefense = 0;
typedef void (*t_PortalUpdate)(void* thisPtr, const void* mi);
static t_PortalUpdate o_PortalUpdate = nullptr;

static void hk_PortalUpdate(void* thisPtr, const void* mi) {
    if (thisPtr && g_config.portalSpawnRate > 0.05f) {
        if (off_portalInterval) {
            static float s_baseInterval = 0.0f;
            float cur = *(float*)((char*)thisPtr + off_portalInterval);
            if (s_baseInterval == 0.0f && cur > 0.0f) s_baseInterval = cur;
            if (s_baseInterval > 0.0f) {
                *(float*)((char*)thisPtr + off_portalInterval) = s_baseInterval / g_config.portalSpawnRate;
            }
        }
        if (off_portalDefense) {
            static float s_baseDefense = 0.0f;
            float curD = *(float*)((char*)thisPtr + off_portalDefense);
            if (s_baseDefense == 0.0f && curD > 0.0f) s_baseDefense = curD;
            if (s_baseDefense > 0.0f) {
                *(float*)((char*)thisPtr + off_portalDefense) = s_baseDefense / g_config.portalSpawnRate;
            }
        }
    }
    if (o_PortalUpdate) o_PortalUpdate(thisPtr, mi);
}

// 4. Greed Queen HP Scaling
typedef void (*t_QueenAwake)(void* thisPtr, const void* mi);
static t_QueenAwake o_QueenAwake = nullptr;
static void* g_typeDamageable = nullptr;
static size_t off_dmgInitHp = 0, off_dmgHp = 0;

static void hk_QueenAwake(void* thisPtr, const void* mi) {
    if (o_QueenAwake) o_QueenAwake(thisPtr, mi);
    if (thisPtr && g_config.greedQueenHPScale > 0.1f && g_config.greedQueenHPScale != 1.0f) {
        if (fn_get_gameObject && fn_GetComponent && g_typeDamageable) {
            void* go = fn_get_gameObject(thisPtr, mi_get_gameObject);
            if (go) {
                void* dmg = fn_GetComponent(go, g_typeDamageable, mi_GetComponent);
                if (dmg && off_dmgInitHp && off_dmgHp) {
                    int hp = *(int*)((char*)dmg + off_dmgInitHp);
                    if (hp > 0) {
                        int scaled = (int)(hp * g_config.greedQueenHPScale + 0.5f);
                        *(int*)((char*)dmg + off_dmgInitHp) = scaled;
                        *(int*)((char*)dmg + off_dmgHp) = scaled;
                    }
                }
            }
        }
    }
}

void InitHardFeatures() {
    // CrownStealer
    void* cStealer = FindClass("Assembly-CSharp", "", "CrownStealer");
    if (cStealer) {
        HookMethod(cStealer, "PickupLoot", 1, (void*)hk_PickupLoot, (void**)&o_PickupLoot);
    }

    // EnemyManager - Wave Size & Threat
    void* cEnemyMgr = FindClass("Assembly-CSharp", "", "EnemyManager");
    if (cEnemyMgr) {
        off_diffMult  = FieldOffset(cEnemyMgr, "difficultyMultiplier");
        off_diffBoss  = FieldOffset(cEnemyMgr, "difficultyMultiplierBosses");
        off_diffCrush = FieldOffset(cEnemyMgr, "difficultyMultiplierBBBCrushers");

        HookMethod(cEnemyMgr, "AddEnemies", 6, (void*)hk_AddEnemies, (void**)&o_AddEnemies);
        HookMethod(cEnemyMgr, "SpawnWave", 6, (void*)hk_SpawnWave, (void**)&o_SpawnWave);
        Log("HardMode: EnemyManager hooks installed (diffMult=0x%zx, diffBoss=0x%zx, diffCrush=0x%zx)",
            off_diffMult, off_diffBoss, off_diffCrush);
    }

    // Portal
    void* cPortal = FindClass("Assembly-CSharp", "", "Portal");
    if (cPortal) {
        off_portalInterval = FieldOffset(cPortal, "_spawnInterval");
        off_portalDefense  = FieldOffset(cPortal, "defenseSpawnCooldown");
        HookMethod(cPortal, "Update", 0, (void*)hk_PortalUpdate, (void**)&o_PortalUpdate);
        Log("HardMode: Portal hook installed (spawnInterval=0x%zx, defenseCooldown=0x%zx)",
            off_portalInterval, off_portalDefense);
    }

    // GreedQueen HP
    void* cQueen = FindClass("Assembly-CSharp", "", "GreedQueen");
    void* cDmg = FindClass("Assembly-CSharp", "", "Damageable");
    if (cDmg && il2cpp_class_get_type && il2cpp_type_get_object) {
        g_typeDamageable = il2cpp_type_get_object(il2cpp_class_get_type(cDmg));
        off_dmgInitHp = FieldOffset(cDmg, "initialHitPoints");
        off_dmgHp     = FieldOffset(cDmg, "hitPoints");
    }
    if (cQueen) {
        HookMethod(cQueen, "Awake", 0, (void*)hk_QueenAwake, (void**)&o_QueenAwake);
    }
}

void SetWaveMultiplier(float mult) {
    g_config.waveMultiplier = mult;
    Log("HardMode: Wave multiplier set to %.2fx", mult);
}

void SetEnemySpeedMultiplier(float mult) {
    g_config.enemySpeedMultiplier = mult;
    Log("HardMode: Enemy speed multiplier set to %.2fx", mult);
}
