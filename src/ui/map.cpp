#include "map.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "fonts.h"
#include "i18n.h"
#include "il2cpp.h"
#include "cheats.h"
#include "config.h"

enum MapSym : uint32_t {
    SYM_CASTLE     = 1,
    SYM_WALL       = 2,
    SYM_FOUNDATION = 3,
    SYM_RIVER      = 4,
    SYM_BERRY      = 5,
    SYM_ENEMY      = 6,
    SYM_PORTAL     = 7,
    SYM_CAMP       = 8,
    SYM_PLAYER     = 9,
    SYM_CHEST      = 10,
    SYM_BOAT       = 11,
    SYM_TELEPORTER = 12,
};

#define MAPCOL(r, g, b) ((uint32_t)(r) | ((uint32_t)(g) << 8) | ((uint32_t)(b) << 16) | 0xFF000000u)

struct MapMarker {
    float    x;
    uint32_t sym;
    uint32_t col;
    int      count;     // 0 = no count, >0 = count (e.g. beggars, enemy cluster size)
    uint8_t  row;       // 0 = structures/landmarks, 1 = mobile entities (player/enemies)
    uint8_t  wall;      // participates in wall line calculations
    char     label[32]; // descriptive display text
};

struct MapSnapshot {
    bool  valid = false;
    DWORD tick = 0;
    float levelWidth = 0;
    bool  hasCastle = false;
    float castleX = 0;
    float leftBorder = 0;
    float rightBorder = 0;
    float exploredL = -80.0f, exploredR = 80.0f;
    std::vector<MapMarker> markers;
};

static MapSnapshot g_map;
static SRWLOCK g_mapLock = SRWLOCK_INIT;
bool g_mapFull = false;

void UpdateMinimapData() {
    extern bool g_mapVisible;
    if (!g_mapVisible || !g_config.enableMinimap) return;

    EnsureAttached();
    static DWORD lastUpdateTick = 0;
    DWORD now = GetTickCount();
    if (lastUpdateTick != 0 && (now - lastUpdateTick) < 1200) {
        return;
    }
    lastUpdateTick = now;

    // Hotkey toggle for Full Island Reveal: Ctrl+F
    static bool fWasDown = false;
    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool fKey = (GetAsyncKeyState('F') & 0x8000) != 0;
    if (ctrl && fKey && !fWasDown) {
        g_mapFull = !g_mapFull;
        Log("Minimap: Full island reveal toggled %s", g_mapFull ? "ON" : "OFF");
    }
    fWasDown = (ctrl && fKey);

    void* player = GetCurrentPlayer();
    if (!player) return;

    // Cache static reflection types and offsets once
    static void* typeCastle = nullptr;
    static void* typeWall = nullptr;
    static void* typeCamp = nullptr;
    static void* typePortal = nullptr;
    static void* typeEnemy = nullptr;
    static void* typeChest = nullptr;
    static void* typeBoat = nullptr;
    static void* typeWharf = nullptr;
    static void* typeMgr = nullptr;

    static size_t off_campBeggars = 0;
    static size_t off_portalType = 0;
    static size_t off_portalState = 0;
    static size_t off_enemyDmg = 0;
    static size_t off_dmgDead = 0;
    static size_t off_mgrKingdom = 0;
    static size_t off_kCastle = 0;

    static bool typesInited = false;
    if (!typesInited) {
        auto GetTypeObj = [](const char* klassName, void** outKlass = nullptr) -> void* {
            void* c = FindClass("Assembly-CSharp", "", klassName);
            if (outKlass) *outKlass = c;
            if (c && il2cpp_class_get_type && il2cpp_type_get_object) {
                return il2cpp_type_get_object(il2cpp_class_get_type(c));
            }
            return nullptr;
        };

        void* cCastle = nullptr;
        void* cWall = nullptr;
        void* cCamp = nullptr;
        void* cPortal = nullptr;
        void* cEnemy = nullptr;
        void* cMgr = nullptr;
        void* cKing = nullptr;
        void* cDmg = nullptr;

        typeCastle = GetTypeObj("Castle", &cCastle);
        typeWall = GetTypeObj("Wall", &cWall);
        typeCamp = GetTypeObj("BeggarCamp", &cCamp);
        typePortal = GetTypeObj("Portal", &cPortal);
        typeEnemy = GetTypeObj("Enemy", &cEnemy);
        typeChest = GetTypeObj("Chest", nullptr);
        if (!typeChest) typeChest = GetTypeObj("PayableChest", nullptr);
        typeBoat = GetTypeObj("Boat", nullptr);
        typeWharf = GetTypeObj("Wharf", nullptr);
        typeMgr = GetTypeObj("Managers", &cMgr);
        FindClass("Assembly-CSharp", "", "Kingdom"); // ensure loaded
        cKing = FindClass("Assembly-CSharp", "", "Kingdom");
        cDmg = FindClass("Assembly-CSharp", "", "Damageable");

        if (cCamp) off_campBeggars = FieldOffset(cCamp, "_beggars");
        if (cPortal) {
            off_portalType = FieldOffset(cPortal, "type");
            off_portalState = FieldOffset(cPortal, "<state>k__BackingField");
        }
        if (cEnemy) off_enemyDmg = FieldOffset(cEnemy, "damageable");
        if (cDmg) off_dmgDead = FieldOffset(cDmg, "<isDead>k__BackingField");
        if (cMgr) off_mgrKingdom = FieldOffset(cMgr, "kingdom");
        if (cKing) {
            off_kCastle = FieldOffset(cKing, "<castle>k__BackingField");
            if (!off_kCastle) off_kCastle = FieldOffset(cKing, "castle");
        }

        typesInited = true;
    }

    MapSnapshot snap;
    snap.valid = true;
    snap.tick = GetTickCount();

    // 1. Get Player position & live exploration bounds
    if (fn_get_transform && fn_get_position) {
        void* tr = fn_get_transform(player, mi_get_transform);
        if (tr) {
            Vector3 p = fn_get_position(tr, mi_get_position);
            MapMarker pm{};
            pm.x = p.x;
            pm.sym = SYM_PLAYER;
            pm.col = MAPCOL(0, 240, 255);
            pm.row = 1;
            strcpy_s(pm.label, "Monarch");
            snap.markers.push_back(pm);

            static float s_exploredL = 0.0f, s_exploredR = 0.0f;
            static bool s_hasExplored = false;
            if (!s_hasExplored) {
                s_exploredL = p.x - 70.0f;
                s_exploredR = p.x + 70.0f;
                s_hasExplored = true;
            }
            s_exploredL = std::min(s_exploredL, p.x - 50.0f);
            s_exploredR = std::max(s_exploredR, p.x + 50.0f);
            snap.exploredL = s_exploredL;
            snap.exploredR = s_exploredR;
        }
    }

    // 2. Scan Castle (Center of the kingdom)
    if (typeCastle && fn_FindAll) {
        void* arr = fn_FindAll(typeCastle, mi_FindAll);
        if (arr && il2cpp_array_length(arr) > 0) {
            void* cObj = *((void**)((char*)arr + 0x20));
            if (cObj) {
                void* tr = fn_get_transform(cObj, mi_get_transform);
                if (tr) {
                    Vector3 p = fn_get_position(tr, mi_get_position);
                    snap.hasCastle = true;
                    snap.castleX = p.x;
                    MapMarker m{};
                    m.x = p.x;
                    m.sym = SYM_CASTLE;
                    m.col = MAPCOL(255, 215, 0); // Gold
                    m.row = 0;
                    strcpy_s(m.label, "Castle");
                    snap.markers.push_back(m);
                }
            }
        }
    }

    // Fallback: Check Managers.kingdom.castle if not found yet
    if (!snap.hasCastle && typeMgr && fn_FindAll && off_mgrKingdom && off_kCastle) {
        void* arr = fn_FindAll(typeMgr, mi_FindAll);
        if (arr && il2cpp_array_length(arr) > 0) {
            void* mgr = *((void**)((char*)arr + 0x20));
            if (mgr) {
                void* kingdom = *(void**)((char*)mgr + off_mgrKingdom);
                if (kingdom) {
                    void* castle = *(void**)((char*)kingdom + off_kCastle);
                    if (castle) {
                        void* tr = fn_get_transform(castle, mi_get_transform);
                        if (tr) {
                            Vector3 p = fn_get_position(tr, mi_get_position);
                            snap.hasCastle = true;
                            snap.castleX = p.x;
                            MapMarker m{};
                            m.x = p.x;
                            m.sym = SYM_CASTLE;
                            m.col = MAPCOL(255, 215, 0);
                            m.row = 0;
                            strcpy_s(m.label, "Castle");
                            snap.markers.push_back(m);
                        }
                    }
                }
            }
        }
    }

    // 3. Scan Walls & Calculate Territory Boundaries
    float minLeftWall = snap.hasCastle ? snap.castleX : 1e9f;
    float maxRightWall = snap.hasCastle ? snap.castleX : -1e9f;

    if (typeWall && fn_FindAll) {
        void* arr = fn_FindAll(typeWall, mi_FindAll);
        if (arr) {
            unsigned n = il2cpp_array_length(arr);
            void** items = (void**)((char*)arr + 0x20);
            for (unsigned i = 0; i < n; i++) {
                if (!items[i]) continue;
                void* tr = fn_get_transform(items[i], mi_get_transform);
                if (tr) {
                    Vector3 p = fn_get_position(tr, mi_get_position);
                    MapMarker m{};
                    m.x = p.x;
                    m.sym = SYM_WALL;
                    m.col = MAPCOL(190, 215, 240); // Fortified stone blue
                    m.row = 0;
                    m.wall = 1;
                    strcpy_s(m.label, "Wall");
                    snap.markers.push_back(m);

                    if (snap.hasCastle) {
                        if (p.x < snap.castleX) minLeftWall = std::min(minLeftWall, p.x);
                        if (p.x > snap.castleX) maxRightWall = std::max(maxRightWall, p.x);
                    }
                }
            }
        }
    }

    snap.leftBorder = minLeftWall;
    snap.rightBorder = maxRightWall;

    // Expand explored zone to automatically encompass all territory within the walls
    if (snap.hasCastle) {
        snap.exploredL = std::min(snap.exploredL, minLeftWall - 15.0f);
        snap.exploredR = std::max(snap.exploredR, maxRightWall + 15.0f);
    }

    // 4. Scan Beggar Camps (with real-time beggar count!)
    if (typeCamp && fn_FindAll) {
        void* arr = fn_FindAll(typeCamp, mi_FindAll);
        if (arr) {
            unsigned n = il2cpp_array_length(arr);
            void** items = (void**)((char*)arr + 0x20);
            for (unsigned i = 0; i < n; i++) {
                if (!items[i]) continue;
                void* tr = fn_get_transform(items[i], mi_get_transform);
                if (tr) {
                    Vector3 p = fn_get_position(tr, mi_get_position);
                    int beggarCount = 0;

                    // Inspect _beggars List<Beggar>
                    if (off_campBeggars) {
                        void* list = *(void**)((char*)items[i] + off_campBeggars);
                        if (list) {
                            void* listItems = *(void**)((char*)list + 0x10);
                            int listSize = *(int*)((char*)list + 0x18);
                            for (int bi = 0; listItems && bi < listSize; bi++) {
                                void* beggar = ((void**)((char*)listItems + 0x20))[bi];
                                if (beggar && fn_get_gameObject) {
                                    void* go = fn_get_gameObject(beggar, mi_get_gameObject);
                                    if (go && fn_ActiveInHier && fn_ActiveInHier(go, mi_ActiveInHier)) {
                                        beggarCount++;
                                    }
                                }
                            }
                        }
                    }

                    MapMarker m{};
                    m.x = p.x;
                    m.sym = SYM_CAMP;
                    m.col = MAPCOL(255, 190, 60); // Warm amber
                    m.row = 0;
                    m.count = beggarCount;
                    if (beggarCount > 0) snprintf(m.label, sizeof(m.label), "Camp (%d)", beggarCount);
                    else strcpy_s(m.label, "Camp");
                    snap.markers.push_back(m);
                }
            }
        }
    }

    // 5. Scan Portals
    if (typePortal && fn_FindAll) {
        void* arr = fn_FindAll(typePortal, mi_FindAll);
        if (arr) {
            unsigned n = il2cpp_array_length(arr);
            void** items = (void**)((char*)arr + 0x20);
            for (unsigned i = 0; i < n; i++) {
                if (!items[i]) continue;
                void* tr = fn_get_transform(items[i], mi_get_transform);
                if (tr) {
                    Vector3 p = fn_get_position(tr, mi_get_position);
                    int pType = off_portalType ? *(int*)((char*)items[i] + off_portalType) : 0;
                    int pState = off_portalState ? *(int*)((char*)items[i] + off_portalState) : 0;

                    MapMarker m{};
                    m.x = p.x;
                    m.sym = SYM_PORTAL;
                    m.row = 0;

                    if (pType == 0) {
                        // Regular forest portal
                        m.col = MAPCOL(175, 75, 255); // Mystic purple
                        strcpy_s(m.label, "Portal");
                    } else if (pType == 1) {
                        // Cliff boss portal
                        if (pState == 5) {
                            m.col = MAPCOL(60, 220, 100); // Destroyed / Cleared
                            strcpy_s(m.label, "Cliff (Cleared)");
                        } else if (pState == 3) {
                            m.col = MAPCOL(70, 150, 255); // Rebuilding
                            strcpy_s(m.label, "Cliff (Rebuild)");
                        } else {
                            m.col = MAPCOL(255, 50, 70); // Active threat
                            strcpy_s(m.label, "Cliff Portal");
                        }
                    } else {
                        // Dock / Wharf portal
                        m.col = MAPCOL(100, 150, 255);
                        strcpy_s(m.label, "Dock Portal");
                    }
                    snap.markers.push_back(m);
                }
            }
        }
    }

    // 6. Scan Chests & Landmarks (Chests, Boats/Wharf)
    if (typeChest && fn_FindAll) {
        void* arr = fn_FindAll(typeChest, mi_FindAll);
        if (arr) {
            unsigned n = il2cpp_array_length(arr);
            void** items = (void**)((char*)arr + 0x20);
            for (unsigned i = 0; i < n; i++) {
                if (!items[i]) continue;
                void* tr = fn_get_transform(items[i], mi_get_transform);
                if (tr) {
                    Vector3 p = fn_get_position(tr, mi_get_position);
                    MapMarker m{};
                    m.x = p.x;
                    m.sym = SYM_CHEST;
                    m.col = MAPCOL(255, 220, 50); // Gold chest
                    m.row = 0;
                    strcpy_s(m.label, "Chest");
                    snap.markers.push_back(m);
                }
            }
        }
    }

    if ((typeBoat || typeWharf) && fn_FindAll) {
        void* arr = typeBoat ? fn_FindAll(typeBoat, mi_FindAll) : fn_FindAll(typeWharf, mi_FindAll);
        if (arr) {
            unsigned n = il2cpp_array_length(arr);
            void** items = (void**)((char*)arr + 0x20);
            for (unsigned i = 0; i < n; i++) {
                if (!items[i]) continue;
                void* tr = fn_get_transform(items[i], mi_get_transform);
                if (tr) {
                    Vector3 p = fn_get_position(tr, mi_get_position);
                    MapMarker m{};
                    m.x = p.x;
                    m.sym = SYM_BOAT;
                    m.col = MAPCOL(70, 220, 220); // Cyan wharf
                    m.row = 0;
                    strcpy_s(m.label, "Wharf");
                    snap.markers.push_back(m);
                }
            }
        }
    }

    // 7. Scan Greed Enemies & Aggregate attack wave clusters
    if (typeEnemy && fn_FindAll) {
        void* arr = fn_FindAll(typeEnemy, mi_FindAll);
        if (arr) {
            unsigned n = il2cpp_array_length(arr);
            void** items = (void**)((char*)arr + 0x20);
            std::vector<float> enemyPositions;
            enemyPositions.reserve(n);

            for (unsigned i = 0; i < n; i++) {
                if (!items[i]) continue;
                if (off_enemyDmg && off_dmgDead) {
                    void* dmg = *(void**)((char*)items[i] + off_enemyDmg);
                    if (dmg && *(bool*)((char*)dmg + off_dmgDead)) continue; // ignore dead
                }
                void* tr = fn_get_transform(items[i], mi_get_transform);
                if (tr) {
                    Vector3 p = fn_get_position(tr, mi_get_position);
                    enemyPositions.push_back(p.x);
                }
            }

            if (!enemyPositions.empty()) {
                std::sort(enemyPositions.begin(), enemyPositions.end());
                for (size_t i = 0; i < enemyPositions.size();) {
                    size_t j = i + 1;
                    float sumX = enemyPositions[i];
                    while (j < enemyPositions.size() && (enemyPositions[j] - enemyPositions[j - 1]) <= 6.0f) {
                        sumX += enemyPositions[j];
                        j++;
                    }
                    int clusterSize = (int)(j - i);
                    float avgX = sumX / (float)clusterSize;

                    MapMarker m{};
                    m.x = avgX;
                    m.sym = SYM_ENEMY;
                    m.col = MAPCOL(255, 55, 65); // High threat crimson
                    m.row = 1;
                    m.count = clusterSize;
                    if (clusterSize > 1) snprintf(m.label, sizeof(m.label), "Greed (%d)", clusterSize);
                    else strcpy_s(m.label, "Greed");
                    snap.markers.push_back(m);

                    i = j;
                }
            }
        }
    }

    AcquireSRWLockExclusive(&g_mapLock);
    g_map = std::move(snap);
    ReleaseSRWLockExclusive(&g_mapLock);
}

void RenderMinimap(ImDrawList* dl, float screenW) {
    MapSnapshot snap;
    AcquireSRWLockShared(&g_mapLock);
    snap = g_map;
    ReleaseSRWLockShared(&g_mapLock);

    if (!snap.valid || snap.markers.empty()) return;

    const float left = 24.0f, right = screenW - 24.0f, top = 8.0f;
    const float stripHeight = 58.0f;

    // 1. Sleek frosted glass panel
    dl->AddRectFilled(ImVec2(left, top), ImVec2(right, top + stripHeight), IM_COL32(12, 16, 24, 230), 8.0f);
    dl->AddRect(ImVec2(left, top), ImVec2(right, top + stripHeight), IM_COL32(55, 75, 110, 160), 8.0f, 0, 1.2f);
    dl->AddLine(ImVec2(left + 2, top + 1), ImVec2(right - 2, top + 1), IM_COL32(120, 160, 230, 80), 1.0f);

    // 2. Island extent and scale
    float mn = 1e9f, mx = -1e9f;
    for (auto& m : snap.markers) {
        mn = std::min(mn, m.x);
        mx = std::max(mx, m.x);
    }
    if (snap.hasCastle) {
        mn = std::min(mn, snap.castleX);
        mx = std::max(mx, snap.castleX);
    }
    if (mn >= mx) { mn -= 80.0f; mx += 80.0f; }

    float width = std::max((mx - mn) * 1.12f + 20.0f, 150.0f);
    float scale = (right - left - 40.0f) / width;
    float center = (left + right) * 0.5f;
    float offset = -(mn + mx) * 0.5f;

    auto X = [&](float wx) { return center + (wx + offset) * scale; };
    auto inFog = [&](float wx) { return !g_mapFull && (wx < snap.exploredL || wx > snap.exploredR); };

    // 3. Ground Baseline axis
    float axisY = top + 34.0f;
    dl->AddLine(ImVec2(left + 15.0f, axisY), ImVec2(right - 15.0f, axisY), IM_COL32(50, 65, 90, 160), 1.5f);

    // 4. Abevol Territory Wall Lines (Kingdom Safe Domain)
    if (snap.hasCastle && snap.leftBorder < snap.castleX && snap.rightBorder > snap.castleX) {
        float scrLeftWall = X(snap.leftBorder);
        float scrRightWall = X(snap.rightBorder);
        float scrCastle = X(snap.castleX);

        scrLeftWall = std::max(left + 15.0f, std::min(right - 15.0f, scrLeftWall));
        scrRightWall = std::max(left + 15.0f, std::min(right - 15.0f, scrRightWall));
        scrCastle = std::max(left + 15.0f, std::min(right - 15.0f, scrCastle));

        // Shaded kingdom territory rectangle
        dl->AddRectFilled(ImVec2(scrLeftWall, axisY - 16.0f), ImVec2(scrRightWall, axisY + 16.0f), IM_COL32(30, 80, 140, 50), 4.0f);
        dl->AddRect(ImVec2(scrLeftWall, axisY - 16.0f), ImVec2(scrRightWall, axisY + 16.0f), IM_COL32(70, 140, 230, 90), 4.0f);

        // Territory connection line: Left Wall <-> Castle <-> Right Wall
        dl->AddLine(ImVec2(scrLeftWall, axisY), ImVec2(scrRightWall, axisY), IM_COL32(90, 190, 255, 230), 2.5f);

        // Subtle territory label
        dl->AddText(nullptr, 10.0f, ImVec2(scrCastle - 20.0f, axisY + 17.0f), IM_COL32(140, 200, 255, 170), "DOMAIN");
    }

    // 5. Select font
    ImFont* textFont = g_fontDefault;
    if (GetLanguage() == Lang::Korean) textFont = g_fontKorean;
    else if (GetLanguage() == Lang::Chinese) textFont = g_fontChinese;

    // 6. Draw markers
    for (auto& m : snap.markers) {
        if (m.row == 0 && inFog(m.x)) continue; // Static landmarks respect fog of war

        float sx = X(m.x);
        if (sx < left + 10.0f || sx > right - 10.0f) continue;

        float sy = (m.row == 0) ? (axisY - 10.0f) : (axisY + 10.0f);

        // Distinct symbol graphics
        if (m.sym == SYM_PLAYER) {
            // Player Monarch: Glowing Chevron Arrow pointing down
            dl->AddTriangleFilled(ImVec2(sx, sy + 6.0f), ImVec2(sx - 5.5f, sy - 5.0f), ImVec2(sx + 5.5f, sy - 5.0f), m.col);
            dl->AddTriangle(ImVec2(sx, sy + 6.0f), ImVec2(sx - 5.5f, sy - 5.0f), ImVec2(sx + 5.5f, sy - 5.0f), IM_COL32(255, 255, 255, 220), 1.2f);
        } else if (m.sym == SYM_CASTLE) {
            // Castle: Royal Keep Tower Shape
            dl->AddRectFilled(ImVec2(sx - 6.0f, sy - 7.0f), ImVec2(sx + 6.0f, sy + 5.0f), m.col, 2.0f);
            dl->AddRect(ImVec2(sx - 6.0f, sy - 7.0f), ImVec2(sx + 6.0f, sy + 5.0f), IM_COL32(255, 255, 255, 220), 2.0f, 0, 1.2f);
            dl->AddLine(ImVec2(sx - 6.0f, sy - 9.0f), ImVec2(sx - 6.0f, sy - 7.0f), m.col, 2.0f);
            dl->AddLine(ImVec2(sx, sy - 9.0f), ImVec2(sx, sy - 7.0f), m.col, 2.0f);
            dl->AddLine(ImVec2(sx + 6.0f, sy - 9.0f), ImVec2(sx + 6.0f, sy - 7.0f), m.col, 2.0f);
        } else if (m.sym == SYM_WALL) {
            // Wall: Fortified stone battlement
            dl->AddRectFilled(ImVec2(sx - 3.5f, sy - 5.0f), ImVec2(sx + 3.5f, sy + 5.0f), m.col, 1.0f);
            dl->AddRect(ImVec2(sx - 3.5f, sy - 5.0f), ImVec2(sx + 3.5f, sy + 5.0f), IM_COL32(255, 255, 255, 180), 1.0f);
        } else if (m.sym == SYM_CAMP) {
            // Beggar Camp: Tent triangle
            dl->AddTriangleFilled(ImVec2(sx, sy - 6.0f), ImVec2(sx - 6.0f, sy + 4.0f), ImVec2(sx + 6.0f, sy + 4.0f), m.col);
            dl->AddTriangle(ImVec2(sx, sy - 6.0f), ImVec2(sx - 6.0f, sy + 4.0f), ImVec2(sx + 6.0f, sy + 4.0f), IM_COL32(255, 255, 255, 200), 1.0f);
            // Beggar count badge if > 0
            if (m.count > 0) {
                char cntStr[12];
                snprintf(cntStr, sizeof(cntStr), "%d", m.count);
                dl->AddCircleFilled(ImVec2(sx, sy - 11.0f), 5.5f, IM_COL32(40, 180, 80, 255));
                dl->AddCircle(ImVec2(sx, sy - 11.0f), 5.5f, IM_COL32(255, 255, 255, 220));
                ImVec2 tsz = ImGui::CalcTextSize(cntStr);
                dl->AddText(nullptr, 10.0f, ImVec2(sx - tsz.x * 0.5f, sy - 16.0f), IM_COL32(255, 255, 255, 255), cntStr);
            }
        } else if (m.sym == SYM_PORTAL) {
            // Portal: Glowing rune circle
            dl->AddCircleFilled(ImVec2(sx, sy), 5.5f, m.col);
            dl->AddCircle(ImVec2(sx, sy), 5.5f, IM_COL32(255, 255, 255, 220), 0, 1.5f);
            dl->AddCircleFilled(ImVec2(sx, sy), 2.5f, IM_COL32(255, 255, 255, 200));
        } else if (m.sym == SYM_CHEST) {
            // Chest: Gold gem / chest badge
            dl->AddRectFilled(ImVec2(sx - 4.5f, sy - 4.0f), ImVec2(sx + 4.5f, sy + 4.0f), m.col, 1.5f);
            dl->AddRect(ImVec2(sx - 4.5f, sy - 4.0f), ImVec2(sx + 4.5f, sy + 4.0f), IM_COL32(255, 255, 255, 220), 1.5f);
        } else if (m.sym == SYM_BOAT) {
            // Wharf / Boat: Boat polygon
            dl->AddRectFilled(ImVec2(sx - 5.0f, sy - 2.0f), ImVec2(sx + 5.0f, sy + 4.0f), m.col, 2.0f);
            dl->AddTriangleFilled(ImVec2(sx, sy - 6.0f), ImVec2(sx - 4.0f, sy - 2.0f), ImVec2(sx + 4.0f, sy - 2.0f), m.col);
        } else if (m.sym == SYM_ENEMY) {
            // Greed Pack: High-threat glowing crimson circle with count
            float rad = m.count > 5 ? 6.5f : 4.5f;
            dl->AddCircleFilled(ImVec2(sx, sy), rad, m.col);
            dl->AddCircle(ImVec2(sx, sy), rad, IM_COL32(255, 255, 255, 220), 0, 1.2f);
            if (m.count > 1) {
                char cntStr[12];
                snprintf(cntStr, sizeof(cntStr), "%d", m.count);
                ImVec2 tsz = ImGui::CalcTextSize(cntStr);
                dl->AddText(nullptr, 10.0f, ImVec2(sx - tsz.x * 0.5f, sy + rad + 1.0f), IM_COL32(255, 100, 100, 255), cntStr);
            }
        } else {
            dl->AddRectFilled(ImVec2(sx - 3.5f, sy - 3.5f), ImVec2(sx + 3.5f, sy + 3.5f), m.col, 1.5f);
        }

        // Draw text label for landmarks
        if (m.label[0] != '\0' && (m.sym == SYM_CASTLE || m.sym == SYM_CAMP || m.sym == SYM_PORTAL || m.sym == SYM_CHEST || m.sym == SYM_BOAT)) {
            ImVec2 sz = textFont->CalcTextSizeA(10.0f, FLT_MAX, 0, m.label);
            float tx = sx - sz.x * 0.5f;
            float ty = (m.row == 0) ? (sy - 15.0f) : (sy + 8.0f);
            // Shadow
            dl->AddText(textFont, 10.0f, ImVec2(tx + 1, ty + 1), IM_COL32(0, 0, 0, 220), m.label);
            dl->AddText(textFont, 10.0f, ImVec2(tx, ty), IM_COL32(230, 235, 245, 220), m.label);
        }
    }

    // 7. Full map indicator / hotkey hint
    if (g_mapFull) {
        dl->AddRectFilled(ImVec2(right - 80.0f, top + 6.0f), ImVec2(right - 8.0f, top + 22.0f), IM_COL32(0, 180, 120, 200), 4.0f);
        dl->AddText(nullptr, 11.0f, ImVec2(right - 74.0f, top + 8.0f), IM_COL32(255, 255, 255, 255), "FULL MAP");
    } else {
        dl->AddText(nullptr, 10.0f, ImVec2(right - 76.0f, top + 8.0f), IM_COL32(140, 150, 175, 180), "Fog (Ctrl+F)");
    }
}
