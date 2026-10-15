#include "spawner.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include "il2cpp.h"
#include "cheats.h"
#include "config.h"

typedef void* (*t_GetCharacterByTag)(void* holder, void* tag, const void* mi);
static t_GetCharacterByTag fn_GetCharByTag = nullptr;
static void* mi_GetCharByTag = nullptr;

typedef void* (*t_GetEnemyPrefab)(void* enemyMgr, int enemyType, const void* mi);
static t_GetEnemyPrefab fn_GetEnemyPrefab = nullptr;
static void* mi_GetEnemyPrefab = nullptr;

static void* g_typeManagers = nullptr;
static size_t off_mgrHolder = 0, off_mgrWorld = 0, off_mgrKingdom = 0, off_mgrEnemies = 0;
static size_t off_worldGameLayer = 0, off_kCampfire = 0;

static bool EnsureSpawnerInited() {
    static bool inited = false;
    if (inited) return true;

    void* cMgr = FindClass("Assembly-CSharp", "", "Managers");
    if (!cMgr) return false;
    g_typeManagers = il2cpp_type_get_object(il2cpp_class_get_type(cMgr));

    off_mgrHolder  = FieldOffset(cMgr, "holder");
    off_mgrWorld   = FieldOffset(cMgr, "world");
    off_mgrKingdom = FieldOffset(cMgr, "kingdom");
    off_mgrEnemies = FieldOffset(cMgr, "enemies");
    if (!off_mgrEnemies) off_mgrEnemies = FieldOffset(cMgr, "_enemies");

    void* cWorld = FindClass("Assembly-CSharp", "", "World");
    if (cWorld) {
        off_worldGameLayer = FieldOffset(cWorld, "<gameLayer>k__BackingField");
        if (!off_worldGameLayer) off_worldGameLayer = FieldOffset(cWorld, "gameLayer");
        if (!off_worldGameLayer) off_worldGameLayer = FieldOffset(cWorld, "_gameLayer");
    }

    void* cKingdom = FindClass("Assembly-CSharp", "", "Kingdom");
    if (cKingdom) {
        off_kCampfire = FieldOffset(cKingdom, "campfirePosition");
        if (!off_kCampfire) off_kCampfire = FieldOffset(cKingdom, "<campfirePosition>k__BackingField");
        if (!off_kCampfire) off_kCampfire = FieldOffset(cKingdom, "_campfireWorldPosition");
        if (!off_kCampfire) off_kCampfire = FieldOffset(cKingdom, "campfire");
    }

    void* cHolder = FindClass("Assembly-CSharp", "", "Holder");
    if (cHolder) {
        mi_GetCharByTag = FindMethod(cHolder, "GetCharacterByTag", 1);
        fn_GetCharByTag = (t_GetCharacterByTag)MethodPtr(mi_GetCharByTag);
    }

    void* cEM = FindClass("Assembly-CSharp", "", "EnemyManager");
    if (cEM) {
        mi_GetEnemyPrefab = FindMethod(cEM, "GetPrefab", 1);
        if (mi_GetEnemyPrefab) fn_GetEnemyPrefab = (t_GetEnemyPrefab)MethodPtr(mi_GetEnemyPrefab);
    }

    inited = (g_typeManagers && off_mgrHolder && fn_SpawnGO);
    return inited;
}

static void* FindPrefabFallback(const char* name) {
    void* cGO = FindClass("UnityEngine.CoreModule", "UnityEngine", "GameObject");
    if (!cGO || !fn_FindAll) return nullptr;
    void* arr = fn_FindAll(il2cpp_type_get_object(il2cpp_class_get_type(cGO)), mi_FindAll);
    if (!arr) return nullptr;
    unsigned n = il2cpp_array_length(arr);
    void** items = (void**)((char*)arr + 0x20);

    void* cObj = FindClass("UnityEngine.CoreModule", "UnityEngine", "Object");
    typedef void* (*t_GetName)(void*, const void*);
    static t_GetName fn_GetName = nullptr;
    static void* mi_GetName = nullptr;
    if (!fn_GetName && cObj) {
        mi_GetName = FindMethod(cObj, "get_name", 0);
        fn_GetName = (t_GetName)MethodPtr(mi_GetName);
    }
    if (!fn_GetName) return nullptr;

    for (unsigned i = 0; i < n; i++) {
        if (!items[i]) continue;
        void* str = fn_GetName(items[i], mi_GetName);
        if (str) {
            const wchar_t* wstr = (const wchar_t*)((char*)str + 0x14);
            int slen = *(int*)((char*)str + 0x10);
            if (slen > 0 && slen < 128) {
                std::string s;
                s.reserve(slen);
                for (int k = 0; k < slen; k++) s += (char)wstr[k];
                if (s == name || s.find(name) != std::string::npos) {
                    return items[i];
                }
            }
        }
    }
    return nullptr;
}

static bool DoSpawnUnit(const char* tag, int amount, bool atCampfire) {
    EnsureAttached();
    if (!EnsureSpawnerInited() || !fn_FindAll) {
        Log("SpawnUnit: spawner components not ready");
        return false;
    }

    void* arr = fn_FindAll(g_typeManagers, mi_FindAll);
    if (!arr) return false;

    unsigned count = il2cpp_array_length(arr);
    void** items = (void**)((char*)arr + 0x20);
    void* mgr = nullptr;
    for (unsigned i = 0; i < count && !mgr; i++) {
        if (!items[i]) continue;
        void* g = fn_get_gameObject(items[i], mi_get_gameObject);
        if (g && fn_ActiveInHier(g, mi_ActiveInHier)) mgr = items[i];
    }
    if (!mgr) return false;

    void* holder  = *(void**)((char*)mgr + off_mgrHolder);
    void* world   = *(void**)((char*)mgr + off_mgrWorld);
    void* kingdom = *(void**)((char*)mgr + off_mgrKingdom);
    if (!holder || !kingdom) return false;

    void* parent = world ? *(void**)((char*)world + off_worldGameLayer) : nullptr;

    float campfireX = 0.0f;
    bool hasCampfire = false;
    if (off_kCampfire) {
        campfireX = *(float*)((char*)kingdom + off_kCampfire);
        if (campfireX > -2500.0f && campfireX < 2500.0f && !std::isnan(campfireX) && !std::isinf(campfireX)) {
            hasCampfire = true;
        }
    }

    Vector3 playerPos{ 0.0f, 1.0f, 1.0f };
    void* player = GetCurrentPlayer();
    if (player && fn_get_transform && fn_get_position) {
        void* tr = fn_get_transform(player, mi_get_transform);
        if (tr) {
            playerPos = fn_get_position(tr, mi_get_position);
        }
    }

    void* prefabGO = nullptr;
    if (fn_GetCharByTag) {
        void* character = fn_GetCharByTag(holder, il2cpp_string_new(tag), mi_GetCharByTag);
        if (character) prefabGO = fn_get_gameObject(character, mi_get_gameObject);
    }
    if (!prefabGO) {
        prefabGO = FindPrefabFallback(tag);
    }

    if (!prefabGO) {
        LogAction("SpawnUnit", LogType::Error, "Failed: Prefab tag '%s' not found", tag);
        return false;
    }

    if (amount < 1) amount = 1;
    if (amount > 50) amount = 50;

    int spawnedCount = 0;
    for (int i = 0; i < amount; i++) {
        Vector3 spawnPos = playerPos;
        if (atCampfire && hasCampfire) {
            spawnPos.x = campfireX + ((rand() % 600) - 300) / 100.0f;
            spawnPos.y = playerPos.y;
            spawnPos.z = playerPos.z;
        } else {
            spawnPos.x = playerPos.x + ((rand() % 400) - 200) / 100.0f;
            spawnPos.y = playerPos.y;
            spawnPos.z = playerPos.z;
        }

        Quaternion rot{ 0, 0, 0, 1 };
        void* spawned = fn_SpawnGO(prefabGO, spawnPos, rot, parent, false, false, true, mi_SpawnGO);
        if (!spawned) {
            spawned = fn_SpawnGO(prefabGO, spawnPos, rot, parent, true, true, false, mi_SpawnGO);
        }
        if (spawned) spawnedCount++;
    }

    if (spawnedCount > 0) {
        LogAction("SpawnUnit", LogType::Success, "Spawned %d '%s' unit(s) at %s (x=%.1f)",
            spawnedCount, tag, (atCampfire && hasCampfire) ? "campfire" : "player", (atCampfire && hasCampfire) ? campfireX : playerPos.x);
    } else {
        LogAction("SpawnUnit", LogType::Error, "Failed to spawn '%s' (SpawnGO returned null)", tag);
    }
    return spawnedCount > 0;
}

bool SpawnUnit(const char* tag, int amount, bool atCampfire) {
    std::string sTag = tag ? tag : "";
    QueueAction([sTag, amount, atCampfire]() {
        DoSpawnUnit(sTag.c_str(), amount, atCampfire);
    });
    return true;
}

bool SpawnHermit(const char* hermitName) {
    std::string sName = hermitName ? hermitName : "";
    QueueAction([sName]() {
        DoSpawnUnit(sName.c_str(), 1, false);
    });
    return true;
}

static bool DoSpawnEnemy(int enemyType, int amount) {
    EnsureAttached();
    if (!EnsureSpawnerInited() || !fn_FindAll) {
        Log("SpawnEnemy: spawner not ready");
        return false;
    }

    void* arr = fn_FindAll(g_typeManagers, mi_FindAll);
    if (!arr) return false;
    unsigned count = il2cpp_array_length(arr);
    void** items = (void**)((char*)arr + 0x20);
    void* mgr = nullptr;
    for (unsigned i = 0; i < count && !mgr; i++) {
        if (!items[i]) continue;
        void* g = fn_get_gameObject(items[i], mi_get_gameObject);
        if (g && fn_ActiveInHier(g, mi_ActiveInHier)) mgr = items[i];
    }
    if (!mgr) return false;

    if (!off_mgrEnemies) {
        void* cMgr = FindClass("Assembly-CSharp", "", "Managers");
        if (cMgr) off_mgrEnemies = FieldOffset(cMgr, "enemies");
    }
    if (!off_mgrEnemies) return false;

    void* enemyMgr = *(void**)((char*)mgr + off_mgrEnemies);
    if (!enemyMgr) {
        Log("SpawnEnemy: enemyMgr is null");
        return false;
    }

    if (!fn_GetEnemyPrefab) {
        void* cEM = FindClass("Assembly-CSharp", "", "EnemyManager");
        if (cEM) {
            mi_GetEnemyPrefab = FindMethod(cEM, "GetPrefab", 1);
            if (mi_GetEnemyPrefab) fn_GetEnemyPrefab = (t_GetEnemyPrefab)MethodPtr(mi_GetEnemyPrefab);
        }
    }
    if (!fn_GetEnemyPrefab) {
        Log("SpawnEnemy: fn_GetEnemyPrefab not found");
        return false;
    }

    void* enemyComp = fn_GetEnemyPrefab(enemyMgr, enemyType, mi_GetEnemyPrefab);
    if (!enemyComp) {
        Log("SpawnEnemy: GetPrefab(%d) returned null", enemyType);
        return false;
    }

    void* prefabGO = fn_get_gameObject(enemyComp, mi_get_gameObject);
    if (!prefabGO) return false;

    Vector3 playerPos{ 0.0f, 1.0f, 1.0f };
    void* player = GetCurrentPlayer();
    if (player && fn_get_transform && fn_get_position) {
        void* tr = fn_get_transform(player, mi_get_transform);
        if (tr) playerPos = fn_get_position(tr, mi_get_position);
    }

    Vector3 spawnBase = playerPos;
    spawnBase.x += 10.0f; // 10 units in front of player

    if (amount < 1) amount = 1;
    if (amount > 50) amount = 50;

    int spawnedCount = 0;
    for (int i = 0; i < amount; i++) {
        Vector3 pos = spawnBase;
        pos.x += ((rand() % 400) - 200) / 100.0f;
        Quaternion rot{ 0, 0, 0, 1 };
        void* spawned = fn_SpawnGO(prefabGO, pos, rot, nullptr, true, true, false, mi_SpawnGO);
        if (!spawned) spawned = fn_SpawnGO(prefabGO, pos, rot, nullptr, false, false, true, mi_SpawnGO);
        if (spawned) spawnedCount++;
    }

    if (spawnedCount > 0) {
        LogAction("SpawnEnemy", LogType::Success, "Spawned %d enemy/enemies (type %d) at x=%.1f", spawnedCount, enemyType, spawnBase.x);
    } else {
        LogAction("SpawnEnemy", LogType::Error, "Failed: SpawnGO returned null for enemy type %d", enemyType);
    }
    return spawnedCount > 0;
}

static int EnemyNameToType(const char* name) {
    if (!name) return 0;
    if (strstr(name, "TrollWeak") || strstr(name, "Greedling")) return 0;
    if (strstr(name, "TrollMedium")) return 1;
    if (strstr(name, "ToughTroll")) return 2;
    if (strstr(name, "Ogre")) return 3;
    if (strstr(name, "Squid") || strstr(name, "Flyer")) return 4;
    if (strstr(name, "Boss") || strstr(name, "Breeder")) return 5;
    if (strstr(name, "KillerBoss")) return 6;
    if (strstr(name, "Stealer")) return 7;
    if (strstr(name, "Crusher")) return 10;
    if (strstr(name, "Knight")) return 11;
    if (strstr(name, "Archer")) return 12;
    return 0;
}

bool SpawnEnemyType(int enemyType, int amount) {
    QueueAction([enemyType, amount]() {
        DoSpawnEnemy(enemyType, amount);
    });
    return true;
}

bool SpawnEnemy(const char* enemyName, int amount) {
    int type = EnemyNameToType(enemyName);
    return SpawnEnemyType(type, amount);
}

void ClearCoins() {
    QueueAction([]() {
        EnsureAttached();
        void* cObj = FindClass("UnityEngine.CoreModule", "UnityEngine", "Object");
        typedef void (*t_Destroy)(void* obj, const void* mi);
        static t_Destroy fn_Destroy = nullptr;
        static void* mi_Destroy = nullptr;
        if (!fn_Destroy && cObj) {
            mi_Destroy = FindMethod(cObj, "Destroy", 1);
            fn_Destroy = (t_Destroy)MethodPtr(mi_Destroy);
        }
        if (!fn_Destroy) {
            LogAction("ClearCoins", LogType::Error, "Failed: UnityEngine.Object.Destroy method not found");
            return;
        }

        void* cCurr = FindClass("Assembly-CSharp", "", "DroppableCurrency");
        if (!cCurr || !fn_FindAll) {
            LogAction("ClearCoins", LogType::Error, "Failed: DroppableCurrency class or FindAll not found");
            return;
        }
        void* arr = fn_FindAll(il2cpp_type_get_object(il2cpp_class_get_type(cCurr)), mi_FindAll);
        if (!arr) {
            LogAction("ClearCoins", LogType::Info, "No droppable currency found on ground");
            return;
        }
        unsigned count = il2cpp_array_length(arr);
        void** items = (void**)((char*)arr + 0x20);

        int cleared = 0;
        for (unsigned i = 0; i < count; i++) {
            if (!items[i]) continue;
            void* go = fn_get_gameObject(items[i], mi_get_gameObject);
            if (go && fn_ActiveInHier(go, mi_ActiveInHier)) {
                fn_Destroy(go, mi_Destroy);
                cleared++;
            }
        }
        LogAction("ClearCoins", LogType::Success, "Cleared %d dropped currency objects from ground", cleared);
    });
}

void SpawnPeasantHotkey() {
    SpawnUnit("Peasant", 1, true);
}
