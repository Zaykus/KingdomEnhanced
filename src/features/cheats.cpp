#include "cheats.h"
#include <cstdio>
#include <cstdlib>
#include "il2cpp.h"
#include "config.h"
#include "MinHook.h"
#include "menu.h"
#include "hard.h"
#include "lab.h"


static void* g_currentPlayer = nullptr;
static void* g_playerMovers[2] = { nullptr, nullptr };
static const float kSpeedSteps[] = { 1.0f, 1.5f, 2.0f, 3.0f, 5.0f };

static std::vector<std::function<void()>> g_actions;
static SRWLOCK g_actionLock = SRWLOCK_INIT;

void QueueAction(std::function<void()> act) {
    if (!act) return;
    AcquireSRWLockExclusive(&g_actionLock);
    g_actions.push_back(act);
    ReleaseSRWLockExclusive(&g_actionLock);
}

// Types and Method Pointers
typedef void (*t_PlayerUpdate)(void* self, const void* mi);
static t_PlayerUpdate o_PlayerUpdate = nullptr;

typedef void* (*t_SpawnCurrency)(int type, Vector3 pos, Quaternion rot, void* parent, const void* mi);
typedef void  (*t_Drop)(void* self, void* dropper, Vector2 force, Vector2 offset, int policy,
                        bool switchToAnyPlayer, bool syncReceipt, bool notDropperTarget, const void* mi);
static t_SpawnCurrency fn_SpawnCurrency = nullptr;
static t_Drop          fn_Drop = nullptr;
static void *mi_SpawnCurrency = nullptr, *mi_Drop = nullptr;

// Boulder types
typedef Vector2 (*t_GetProjectileForce)(Vector2 shooter, Vector2 target, float angle, float* distance,
                                        float gravity, const void* mi);
typedef void  (*t_Launch)(void* self, Vector2 force, void* launcher, const void* mi);
typedef int   (*t_GetDirection)(void* mover, const void* mi);
typedef void* (*t_GetRigidbody)(void* self, const void* mi);
typedef float (*t_GetGravityScale)(void* rb, const void* mi);
typedef Vector2 (*t_GetGravity)(const void* mi);

static t_GetProjectileForce fn_ProjForce = nullptr;
static t_Launch fn_Launch = nullptr;
static t_GetDirection fn_GetDirection = nullptr;
static t_GetRigidbody fn_GetRigidbody = nullptr;
static t_GetGravityScale fn_GravityScale = nullptr;
static t_GetGravity fn_Gravity = nullptr;

static void *mi_ProjForce = nullptr, *mi_Launch = nullptr;
static void *mi_GetDirection = nullptr, *mi_GetRigidbody = nullptr, *mi_GravityScale = nullptr, *mi_Gravity = nullptr;

static void *g_typeBoulderData = nullptr, *g_typeLaunchable = nullptr, *g_typeEnemy = nullptr, *g_typePortal = nullptr;
static void *g_typeBeggar = nullptr, *g_typeArcher = nullptr, *g_typeKnight = nullptr;

static size_t off_wallet = 0, off_capacity = 0, off_playerMover = 0, off_playerSteed = 0;
static size_t off_steedStamina = 0, off_steedTired = 0, off_steedWellFed = 0;
static size_t off_bdBoulder = 0, off_launchedByEnemies = 0;
static void* g_typeCatapult = nullptr;
static size_t off_catapultBoulderPrefab = 0, off_catapultBoulderData = 0;
static size_t off_prevDir = 0;

// Speed hook
typedef void (*t_SetSpeed1)(void* mover, float speed, const void* mi);
typedef void (*t_SetSpeed2)(void* mover, float speed, int dir, const void* mi);
static t_SetSpeed1 o_SetSpeed1 = nullptr;
static t_SetSpeed2 o_SetSpeed2 = nullptr, o_ForceSetSpeed = nullptr;

static float CurrentSpeedMul(void* mover) {
    if (mover && (mover == g_playerMovers[0] || mover == g_playerMovers[1])) {
        if (g_config.speedMultiplier > 0.1f && g_config.speedMultiplier != 1.0f)
            return g_config.speedMultiplier;
        if (g_config.speedIndex >= 0 && g_config.speedIndex < 5)
            return kSpeedSteps[g_config.speedIndex];
    }
    if (mover && g_config.enemySpeedMultiplier > 0.1f && g_config.enemySpeedMultiplier != 1.0f) {
        if (g_typeEnemy && fn_get_gameObject && fn_GetComponent) {
            void* go = fn_get_gameObject(mover, mi_get_gameObject);
            if (go && fn_GetComponent(go, g_typeEnemy, mi_GetComponent)) {
                return g_config.enemySpeedMultiplier;
            }
        }
    }
    return 1.0f;
}

static void hk_SetSpeed1(void* m, float s, const void* mi) {
    o_SetSpeed1(m, s * CurrentSpeedMul(m), mi);
}
static void hk_SetSpeed2(void* m, float s, int d, const void* mi) {
    o_SetSpeed2(m, s * CurrentSpeedMul(m), d, mi);
}
static void hk_ForceSetSpeed(void* m, float s, int d, const void* mi) {
    o_ForceSetSpeed(m, s * CurrentSpeedMul(m), d, mi);
}

// Forward declarations for main-thread background feature updates
void UpdateMinimapData();
void UpdateStaminaData();
void UpdateKingdomStats();
extern bool g_mapVisible;
extern bool g_menuVisible;

// Player.Update hook
static void hk_PlayerUpdate(void* self, const void* mi) {
    o_PlayerUpdate(self, mi);
    if (!self) return;

    g_currentPlayer = self;

    // Optimized: Only refresh expensive reflection data when modules are active
    if (g_mapVisible && g_config.enableMinimap) {
        UpdateMinimapData();
    }
    if (g_config.enableStaminaBar) {
        UpdateStaminaData();
    }
    if (g_menuVisible && GetActiveTab() == 5) {
        UpdateKingdomStats();
    }

    static DWORD lastBuilderTick = 0;
    DWORD nowTick = GetTickCount();
    if (nowTick - lastBuilderTick >= 800) {
        lastBuilderTick = nowTick;
        extern void UpdateBuilderBuffs();
        UpdateBuilderBuffs();
    }

    // Drain queued actions on the Unity main thread
    std::vector<std::function<void()>> localActions;
    AcquireSRWLockExclusive(&g_actionLock);
    if (!g_actions.empty()) {
        localActions.swap(g_actions);
    }
    ReleaseSRWLockExclusive(&g_actionLock);

    for (auto& act : localActions) {
        if (act) act();
    }

    // Track mover
    void* mover = *(void**)((char*)self + off_playerMover);
    if (mover) {
        if (!g_playerMovers[0]) g_playerMovers[0] = mover;
        else if (g_playerMovers[0] != mover && !g_playerMovers[1]) g_playerMovers[1] = mover;
    }

    // Player size scaling if enabled
    if (g_config.playerSize > 0.1f && g_config.playerSize != 1.0f && fn_get_transform) {
        void* tr = fn_get_transform(self, mi_get_transform);
        if (tr) {
            void* cTr = FindClass("UnityEngine.CoreModule", "UnityEngine", "Transform");
            typedef void (*t_SetScale)(void* tr, Vector3 scale, const void* mi);
            static t_SetScale fn_SetScale = nullptr;
            static void* mi_SetScale = nullptr;
            if (!fn_SetScale && cTr) {
                mi_SetScale = FindMethod(cTr, "set_localScale", 1);
                fn_SetScale = (t_SetScale)MethodPtr(mi_SetScale);
            }
            if (fn_SetScale) {
                fn_SetScale(tr, Vector3{ g_config.playerSize, g_config.playerSize, g_config.playerSize }, mi_SetScale);
            }
        }
    }

    // Capacity override
    void* wallet = *(void**)((char*)self + off_wallet);
    if (wallet && off_capacity) {
        *(int*)((char*)wallet + off_capacity) = 100;
    }

    // Infinite stamina override
    if (g_config.infiniteStamina) {
        void* steed = *(void**)((char*)self + off_playerSteed);
        if (steed) {
            if (off_steedStamina) *(float*)((char*)steed + off_steedStamina) = 100.0f;
            if (off_steedTired)   *(float*)((char*)steed + off_steedTired) = 0.0f;
            if (off_steedWellFed) *(float*)((char*)steed + off_steedWellFed) = 600.0f;
        }
    }

    // Hotkey checks if enabled
    if (g_config.enableHotkeys) {
        static bool k1WasDown = false, k3WasDown = false, k4WasDown = false, spWasDown = false;
        bool k1 = (GetAsyncKeyState('1') & 0x8000) != 0;
        bool k3 = (GetAsyncKeyState('3') & 0x8000) != 0;
        bool k4 = (GetAsyncKeyState('4') & 0x8000) != 0;
        bool sp = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

        if (k1 && !k1WasDown) DropCoins(50);
        if (k3 && !k3WasDown) {
            g_config.speedIndex = (g_config.speedIndex + 1) % 5;
            Log("Speed cycled to %.1fx", kSpeedSteps[g_config.speedIndex]);
        }
        if (k4 && !k4WasDown) {
            g_config.infiniteStamina = !g_config.infiniteStamina;
            Log("Infinite stamina: %s", g_config.infiniteStamina ? "ON" : "OFF");
        }
        if (sp && !spWasDown) LaunchBoulder();

        k1WasDown = k1;
        k3WasDown = k3;
        k4WasDown = k4;
        spWasDown = sp;
    }
}

void InitFeatures() {
    Log("InitFeatures: resolving game classes & methods...");

    void* cPlayer = FindClass("Assembly-CSharp", "", "Player");
    if (cPlayer) {
        off_wallet      = FieldOffset(cPlayer, "<wallet>k__BackingField");
        if (!off_wallet) off_wallet = FieldOffset(cPlayer, "wallet");
        if (!off_wallet) off_wallet = FieldOffset(cPlayer, "_wallet");

        off_playerMover = FieldOffset(cPlayer, "<mover>k__BackingField");
        if (!off_playerMover) off_playerMover = FieldOffset(cPlayer, "mover");
        if (!off_playerMover) off_playerMover = FieldOffset(cPlayer, "_mover");

        off_playerSteed = FieldOffset(cPlayer, "_steed");
        if (!off_playerSteed) off_playerSteed = FieldOffset(cPlayer, "<steed>k__BackingField");
        if (!off_playerSteed) off_playerSteed = FieldOffset(cPlayer, "steed");

        off_prevDir = FieldOffset(cPlayer, "_previousActiveDirection");
        if (!off_prevDir) off_prevDir = FieldOffset(cPlayer, "_previousKeyDirection");

        HookMethod(cPlayer, "Update", 0, (void*)hk_PlayerUpdate, (void**)&o_PlayerUpdate);
    }

    void* cWallet = FindClass("Assembly-CSharp", "", "Wallet");
    if (cWallet) {
        off_capacity = FieldOffset(cWallet, "TotalCapacity");
    }

    void* cSteed = FindClass("Assembly-CSharp", "", "Steed");
    if (cSteed) {
        off_steedStamina = FieldOffset(cSteed, "<Stamina>k__BackingField");
        if (!off_steedStamina) off_steedStamina = FieldOffset(cSteed, "stamina");
        if (!off_steedStamina) off_steedStamina = FieldOffset(cSteed, "_stamina");

        off_steedTired   = FieldOffset(cSteed, "_tiredTimer");
        if (!off_steedTired) off_steedTired = FieldOffset(cSteed, "tiredTime");

        off_steedWellFed = FieldOffset(cSteed, "<WellFedTimer>k__BackingField");
        if (!off_steedWellFed) off_steedWellFed = FieldOffset(cSteed, "wellFedTime");
    }

    void* cMover = FindClass("Assembly-CSharp", "", "Mover");
    if (cMover) {
        HookMethod(cMover, "SetSpeed", 1, (void*)hk_SetSpeed1, (void**)&o_SetSpeed1);
        HookMethod(cMover, "SetSpeed", 2, (void*)hk_SetSpeed2, (void**)&o_SetSpeed2);
        HookMethod(cMover, "ForceSetSpeed", 2, (void*)hk_ForceSetSpeed, (void**)&o_ForceSetSpeed);
        mi_GetDirection = FindMethod(cMover, "GetDirection", 0);
        if (mi_GetDirection) fn_GetDirection = (t_GetDirection)MethodPtr(mi_GetDirection);
    }

    void* cPool = FindClass("Assembly-CSharp", "", "Pool");
    if (cPool) {
        mi_SpawnGO = FindMethod(cPool, "SpawnGO", 7);
        fn_SpawnGO = (t_SpawnGO)MethodPtr(mi_SpawnGO);
    }

    void* cCurExt = FindClass("Assembly-CSharp", "", "CurrencyManagerExt");
    if (cCurExt) {
        mi_SpawnCurrency = FindMethod(cCurExt, "SpawnCurrency", 4);
        fn_SpawnCurrency = (t_SpawnCurrency)MethodPtr(mi_SpawnCurrency);
    }

    void* cDroppable = FindClass("Assembly-CSharp", "", "Droppable");
    if (cDroppable) {
        mi_Drop = FindMethod(cDroppable, "Drop", 7);
        fn_Drop = (t_Drop)MethodPtr(mi_Drop);
    }

    Log("InitFeatures: SpawnCur=%p Drop=%p SpawnGO=%p walletOff=0x%zx moverOff=0x%zx steedOff=0x%zx",
        fn_SpawnCurrency, fn_Drop, fn_SpawnGO, off_wallet, off_playerMover, off_playerSteed);

    // Boulder & Physics
    void* cBD = FindClass("Assembly-CSharp", "", "BoulderData");
    if (cBD) {
        g_typeBoulderData = il2cpp_type_get_object(il2cpp_class_get_type(cBD));
        off_bdBoulder = FieldOffset(cBD, "_boulder");
        if (!off_bdBoulder) off_bdBoulder = FieldOffset(cBD, "boulder");
    }
    void* cCatapult = FindClass("Assembly-CSharp", "", "Catapult");
    if (cCatapult) {
        g_typeCatapult = il2cpp_type_get_object(il2cpp_class_get_type(cCatapult));
        off_catapultBoulderPrefab = FieldOffset(cCatapult, "boulderPrefab");
        off_catapultBoulderData   = FieldOffset(cCatapult, "_boulderData");
    }
    void* cLaunchable = FindClass("Assembly-CSharp", "", "Launchable");
    if (cLaunchable) {
        g_typeLaunchable = il2cpp_type_get_object(il2cpp_class_get_type(cLaunchable));
        off_launchedByEnemies = FieldOffset(cLaunchable, "launchedByEnemies");
        mi_Launch = FindMethod(cLaunchable, "Launch", 2);
        fn_Launch = (t_Launch)MethodPtr(mi_Launch);
        mi_GetRigidbody = FindMethod(cLaunchable, "get_rigidbody", 0);
        fn_GetRigidbody = (t_GetRigidbody)MethodPtr(mi_GetRigidbody);
    }
    void* cUtil = FindClass("Assembly-CSharp", "", "Util");
    if (cUtil) {
        mi_ProjForce = FindMethod(cUtil, "GetProjectileForce", 5);
        fn_ProjForce = (t_GetProjectileForce)MethodPtr(mi_ProjForce);
    }
    void* cPhysics2D = FindClass("UnityEngine.Physics2DModule", "UnityEngine", "Physics2D");
    if (cPhysics2D) {
        mi_Gravity = FindMethod(cPhysics2D, "get_gravity", 0);
        fn_Gravity = (t_GetGravity)MethodPtr(mi_Gravity);
    }
    void* cRB2D = FindClass("UnityEngine.Physics2DModule", "UnityEngine", "Rigidbody2D");
    if (cRB2D) {
        mi_GravityScale = FindMethod(cRB2D, "get_gravityScale", 0);
        fn_GravityScale = (t_GetGravityScale)MethodPtr(mi_GravityScale);
    }

    // Entity types for cheats
    void* cEnemy = FindClass("Assembly-CSharp", "", "Enemy");
    if (cEnemy) g_typeEnemy = il2cpp_type_get_object(il2cpp_class_get_type(cEnemy));
    void* cPortal = FindClass("Assembly-CSharp", "", "Portal");
    if (cPortal) g_typePortal = il2cpp_type_get_object(il2cpp_class_get_type(cPortal));
    void* cBeggar = FindClass("Assembly-CSharp", "", "Beggar");
    if (cBeggar) g_typeBeggar = il2cpp_type_get_object(il2cpp_class_get_type(cBeggar));
    void* cArcher = FindClass("Assembly-CSharp", "", "Archer");
    if (cArcher) g_typeArcher = il2cpp_type_get_object(il2cpp_class_get_type(cArcher));
    void* cKnight = FindClass("Assembly-CSharp", "", "Knight");
    if (cKnight) g_typeKnight = il2cpp_type_get_object(il2cpp_class_get_type(cKnight));

    InitHardFeatures();
    InitLabFeatures();

    Log("InitFeatures: complete!");
}

void* GetCurrentPlayer() {
    return g_currentPlayer;
}

static void DoDropCoins(int count) {
    EnsureAttached();
    if (!g_currentPlayer || !fn_SpawnCurrency || !fn_Drop || !fn_get_transform || !fn_get_gameObject) {
        LogAction("DropCoins", LogType::Error, "Failed: Player or Currency API unavailable");
        return;
    }
    void* tr = fn_get_transform(g_currentPlayer, mi_get_transform);
    void* go = fn_get_gameObject(g_currentPlayer, mi_get_gameObject);
    if (!tr || !go) {
        LogAction("DropCoins", LogType::Error, "Failed: Player Transform or GameObject null");
        return;
    }

    Vector3 pos = fn_get_position(tr, mi_get_position);
    pos.y += 1.0f;
    Quaternion rot{ 0, 0, 0, 1 };

    int dropped = 0;
    for (int i = 0; i < count; i++) {
        void* coin = fn_SpawnCurrency(0 /*Coins*/, pos, rot, nullptr, mi_SpawnCurrency);
        if (!coin) continue;
        Vector2 force{ ((rand() % 600) - 300) / 100.0f, 3.0f + (rand() % 300) / 100.0f };
        Vector2 offset{ 0, 0 };
        fn_Drop(coin, go, force, offset, 1, false, false, false, mi_Drop);
        dropped++;
    }
    if (dropped > 0) {
        LogAction("DropCoins", LogType::Success, "Dropped %d/%d coins at (%.1f, %.1f)", dropped, count, pos.x, pos.y);
    } else {
        LogAction("DropCoins", LogType::Error, "Failed: SpawnCurrency returned null for all %d coin attempts", count);
    }
}

void DropCoins(int count) {
    QueueAction([count]() { DoDropCoins(count); });
}

static void DoAddGems(int count) {
    EnsureAttached();
    if (!g_currentPlayer || !fn_SpawnCurrency || !fn_Drop || !fn_get_transform || !fn_get_gameObject) {
        LogAction("AddGems", LogType::Error, "Failed: Player or Currency API unavailable");
        return;
    }
    void* tr = fn_get_transform(g_currentPlayer, mi_get_transform);
    void* go = fn_get_gameObject(g_currentPlayer, mi_get_gameObject);
    if (!tr || !go) {
        LogAction("AddGems", LogType::Error, "Failed: Player Transform or GameObject null");
        return;
    }

    Vector3 pos = fn_get_position(tr, mi_get_position);
    pos.y += 1.0f;
    Quaternion rot{ 0, 0, 0, 1 };

    int dropped = 0;
    for (int i = 0; i < count; i++) {
        void* gem = fn_SpawnCurrency(1 /*Gems*/, pos, rot, nullptr, mi_SpawnCurrency);
        if (!gem) continue;
        Vector2 force{ ((rand() % 400) - 200) / 100.0f, 2.5f + (rand() % 200) / 100.0f };
        Vector2 offset{ 0, 0 };
        fn_Drop(gem, go, force, offset, 1, false, false, false, mi_Drop);
        dropped++;
    }
    if (dropped > 0) {
        LogAction("AddGems", LogType::Success, "Dropped %d/%d gems at (%.1f, %.1f)", dropped, count, pos.x, pos.y);
    } else {
        LogAction("AddGems", LogType::Error, "Failed: SpawnCurrency returned null for all %d gem attempts", count);
    }
}

void AddGems(int count) {
    QueueAction([count]() { DoAddGems(count); });
}

void FillWallet() {
    LogAction("FillWallet", LogType::Action, "Filling wallet (+100 coins)...");
    QueueAction([]() { DoDropCoins(100); });
}

static void* FindBoulderPrefab() {
    void* prefab = nullptr;

    // 1. Try finding via BoulderData instances
    if (g_typeBoulderData && fn_FindAll && off_bdBoulder) {
        void* arr = fn_FindAll(g_typeBoulderData, mi_FindAll);
        if (arr) {
            unsigned n = il2cpp_array_length(arr);
            void** items = (void**)((char*)arr + 0x20);
            for (unsigned i = 0; i < n && !prefab; i++) {
                if (items[i]) {
                    prefab = *(void**)((char*)items[i] + off_bdBoulder);
                }
            }
        }
    }

    // 2. Fallback: Try Catapult instances
    if (!prefab && g_typeCatapult && fn_FindAll) {
        void* arr = fn_FindAll(g_typeCatapult, mi_FindAll);
        if (arr) {
            unsigned n = il2cpp_array_length(arr);
            void** items = (void**)((char*)arr + 0x20);
            for (unsigned i = 0; i < n && !prefab; i++) {
                if (!items[i]) continue;
                if (off_catapultBoulderPrefab) {
                    prefab = *(void**)((char*)items[i] + off_catapultBoulderPrefab);
                }
                if (!prefab && off_catapultBoulderData && off_bdBoulder) {
                    void* bd = *(void**)((char*)items[i] + off_catapultBoulderData);
                    if (bd) prefab = *(void**)((char*)bd + off_bdBoulder);
                }
            }
        }
    }

    // 3. Fallback: Search all GameObjects for "Boulder"
    if (!prefab && fn_FindAll) {
        void* cGO = FindClass("UnityEngine.CoreModule", "UnityEngine", "GameObject");
        if (cGO) {
            void* arr = fn_FindAll(il2cpp_type_get_object(il2cpp_class_get_type(cGO)), mi_FindAll);
            if (arr) {
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
                if (fn_GetName) {
                    for (unsigned i = 0; i < n && !prefab; i++) {
                        if (!items[i]) continue;
                        void* str = fn_GetName(items[i], mi_GetName);
                        if (str) {
                            const wchar_t* wstr = (const wchar_t*)((char*)str + 0x14);
                            int slen = *(int*)((char*)str + 0x10);
                            if (slen > 0 && slen < 128) {
                                std::string s;
                                s.reserve(slen);
                                for (int k = 0; k < slen; k++) s += (char)wstr[k];
                                if (s.find("Boulder") != std::string::npos && s.find("Shop") == std::string::npos) {
                                    prefab = items[i];
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return prefab;
}

static void DoLaunchBoulder() {
    EnsureAttached();
    if (!g_currentPlayer) {
        LogAction("LaunchBoulder", LogType::Error, "Failed: Player instance not found");
        return;
    }
    if (!fn_SpawnGO) {
        LogAction("LaunchBoulder", LogType::Error, "Failed: Pool.SpawnGO not found");
        return;
    }

    void* prefab = FindBoulderPrefab();
    if (!prefab) {
        LogAction("LaunchBoulder", LogType::Error, "Failed: Boulder prefab not found in BoulderData, Catapult, or Scene");
        return;
    }

    void* tr = fn_get_transform(g_currentPlayer, mi_get_transform);
    void* go = fn_get_gameObject(g_currentPlayer, mi_get_gameObject);
    void* mover = *(void**)((char*)g_currentPlayer + off_playerMover);
    if (!tr || !go) {
        LogAction("LaunchBoulder", LogType::Error, "Failed: Player Transform or GameObject null");
        return;
    }

    int dir = 0;
    // 1. Try Player._previousActiveDirection
    if (off_prevDir) {
        int pd = *(int*)((char*)g_currentPlayer + off_prevDir);
        if (pd == 1 || pd == -1) dir = pd;
    }
    // 2. Try Mover.GetDirection()
    if (dir == 0 && mover && fn_GetDirection && mi_GetDirection) {
        int md = fn_GetDirection(mover, mi_GetDirection);
        if (md == 1 || md == -1) dir = md;
    }
    // 3. Fallback: Transform localScale.x
    if (dir == 0 && tr) {
        void* cTr = FindClass("UnityEngine.CoreModule", "UnityEngine", "Transform");
        typedef Vector3 (*t_GetScale)(void*, const void*);
        static t_GetScale fn_GetScale = nullptr;
        static void* mi_GetScale = nullptr;
        if (!fn_GetScale && cTr) {
            mi_GetScale = FindMethod(cTr, "get_localScale", 0);
            if (mi_GetScale) fn_GetScale = (t_GetScale)MethodPtr(mi_GetScale);
        }
        if (fn_GetScale && mi_GetScale) {
            Vector3 sc = fn_GetScale(tr, mi_GetScale);
            if (sc.x < -0.01f) dir = -1;
            else if (sc.x > 0.01f) dir = 1;
        }
    }
    if (dir == 0) dir = 1;

    Vector3 p = fn_get_position(tr, mi_get_position);
    Vector3 spawn{ p.x + dir * 1.5f, p.y + 2.0f, p.z };

    void* boulderGO = fn_SpawnGO(prefab, spawn, Quaternion{ 0, 0, 0, 1 }, nullptr, true, true, false, mi_SpawnGO);
    if (!boulderGO) {
        boulderGO = fn_SpawnGO(prefab, spawn, Quaternion{ 0, 0, 0, 1 }, nullptr, false, false, true, mi_SpawnGO);
    }
    if (!boulderGO) {
        LogAction("LaunchBoulder", LogType::Error, "Failed: Pool.SpawnGO returned null for boulder");
        return;
    }

    void* launchable = nullptr;
    if (g_typeLaunchable && fn_GetComponent) {
        launchable = fn_GetComponent(boulderGO, g_typeLaunchable, mi_GetComponent);
    }

    if (!launchable) {
        LogAction("LaunchBoulder", LogType::Success, "Spawned boulder at (%.1f, %.1f) [no Launchable component, spawned as physical object]", spawn.x, spawn.y);
        return;
    }

    float gravity = 9.81f;
    if (fn_GetRigidbody && fn_Gravity && fn_GravityScale) {
        void* rb = fn_GetRigidbody(launchable, mi_GetRigidbody);
        if (rb) gravity = -fn_Gravity(mi_Gravity).y * fn_GravityScale(rb, mi_GravityScale);
    }
    if (gravity <= 0.01f) gravity = 9.81f;

    float dist = 0;
    Vector2 targetPos{ p.x + dir * 18.0f, p.y };
    Vector2 force{ (float)(dir * 18.0f), 12.0f };
    if (fn_ProjForce) {
        force = fn_ProjForce(Vector2{ spawn.x, spawn.y }, targetPos, 45.0f, &dist, gravity, mi_ProjForce);
    }
    if (dir < 0 && force.x > 0) force.x = -force.x;
    if (dir > 0 && force.x < 0) force.x = -force.x;
    
    if (fn_Launch) {
        fn_Launch(launchable, force, go, mi_Launch);
    }

    if (off_launchedByEnemies) {
        *(bool*)((char*)launchable + off_launchedByEnemies) = false;
    }

    LogAction("LaunchBoulder", LogType::Success, "Spawned & launched boulder facing %s from (%.1f, %.1f) with force (%.1f, %.1f)", dir < 0 ? "LEFT" : "RIGHT", spawn.x, spawn.y, force.x, force.y);
}

void LaunchBoulder() {
    QueueAction([]() { DoLaunchBoulder(); });
}

static void DoKillAllEnemies() {
    EnsureAttached();
    if (!fn_FindAll || !g_typeEnemy) {
        LogAction("KillAllEnemies", LogType::Error, "Failed: Enemy type or FindAll API not available");
        return;
    }
    void* arr = fn_FindAll(g_typeEnemy, mi_FindAll);
    if (!arr) {
        LogAction("KillAllEnemies", LogType::Info, "No enemies found in scene");
        return;
    }

    unsigned count = il2cpp_array_length(arr);
    void** items = (void**)((char*)arr + 0x20);
    int killed = 0;

    void* cDamageable = FindClass("Assembly-CSharp", "", "Damageable");
    void* cEnemy = FindClass("Assembly-CSharp", "", "Enemy");
    void* mi_DestroyNow = cDamageable ? FindMethod(cDamageable, "DestroyNow", 0) : nullptr;
    void* mi_ReceiveFatal = cDamageable ? FindMethod(cDamageable, "ReceiveFatalDamage", 2) : nullptr;
    void* mi_Despawn = cEnemy ? FindMethod(cEnemy, "Despawn", 2) : nullptr;

    typedef void (*t_DestroyNow)(void*, const void*);
    typedef void (*t_ReceiveFatal)(void*, void*, int, const void*);
    typedef void (*t_Despawn)(void*, float, bool, const void*);

    void* cObj = FindClass("UnityEngine.CoreModule", "UnityEngine", "Object");
    void* mi_DestroyObj = cObj ? FindMethod(cObj, "Destroy", 1) : nullptr;
    typedef void (*t_DestroyObj)(void*, const void*);

    for (unsigned i = 0; i < count; i++) {
        if (!items[i]) continue;
        void* go = fn_get_gameObject ? fn_get_gameObject(items[i], mi_get_gameObject) : nullptr;
        if (!go) continue;
        if (fn_ActiveInHier && !fn_ActiveInHier(go, mi_ActiveInHier)) continue;

        bool done = false;
        if (cDamageable && fn_GetComponent) {
            void* dmg = fn_GetComponent(go, il2cpp_type_get_object(il2cpp_class_get_type(cDamageable)), mi_GetComponent);
            if (dmg) {
                if (mi_DestroyNow) {
                    ((t_DestroyNow)MethodPtr(mi_DestroyNow))(dmg, mi_DestroyNow);
                    done = true;
                } else if (mi_ReceiveFatal) {
                    ((t_ReceiveFatal)MethodPtr(mi_ReceiveFatal))(dmg, nullptr, 0, mi_ReceiveFatal);
                    done = true;
                }
            }
        }
        if (!done && mi_Despawn) {
            ((t_Despawn)MethodPtr(mi_Despawn))(items[i], 0.0f, true, mi_Despawn);
            done = true;
        }
        if (!done && mi_DestroyObj) {
            ((t_DestroyObj)MethodPtr(mi_DestroyObj))(go, mi_DestroyObj);
            done = true;
        }
        if (done) killed++;
    }
    LogAction("KillAllEnemies", LogType::Success, "Dispatched %d active enemies (scanned %u objects)", killed, count);
}

void KillAllEnemies() {
    QueueAction([]() { DoKillAllEnemies(); });
}

static void DoDestroyAllPortals() {
    EnsureAttached();
    if (!fn_FindAll || !g_typePortal) {
        LogAction("DestroyAllPortals", LogType::Error, "Failed: Portal type or FindAll API not available");
        return;
    }
    void* arr = fn_FindAll(g_typePortal, mi_FindAll);
    if (!arr) {
        LogAction("DestroyAllPortals", LogType::Info, "No portals found in scene");
        return;
    }

    unsigned count = il2cpp_array_length(arr);
    void** items = (void**)((char*)arr + 0x20);
    int destroyed = 0;
    void* cPortal = FindClass("Assembly-CSharp", "", "Portal");
    void* mi_SetDestroyed = cPortal ? FindMethod(cPortal, "SetDestroyed", 1) : nullptr;
    void* mi_OnDeath = cPortal ? FindMethod(cPortal, "HandleOnDeath", 1) : nullptr;
    typedef void (*t_SetDestroyed)(void*, bool, const void*);
    typedef void (*t_OnDeath)(void*, void*, const void*);

    for (unsigned i = 0; i < count; i++) {
        if (!items[i]) continue;
        if (mi_SetDestroyed) {
            ((t_SetDestroyed)MethodPtr(mi_SetDestroyed))(items[i], true, mi_SetDestroyed);
            destroyed++;
        } else if (mi_OnDeath) {
            ((t_OnDeath)MethodPtr(mi_OnDeath))(items[i], nullptr, mi_OnDeath);
            destroyed++;
        }
    }
    LogAction("DestroyAllPortals", LogType::Success, "Destroyed %d portals (scanned %u portals)", destroyed, count);
}

void DestroyAllPortals() {
    QueueAction([]() { DoDestroyAllPortals(); });
}

static void DoRecruitAllVagrants() {
    EnsureAttached();
    if (!fn_FindAll || !g_typeBeggar) {
        LogAction("RecruitVagrants", LogType::Error, "Failed: Beggar type or FindAll API not available");
        return;
    }
    void* arr = fn_FindAll(g_typeBeggar, mi_FindAll);
    if (!arr) {
        LogAction("RecruitVagrants", LogType::Info, "No vagrants found in scene");
        return;
    }

    unsigned count = il2cpp_array_length(arr);
    void** items = (void**)((char*)arr + 0x20);
    int recruited = 0;
    void* cBeggar = FindClass("Assembly-CSharp", "", "Beggar");
    void* cChar = FindClass("Assembly-CSharp", "", "Character");
    size_t off_char = cBeggar ? FieldOffset(cBeggar, "_character") : 0;
    void* mi_Promote = cChar ? FindMethod(cChar, "Promote", 2) : nullptr;
    typedef void* (*t_Promote)(void*, void*, void*, const void*);
    void* peasantStr = il2cpp_string_new("Peasant");

    for (unsigned i = 0; i < count; i++) {
        if (!items[i]) continue;
        if (off_char && mi_Promote && peasantStr) {
            void* ch = *(void**)((char*)items[i] + off_char);
            if (ch) {
                ((t_Promote)MethodPtr(mi_Promote))(ch, peasantStr, nullptr, mi_Promote);
                recruited++;
                continue;
            }
        }
    }
    LogAction("RecruitVagrants", LogType::Success, "Recruited %d vagrants into citizens (scanned %u vagrants)", recruited, count);
}

void RecruitAllVagrants() {
    QueueAction([]() { DoRecruitAllVagrants(); });
}

void MaxArmy() {
    LogAction("MaxArmy", LogType::Action, "Boosting army by recruiting all available vagrants...");
    QueueAction([]() { DoRecruitAllVagrants(); });
}

// ── DropTool: spawns an Archer bow or Builder hammer pickup ─────────────────
static void* FindToolPrefab(const char* toolName) {
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
                if (s.find(toolName) != std::string::npos && s.find("Shop") == std::string::npos) {
                    return items[i];
                }
            }
        }
    }
    return nullptr;
}

static void DoDropTool(const char* toolType) {
    EnsureAttached();
    if (!g_currentPlayer || !fn_get_transform || !fn_get_position || !fn_SpawnGO) {
        LogAction("DropTool", LogType::Error, "Failed: Player Transform or SpawnGO unavailable");
        return;
    }

    // Real game tool names: ToolBow and ToolHammer
    const char* toolName = (strcmp(toolType, "Archer") == 0) ? "ToolBow" : "ToolHammer";

    void* tr = fn_get_transform(g_currentPlayer, mi_get_transform);
    if (!tr) {
        LogAction("DropTool", LogType::Error, "Failed: Player Transform null");
        return;
    }
    Vector3 pos = fn_get_position(tr, mi_get_position);
    pos.y += 0.5f;

    void* prefab = FindToolPrefab(toolName);
    if (!prefab) {
        LogAction("DropTool", LogType::Error, "Failed: Tool prefab '%s' not found in scene or loaded assets", toolName);
        return;
    }

    void* spawned = fn_SpawnGO(prefab, pos, Quaternion{0,0,0,1}, nullptr, true, true, false, mi_SpawnGO);
    if (!spawned) spawned = fn_SpawnGO(prefab, pos, Quaternion{0,0,0,1}, nullptr, false, false, true, mi_SpawnGO);

    if (spawned) {
        LogAction("DropTool", LogType::Success, "Spawned '%s' pickup at (%.1f, %.1f)", toolName, pos.x, pos.y);
    } else {
        LogAction("DropTool", LogType::Error, "Failed: SpawnGO returned null for '%s'", toolName);
    }
}

void DropTool(const char* toolType) {
    char buf[32] = {};
    strncpy(buf, toolType, 31);
    std::string s(buf);
    QueueAction([s]() { DoDropTool(s.c_str()); });
}

// ── Builder Buffs (Speed & Efficiency) ──────────────────────────────────────
static void* g_typeWorker = nullptr;
static size_t off_workerRunSpeed = 0;
static size_t off_workerWorkTime = 0;

void UpdateBuilderBuffs() {
    EnsureAttached();
    if (!g_typeWorker) {
        void* cWorker = FindClass("Assembly-CSharp", "", "Worker");
        if (cWorker) {
            g_typeWorker = il2cpp_type_get_object(il2cpp_class_get_type(cWorker));
            off_workerRunSpeed = FieldOffset(cWorker, "runSpeed");
            off_workerWorkTime = FieldOffset(cWorker, "workTime");
        }
    }
    if (!g_typeWorker || !fn_FindAll || (!off_workerRunSpeed && !off_workerWorkTime)) return;

    void* arr = fn_FindAll(g_typeWorker, mi_FindAll);
    if (!arr) return;
    unsigned n = il2cpp_array_length(arr);
    void** items = (void**)((char*)arr + 0x20);

    for (unsigned i = 0; i < n; i++) {
        if (!items[i]) continue;
        void* go = fn_get_gameObject(items[i], mi_get_gameObject);
        if (!go || !fn_ActiveInHier(go, mi_ActiveInHier)) continue;

        if (off_workerRunSpeed) {
            static float s_baseRunSpeed = 0.0f;
            float cur = *(float*)((char*)items[i] + off_workerRunSpeed);
            if (s_baseRunSpeed == 0.0f && cur > 0.0f) s_baseRunSpeed = cur;
            float mult = g_config.builderSpeedMult;
            if (g_config.hyperBuilders && mult < 2.5f) mult = 2.5f;
            if (s_baseRunSpeed > 0.0f) {
                *(float*)((char*)items[i] + off_workerRunSpeed) = s_baseRunSpeed * mult;
            }
        }

        if (off_workerWorkTime) {
            static float s_baseWorkTime = 0.0f;
            float cur = *(float*)((char*)items[i] + off_workerWorkTime);
            if (s_baseWorkTime == 0.0f && cur > 0.0f) s_baseWorkTime = cur;
            float eff = g_config.builderEfficiency;
            if (g_config.hyperBuilders) eff = 0.005f; // near instant work!
            if (s_baseWorkTime > 0.0f && eff > 0.0001f) {
                *(float*)((char*)items[i] + off_workerWorkTime) = s_baseWorkTime * eff;
            }
        }
    }
}

