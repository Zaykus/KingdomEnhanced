#include "monitor.h"
#include "il2cpp.h"
#include "cheats.h"
#include "config.h"
#include <cstring>
#include <cstdio>

static void* g_typeDay     = nullptr;
static void* g_typeBeggar  = nullptr;
static void* g_typeArcher  = nullptr;
static void* g_typeBuilder = nullptr;
static void* g_typeWall    = nullptr;
static void* g_typePortal  = nullptr;

static bool EnsureMonitorInited() {
    static bool inited = false;
    if (inited) return true;

    void* cDay = FindClass("Assembly-CSharp", "", "Day");
    if (cDay) g_typeDay = il2cpp_type_get_object(il2cpp_class_get_type(cDay));
    void* cBeggar = FindClass("Assembly-CSharp", "", "Beggar");
    if (cBeggar) g_typeBeggar = il2cpp_type_get_object(il2cpp_class_get_type(cBeggar));
    void* cArcher = FindClass("Assembly-CSharp", "", "Archer");
    if (cArcher) g_typeArcher = il2cpp_type_get_object(il2cpp_class_get_type(cArcher));
    void* cBuilder = FindClass("Assembly-CSharp", "", "Worker");
    if (!cBuilder) cBuilder = FindClass("Assembly-CSharp", "", "Builder");
    if (cBuilder) g_typeBuilder = il2cpp_type_get_object(il2cpp_class_get_type(cBuilder));
    void* cWall = FindClass("Assembly-CSharp", "", "Wall");
    if (cWall) g_typeWall = il2cpp_type_get_object(il2cpp_class_get_type(cWall));
    void* cPortal = FindClass("Assembly-CSharp", "", "Portal");
    if (cPortal) g_typePortal = il2cpp_type_get_object(il2cpp_class_get_type(cPortal));

    inited = true;
    return true;
}

static int CountActive(void* typeObj) {
    if (!typeObj || !fn_FindAll) return 0;
    void* arr = fn_FindAll(typeObj, mi_FindAll);
    if (!arr) return 0;
    unsigned n = il2cpp_array_length(arr);
    void** items = (void**)((char*)arr + 0x20);
    int active = 0;
    for (unsigned i = 0; i < n; i++) {
        if (!items[i]) continue;
        void* go = fn_get_gameObject(items[i], mi_get_gameObject);
        if (go && fn_ActiveInHier(go, mi_ActiveInHier)) active++;
    }
    return active;
}

static KingdomStats s_cachedStats{};
static SRWLOCK s_statsLock = SRWLOCK_INIT;

void UpdateKingdomStats() {
    EnsureAttached();
    static DWORD lastUpdateTick = 0;
    DWORD now = GetTickCount();
    if (lastUpdateTick != 0 && (now - lastUpdateTick) < 1500) return;
    lastUpdateTick = now;

    EnsureMonitorInited();
    KingdomStats stats{};

    stats.vagrants      = CountActive(g_typeBeggar);
    stats.archers       = CountActive(g_typeArcher);
    stats.builders      = CountActive(g_typeBuilder);
    stats.standingWalls = CountActive(g_typeWall);
    stats.intactPortals = CountActive(g_typePortal);
    stats.totalCitizens = stats.archers + stats.builders;

    // Day number
    if (g_typeDay && fn_FindAll) {
        void* arr = fn_FindAll(g_typeDay, mi_FindAll);
        if (arr && il2cpp_array_length(arr) > 0) {
            void* day = *((void**)((char*)arr + 0x20));
            void* cDay = FindClass("Assembly-CSharp", "", "Day");
            if (cDay && day) {
                size_t off_day = FieldOffset(cDay, "dayNumber");
                if (off_day) stats.dayNumber = *(int*)((char*)day + off_day);
            }
        }
    }

    // Coins & gems from Player wallet
    void* player = GetCurrentPlayer();
    if (player) {
        void* cPlayer = FindClass("Assembly-CSharp", "", "Player");
        if (cPlayer) {
            size_t off_wallet = FieldOffset(cPlayer, "<wallet>k__BackingField");
            if (!off_wallet) off_wallet = FieldOffset(cPlayer, "wallet");
            if (off_wallet) {
                void* wallet = *(void**)((char*)player + off_wallet);
                if (wallet) {
                    void* cWallet = FindClass("Assembly-CSharp", "", "Wallet");
                    if (cWallet) {
                        size_t off_coins = FieldOffset(cWallet, "coins");
                        if (!off_coins) off_coins = FieldOffset(cWallet, "<Coins>k__BackingField");
                        size_t off_gems  = FieldOffset(cWallet, "gems");
                        if (!off_gems)  off_gems  = FieldOffset(cWallet, "<Gems>k__BackingField");
                        if (off_coins) stats.coins = *(int*)((char*)wallet + off_coins);
                        if (off_gems)  stats.gems  = *(int*)((char*)wallet + off_gems);
                    }
                }
            }
        }

        // Steed name & stamina
        void* cPlayer2 = FindClass("Assembly-CSharp", "", "Player");
        if (cPlayer2) {
            size_t off_steed = FieldOffset(cPlayer2, "_steed");
            if (!off_steed) off_steed = FieldOffset(cPlayer2, "<steed>k__BackingField");
            if (!off_steed) off_steed = FieldOffset(cPlayer2, "steed");
            if (off_steed) {
                void* steed = *(void**)((char*)player + off_steed);
                if (steed) {
                    void* cSteed = FindClass("Assembly-CSharp", "", "Steed");
                    if (cSteed) {
                        size_t off_stamina = FieldOffset(cSteed, "<Stamina>k__BackingField");
                        if (!off_stamina) off_stamina = FieldOffset(cSteed, "stamina");
                        if (off_stamina) {
                            float stam = *(float*)((char*)steed + off_stamina);
                            stats.steedStaminaPct = stam / 100.0f;
                            if (stats.steedStaminaPct > 1.0f) stats.steedStaminaPct = 1.0f;
                            if (stats.steedStaminaPct < 0.0f) stats.steedStaminaPct = 0.0f;
                        }
                        // Try to get steed type name via GameObject name
                        if (fn_get_gameObject && fn_get_gameObject) {
                            void* go = fn_get_gameObject(steed, mi_get_gameObject);
                            if (go) {
                                void* cGO = FindClass("UnityEngine.CoreModule", "UnityEngine", "GameObject");
                                if (cGO) {
                                    typedef void* (*t_GetName)(void*, const void*);
                                    static t_GetName fn_getName = nullptr;
                                    static void* mi_getName = nullptr;
                                    if (!fn_getName) {
                                        mi_getName = FindMethod(cGO, "get_name", 0);
                                        fn_getName = (t_GetName)MethodPtr(mi_getName);
                                    }
                                    if (fn_getName) {
                                        void* nameStr = fn_getName(go, mi_getName);
                                        if (nameStr) {
                                            // IL2CPP string: 8-byte header, then UTF-16 chars
                                            int len = *(int*)((char*)nameStr + sizeof(void*) * 2);
                                            if (len > 0 && len < 63) {
                                                wchar_t* wchars = (wchar_t*)((char*)nameStr + sizeof(void*) * 2 + 4);
                                                int written = 0;
                                                for (int ci = 0; ci < len && ci < 63; ci++) {
                                                    unsigned short c = (unsigned short)wchars[ci];
                                                    if (c > 127) {
                                                        stats.steedName[written++] = '?';
                                                    } else {
                                                        stats.steedName[written++] = (char)c;
                                                    }
                                                }
                                                stats.steedName[written] = '\0';
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    AcquireSRWLockExclusive(&s_statsLock);
    s_cachedStats = stats;
    ReleaseSRWLockExclusive(&s_statsLock);
}

KingdomStats GetCurrentStats() {
    KingdomStats copy;
    AcquireSRWLockShared(&s_statsLock);
    copy = s_cachedStats;
    ReleaseSRWLockShared(&s_statsLock);
    return copy;
}

// ── On-Screen Monitor HUD (F4) ──────────────────────────────────────────────
bool g_monitorHudVisible = false;
static int s_monitorStyle = 0; // 0=Classic, 1=Neon, 2=Light, 3=Ghost

void CycleMonitorStyle() {
    s_monitorStyle = (s_monitorStyle + 1) % 4;
}

const char* GetMonitorStyleName() {
    switch (s_monitorStyle) {
        case 0: return "Classic";
        case 1: return "Neon";
        case 2: return "Light";
        case 3: return "Ghost";
        default: return "Classic";
    }
}

void RenderKingdomMonitorHud() {
    if (!g_monitorHudVisible) return;

    KingdomStats st = GetCurrentStats();

    ImGui::SetNextWindowSize(ImVec2(240, 280), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 80), ImGuiCond_FirstUseEver);

    float alpha = g_config.monitorBgAlpha;
    if (alpha < 0.15f || alpha > 1.0f) alpha = 0.55f;

    ImVec4 bgCol, textCol, headerCol, borderCol;
    if (s_monitorStyle == 0) { // Classic Translucent
        bgCol     = ImVec4(0.08f, 0.09f, 0.12f, alpha);
        textCol   = ImVec4(0.92f, 0.92f, 0.94f, 1.0f);
        headerCol = ImVec4(0.95f, 0.80f, 0.25f, 1.0f);
        borderCol = ImVec4(0.55f, 0.45f, 0.22f, 0.70f);
    } else if (s_monitorStyle == 1) { // Neon Translucent
        bgCol     = ImVec4(0.04f, 0.02f, 0.10f, alpha);
        textCol   = ImVec4(0.0f, 1.0f, 0.70f, 1.0f);
        headerCol = ImVec4(0.0f, 0.90f, 1.0f, 1.0f);
        borderCol = ImVec4(0.95f, 0.0f, 0.65f, 0.75f);
    } else if (s_monitorStyle == 2) { // Warm Light Translucent
        bgCol     = ImVec4(0.20f, 0.18f, 0.15f, alpha);
        textCol   = ImVec4(0.95f, 0.92f, 0.85f, 1.0f);
        headerCol = ImVec4(0.95f, 0.75f, 0.25f, 1.0f);
        borderCol = ImVec4(0.60f, 0.50f, 0.40f, 0.70f);
    } else { // Ghost Ultra-Sheer
        bgCol     = ImVec4(0.02f, 0.02f, 0.04f, alpha * 0.60f);
        textCol   = ImVec4(0.85f, 0.90f, 1.0f, 0.95f);
        headerCol = ImVec4(0.70f, 0.85f, 1.0f, 1.0f);
        borderCol = ImVec4(0.30f, 0.40f, 0.50f, 0.45f);
    }

    ImGui::SetNextWindowBgAlpha(alpha);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bgCol);
    ImGui::PushStyleColor(ImGuiCol_Text, textCol);
    ImGui::PushStyleColor(ImGuiCol_Border, borderCol);

    if (ImGui::Begin("Kingdom Monitor##HUD", &g_monitorHudVisible, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(headerCol, "Day %d", st.dayNumber);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.85f, 0.3f, 1.0f), "[ SAFE ]");
        ImGui::Separator();

        ImGui::Text("Wallet: %d Coins | %d Gems", st.coins, st.gems);
        ImGui::Text("Steed: %s (%.0f%%)", st.steedName, st.steedStaminaPct * 100.0f);
        ImGui::Separator();

        ImGui::Text("Archers: %d", st.archers);
        ImGui::SameLine(120);
        ImGui::Text("Builders: %d", st.builders);

        ImGui::Text("Citizens: %d", st.totalCitizens);
        ImGui::SameLine(120);
        ImGui::Text("Vagrants: %d", st.vagrants);

        ImGui::Text("Walls: %d", st.standingWalls);
        ImGui::SameLine(120);
        ImGui::Text("Portals: %d", st.intactPortals);

        ImGui::Separator();
        if (ImGui::SmallButton("Style")) {
            CycleMonitorStyle();
            LogAction("MonitorHUD", LogType::Info, "Monitor HUD style changed to %s", GetMonitorStyleName());
        }
        ImGui::SameLine();
        ImGui::TextDisabled("[%s]", GetMonitorStyleName());
        ImGui::SameLine();
        if (ImGui::SmallButton("Alpha")) {
            g_config.monitorBgAlpha = (g_config.monitorBgAlpha >= 0.85f) ? 0.35f : (g_config.monitorBgAlpha + 0.20f);
            LogAction("MonitorHUD", LogType::Info, "Monitor HUD alpha set to %.2f", g_config.monitorBgAlpha);
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(3);
}

