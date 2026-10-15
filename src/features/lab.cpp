#include "lab.h"
#include <cstdio>
#include "il2cpp.h"
#include "config.h"

static void* g_typeDay = nullptr;
static void* g_typeWall = nullptr;
static void* g_typeTree = nullptr;

static bool EnsureLabInited() {
    static bool inited = false;
    if (inited) return true;

    void* cDay = FindClass("Assembly-CSharp", "", "Day");
    if (cDay) g_typeDay = il2cpp_type_get_object(il2cpp_class_get_type(cDay));

    void* cWall = FindClass("Assembly-CSharp", "", "Wall");
    if (cWall) g_typeWall = il2cpp_type_get_object(il2cpp_class_get_type(cWall));

    void* cTree = FindClass("Assembly-CSharp", "", "Tree");
    if (cTree) g_typeTree = il2cpp_type_get_object(il2cpp_class_get_type(cTree));

    inited = (g_typeDay != nullptr);
    return inited;
}

static void* GetDayInstance() {
    if (!EnsureLabInited() || !fn_FindAll || !g_typeDay) return nullptr;
    void* arr = fn_FindAll(g_typeDay, mi_FindAll);
    if (!arr) return nullptr;
    unsigned n = il2cpp_array_length(arr);
    void** items = (void**)((char*)arr + 0x20);
    for (unsigned i = 0; i < n; i++) {
        if (items[i]) return items[i];
    }
    return nullptr;
}

#include "cheats.h"

static void* GetDirectorInstance() {
    EnsureLabInited();
    void* cMgr = FindClass("Assembly-CSharp", "", "Managers");
    if (cMgr && fn_FindAll) {
        void* tMgr = il2cpp_type_get_object(il2cpp_class_get_type(cMgr));
        if (tMgr) {
            void* arr = fn_FindAll(tMgr, mi_FindAll);
            if (arr && il2cpp_array_length(arr) > 0) {
                void* mgr = *(void**)((char*)arr + 0x20);
                if (mgr) {
                    size_t off_dir = FieldOffset(cMgr, "director");
                    if (off_dir) {
                        void* dir = *(void**)((char*)mgr + off_dir);
                        if (dir) return dir;
                    }
                }
            }
        }
    }
    void* cDir = FindClass("Assembly-CSharp", "", "Director");
    if (cDir && fn_FindAll) {
        void* tDir = il2cpp_type_get_object(il2cpp_class_get_type(cDir));
        if (tDir) {
            void* arr = fn_FindAll(tDir, mi_FindAll);
            if (arr && il2cpp_array_length(arr) > 0) {
                return *(void**)((char*)arr + 0x20);
            }
        }
    }
    return nullptr;
}

static void DoSetSeason(int season) {
    EnsureAttached();
    void* director = GetDirectorInstance();
    if (!director) {
        LogAction("SetSeason", LogType::Error, "Failed: Director instance not found");
        return;
    }
    void* cDir = FindClass("Assembly-CSharp", "", "Director");
    if (!cDir) {
        LogAction("SetSeason", LogType::Error, "Failed: Director class not found");
        return;
    }

    // Season enum: Summer = 1, Autumn = 2, Winter = 4, Spring = 8
    // User passes: 0=Spring, 1=Summer, 2=Autumn, 3=Winter
    const int seasonValues[] = { 8, 1, 2, 4 };
    const char* sNames[] = { "Spring", "Summer", "Autumn", "Winter" };
    int targetEnum = (season >= 0 && season < 4) ? seasonValues[season] : 1;
    const char* sName = (season >= 0 && season < 4) ? sNames[season] : "Unknown";

    void* mi_SetSeason = FindMethod(cDir, "set_CurrentSeason", 1);
    void* mi_ForceUpdate = FindMethod(cDir, "ForceUpdateSeason", 0);
    size_t off_curSeason = FieldOffset(cDir, "_currentSeason");

    if (off_curSeason) {
        *(int*)((char*)director + off_curSeason) = targetEnum;
    }

    if (mi_SetSeason) {
        typedef void (*t_SetSeason)(void*, int, const void*);
        ((t_SetSeason)MethodPtr(mi_SetSeason))(director, targetEnum, mi_SetSeason);
    }
    if (mi_ForceUpdate) {
        typedef void (*t_Force)(void*, const void*);
        ((t_Force)MethodPtr(mi_ForceUpdate))(director, mi_ForceUpdate);
    }

    LogAction("SetSeason", LogType::Success, "Season changed to %s (enum val %d)", sName, targetEnum);
}

void SetSeason(int season) {
    QueueAction([season]() { DoSetSeason(season); });
}

static void DoSkipTimePhase() {
    EnsureAttached();
    void* director = GetDirectorInstance();
    if (!director) {
        LogAction("SkipTimePhase", LogType::Error, "Failed: Director instance not found");
        return;
    }
    void* cDir = FindClass("Assembly-CSharp", "", "Director");
    if (!cDir) {
        LogAction("SkipTimePhase", LogType::Error, "Failed: Director class not found");
        return;
    }

    void* mi_Advance = FindMethod(cDir, "AdvanceTime", 1);
    if (mi_Advance) {
        typedef void (*t_Adv)(void*, float, const void*);
        ((t_Adv)MethodPtr(mi_Advance))(director, 6.0f, mi_Advance);
        LogAction("SkipTimePhase", LogType::Success, "Advanced time by +6 hours");
        return;
    }
    LogAction("SkipTimePhase", LogType::Error, "Failed: Director.AdvanceTime method not found");
}

void SkipTimePhase() {
    QueueAction([]() { DoSkipTimePhase(); });
}

static void DoSetBloodMoon(bool disabled) {
    EnsureAttached();
    void* day = GetDayInstance();
    if (!day) return;
    void* cDay = FindClass("Assembly-CSharp", "", "Day");
    if (!cDay) return;

    size_t off_bm = FieldOffset(cDay, "isBloodMoon");
    if (off_bm && disabled) {
        *(bool*)((char*)day + off_bm) = false;
        LogAction("BloodMoon", LogType::Success, "Blood moon forced OFF");
    }
}

void SetBloodMoon(bool disabled) {
    QueueAction([disabled]() { DoSetBloodMoon(disabled); });
}

static void DoApplyIndestructibleWalls() {
    EnsureAttached();
    if (!EnsureLabInited() || !fn_FindAll || !g_typeWall) {
        LogAction("IndestructibleWalls", LogType::Error, "Failed: Wall type or FindAll API not available");
        return;
    }
    void* arr = fn_FindAll(g_typeWall, mi_FindAll);
    if (!arr) {
        LogAction("IndestructibleWalls", LogType::Info, "No walls found in scene");
        return;
    }

    unsigned count = il2cpp_array_length(arr);
    void** items = (void**)((char*)arr + 0x20);
    void* cDamageable = FindClass("Assembly-CSharp", "", "Damageable");
    if (!cDamageable) {
        LogAction("IndestructibleWalls", LogType::Error, "Failed: Damageable class not found");
        return;
    }

    size_t off_hp = FieldOffset(cDamageable, "currentHitpoints");
    size_t off_maxHp = FieldOffset(cDamageable, "maxHitpoints");

    int repaired = 0;
    for (unsigned i = 0; i < count; i++) {
        if (!items[i]) continue;
        void* go = fn_get_gameObject(items[i], mi_get_gameObject);
        if (!go) continue;
        void* dmg = fn_GetComponent(go, il2cpp_type_get_object(il2cpp_class_get_type(cDamageable)), mi_GetComponent);
        if (dmg && off_hp && off_maxHp) {
            float maxH = *(float*)((char*)dmg + off_maxHp);
            *(float*)((char*)dmg + off_hp) = maxH;
            repaired++;
        }
    }
    LogAction("IndestructibleWalls", LogType::Success, "Max-repaired %d wall segments (scanned %u objects)", repaired, count);
}

void ApplyIndestructibleWalls() {
    QueueAction([]() { DoApplyIndestructibleWalls(); });
}

void RegrowTrees() {
    QueueAction([]() {
        EnsureAttached();
        void* cWTree = FindClass("Assembly-CSharp", "", "WorkableTree");
        if (!cWTree || !fn_FindAll) {
            LogAction("RegrowTrees", LogType::Error, "Failed: WorkableTree class or FindAll not found");
            return;
        }
        void* tWTree = il2cpp_type_get_object(il2cpp_class_get_type(cWTree));
        void* arr = fn_FindAll(tWTree, mi_FindAll);
        if (!arr) {
            LogAction("RegrowTrees", LogType::Info, "No workable trees found in scene");
            return;
        }
        unsigned count = il2cpp_array_length(arr);
        void** items = (void**)((char*)arr + 0x20);
        size_t off_chopped = FieldOffset(cWTree, "_chopped");
        size_t off_marked = FieldOffset(cWTree, "_marked");
        int restored = 0;
        for (unsigned i = 0; i < count; i++) {
            if (!items[i]) continue;
            if (off_chopped && *(bool*)((char*)items[i] + off_chopped)) {
                *(bool*)((char*)items[i] + off_chopped) = false;
                restored++;
            }
            if (off_marked) *(bool*)((char*)items[i] + off_marked) = false;
        }
        LogAction("RegrowTrees", LogType::Success, "Regrew/restored %d trees (total trees scanned: %u)", restored, count);
    });
}

static size_t off_archerCooldown = 0;
typedef void (*t_ArcherUpdate)(void*, const void*);
static t_ArcherUpdate o_ArcherUpdate = nullptr;

static void hk_ArcherUpdate(void* thisPtr, const void* mi) {
    if (thisPtr && off_archerCooldown && (g_config.archerFireBoost || g_config.towerFireBoost)) {
        static float s_baseCd = 0.0f;
        float cur = *(float*)((char*)thisPtr + off_archerCooldown);
        if (s_baseCd == 0.0f && cur > 0.05f) s_baseCd = cur;
        if (s_baseCd > 0.05f) {
            float mult = 1.0f;
            if (g_config.archerFireBoost) mult *= 2.0f;
            if (g_config.towerFireBoost) mult *= 2.0f;
            *(float*)((char*)thisPtr + off_archerCooldown) = s_baseCd / mult;
        }
    }
    if (o_ArcherUpdate) o_ArcherUpdate(thisPtr, mi);
}

typedef bool (*t_RulerCanActivate)(void*, const void*);
static t_RulerCanActivate o_RulerCanActivate = nullptr;
static bool hk_RulerCanActivate(void* thisPtr, const void* mi) {
    if (g_config.noToolCooldowns) return true;
    return o_RulerCanActivate ? o_RulerCanActivate(thisPtr, mi) : true;
}

typedef void (*t_StartRedMoon)(void*, const void*);
static t_StartRedMoon o_StartRedMoon = nullptr;
static void hk_StartRedMoon(void* thisPtr, const void* mi) {
    if (g_config.bloodMoonDisabled) return;
    if (o_StartRedMoon) o_StartRedMoon(thisPtr, mi);
}

void InitLabFeatures() {
    void* cArcher = FindClass("Assembly-CSharp", "", "Archer");
    if (cArcher) {
        off_archerCooldown = FieldOffset(cArcher, "shootCooldownTime");
        if (!off_archerCooldown) off_archerCooldown = FieldOffset(cArcher, "_shootCooldownTime");
        HookMethod(cArcher, "Update", 0, (void*)hk_ArcherUpdate, (void**)&o_ArcherUpdate);
    }

    void* cRuler = FindClass("Assembly-CSharp", "Rulers", "ItemBasedRulerAbility");
    if (!cRuler) cRuler = FindClass("Assembly-CSharp", "", "ItemBasedRulerAbility");
    if (cRuler) {
        HookMethod(cRuler, "CanActivate", 0, (void*)hk_RulerCanActivate, (void**)&o_RulerCanActivate);
    }

    void* cEnemyMgr = FindClass("Assembly-CSharp", "", "EnemyManager");
    if (cEnemyMgr) {
        HookMethod(cEnemyMgr, "StartRedMoon", 0, (void*)hk_StartRedMoon, (void**)&o_StartRedMoon);
    }
}
