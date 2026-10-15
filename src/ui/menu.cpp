#include "menu.h"
#include <windows.h>
#include <shellapi.h>
#include "imgui.h"
#include "fonts.h"
#include "i18n.h"
#include "config.h"
#include "cheats.h"
#include "spawner.h"
#include "lab.h"
#include "hard.h"
#include "monitor.h"
#include "stamina.h"
#include "il2cpp.h"
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>

static int s_activeTab = 0;

int GetActiveTab() {
    return s_activeTab;
}

static void DrawSectionHeader(const char* title) {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.94f, 0.75f, 0.15f, 1.0f), "[ %s ]", title);
    ImGui::Separator();
    ImGui::Spacing();
}

void RenderModMenu() {
    ImFont* activeFont = g_config.boldFont ? g_fontDefaultBold : g_fontDefault;
    if (GetLanguage() == Lang::Korean) {
        activeFont = g_config.boldFont ? g_fontKoreanBold : g_fontKorean;
    } else if (GetLanguage() == Lang::Chinese) {
        activeFont = g_config.boldFont ? g_fontChineseBold : g_fontChinese;
    }
    if (!activeFont) activeFont = g_fontDefault;

    ImGui::PushFont(activeFont);

    float scale = (g_config.uiScale < 0.75f || g_config.uiScale > 3.0f) ? 1.0f : g_config.uiScale;
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = scale;

    float baseW = 920.0f * scale;
    float baseH = 680.0f * scale;

    static float s_appliedScale = 1.0f;
    if (fabsf(scale - s_appliedScale) > 0.02f) {
        ImGui::SetNextWindowSize(ImVec2(baseW, baseH), ImGuiCond_Always);
        s_appliedScale = scale;
    } else {
        ImGui::SetNextWindowSize(ImVec2(baseW, baseH), ImGuiCond_FirstUseEver);
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2(700.0f * scale, 500.0f * scale), ImVec2(io.DisplaySize.x, io.DisplaySize.y));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("Kingdom Enhanced ##MainWindow", nullptr, flags)) {

        // ── Window Header ───────────────────────────────────────────────
        ImGui::TextColored(ImVec4(0.94f, 0.75f, 0.15f, 1.0f), "KINGDOM ENHANCED");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.60f, 0.55f, 0.45f, 1.0f), "v0.1.0-alpha");

        float winWidth = ImGui::GetWindowWidth();
        if (winWidth > 550.0f * scale) {
            ImGui::SameLine(winWidth - 250.0f * scale);
            ImGui::TextColored(ImVec4(0.55f, 0.50f, 0.40f, 1.0f), "by Zaykus  |  Thanks to Abevol");
        }
        ImGui::Separator();
        ImGui::Spacing();

        // ── Main Body: Left Sidebar + Right Content Area ────────────────
        float sidebarWidth = 150.0f * scale;

        // 1. SIDEBAR
        ImGui::BeginChild("MenuSidebar", ImVec2(sidebarWidth, 0), true);
        ImGui::Spacing();

        const char* tabKeys[] = {
            "TAB_MAIN",
            "TAB_CHEATS",
            "TAB_SPAWNER",
            "TAB_LAB",
            "TAB_HARD",
            "TAB_MONITOR",
            "TAB_SETTINGS",
            "TAB_LOGS"
        };

        for (int i = 0; i < 8; i++) {
            bool isActive = (s_activeTab == i);
            if (isActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.185f, 0.110f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_Text,   ImVec4(0.94f, 0.75f, 0.15f, 1.00f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.095f, 0.065f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_Text,   ImVec4(0.65f, 0.60f, 0.50f, 1.00f));
            }
            if (ImGui::Button(TR(tabKeys[i]), ImVec2(sidebarWidth - 16.0f, 36.0f))) {
                s_activeTab = i;
            }
            ImGui::PopStyleColor(2);
            ImGui::Spacing();
        }

        // Sidebar Footer
        float sideH = ImGui::GetWindowHeight();
        if (sideH > 150.0f) {
            ImGui::SetCursorPosY(sideH - 36.0f);
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.50f, 0.45f, 0.38f, 1.0f), " F1: Toggle Menu");
        }
        ImGui::EndChild();

        // 2. CONTENT AREA
        ImGui::SameLine();
        ImGui::BeginChild("MenuContent", ImVec2(0, 0), true);

        // ── TAB 0: MAIN ──────────────────────────────────────────────────
        if (s_activeTab == 0) {
            DrawSectionHeader("QUICK ESSENTIALS");
            if (ImGui::Button("+10 Coins",   ImVec2(100, 32))) DropCoins(10);
            ImGui::SameLine();
            if (ImGui::Button("+50 Coins",   ImVec2(100, 32))) DropCoins(50);
            ImGui::SameLine();
            if (ImGui::Button("+5 Gems",     ImVec2(90,  32))) AddGems(5);
            ImGui::SameLine();
            if (ImGui::Button("Fill Wallet", ImVec2(120, 32))) FillWallet();
            ImGui::SameLine();
            if (ImGui::Button("Clear Coins", ImVec2(110, 32))) ClearCoins();

            DrawSectionHeader("PLAYER & MOVEMENT");
            ImGui::Checkbox("God Mode",         &g_config.godMode);
            ImGui::SameLine(280);
            ImGui::Checkbox("Infinite Stamina", &g_config.infiniteStamina);

            ImGui::Spacing();
            ImGui::SliderFloat("Travel Speed Multiplier", &g_config.speedMultiplier, 0.5f, 10.0f, "%.1fx");
            ImGui::SliderFloat("Player Visual Size",      &g_config.playerSize,      0.2f,  3.0f, "%.2fx");

            DrawSectionHeader("INTERFACE MODULES");
            ImGui::Checkbox("Stamina Bar (N)",  &g_config.enableStaminaBar);
            ImGui::SameLine(200);
            ImGui::Checkbox("Minimap (M)",      &g_config.enableMinimap);
            ImGui::SameLine(350);
            ImGui::Checkbox("Monitor HUD (F4)", &g_monitorHudVisible);

            extern bool g_mapFull;
            ImGui::Checkbox("Minimap Reveal All (Ctrl+F)", &g_mapFull);

            ImGui::Spacing();
            char stamStyleBuf[64];
            sprintf(stamStyleBuf, "Energy Bar Style: [%s]", GetStaminaStyleName());
            if (ImGui::Button(stamStyleBuf, ImVec2(240, 30))) {
                CycleStaminaBarStyle();
            }
            ImGui::SameLine();
            char stamPosBuf[64];
            sprintf(stamPosBuf, "Energy Bar Pos: [%s]", GetStaminaPositionName());
            if (ImGui::Button(stamPosBuf, ImVec2(240, 30))) {
                CycleStaminaBarPosition();
            }

            char monStyleBuf[64];
            sprintf(monStyleBuf, "Monitor Style: [%s]", GetMonitorStyleName());
            if (ImGui::Button(monStyleBuf, ImVec2(240, 30))) {
                CycleMonitorStyle();
            }
            ImGui::SameLine();
            if (ImGui::Button(g_monitorHudVisible ? "Hide Monitor HUD (F4)" : "Show Monitor HUD (F4)", ImVec2(240, 30))) {
                g_monitorHudVisible = !g_monitorHudVisible;
            }

            DrawSectionHeader("DISPLAY & 4K RESOLUTION SCALING");
            ImGui::Text("Adjust Menu & Font Size for 4K / High-DPI monitors:");
            if (ImGui::SliderFloat("Menu Scale", &g_config.uiScale, 1.0f, 2.5f, "%.2fx")) {
                SaveConfig();
            }
            if (ImGui::Button("1080p (1.0x)")) {
                g_config.uiScale = 1.0f;
                SaveConfig();
            }
            ImGui::SameLine();
            if (ImGui::Button("1440p (1.3x)")) {
                g_config.uiScale = 1.3f;
                SaveConfig();
            }
            ImGui::SameLine();
            if (ImGui::Button("4K (1.8x)")) {
                g_config.uiScale = 1.8f;
                SaveConfig();
            }
            ImGui::SameLine();
            if (ImGui::Button("4K Large (2.2x)")) {
                g_config.uiScale = 2.2f;
                SaveConfig();
            }
        }

        // ── TAB 1: CHEATS ────────────────────────────────────────────────
        else if (s_activeTab == 1) {
            DrawSectionHeader("INVINCIBILITY & STATUS");
            if (ImGui::Checkbox("God Mode", &g_config.godMode)) {
                LogAction("Cheat", LogType::Action, "God Mode toggled %s", g_config.godMode ? "ON" : "OFF");
            }
            ImGui::SameLine(220);
            if (ImGui::Checkbox("Infinite Stamina", &g_config.infiniteStamina)) {
                LogAction("Cheat", LogType::Action, "Infinite Stamina toggled %s", g_config.infiniteStamina ? "ON" : "OFF");
            }
            ImGui::SameLine(420);
            if (ImGui::Checkbox("No Tool Cooldowns", &g_config.noToolCooldowns)) {
                LogAction("Cheat", LogType::Action, "No Tool Cooldowns %s", g_config.noToolCooldowns ? "ON" : "OFF");
            }

            DrawSectionHeader("MOVEMENT & SCALING");
            if (ImGui::SliderFloat("Travel Speed", &g_config.speedMultiplier, 0.5f, 10.0f, "%.1fx")) {
                LogAction("Cheat", LogType::Action, "Travel speed set to %.1fx", g_config.speedMultiplier);
            }
            if (ImGui::SliderFloat("Player Scale", &g_config.playerSize, 0.2f, 3.0f, "%.2fx")) {
                LogAction("Cheat", LogType::Action, "Player visual scale set to %.2fx", g_config.playerSize);
            }

            DrawSectionHeader("ARTEMIS BOW");
            if (ImGui::Button("Launch Catapult Boulder (Space)", ImVec2(260, 32))) LaunchBoulder();
            ImGui::SliderInt  ("Artemis Arrow Count",  &g_config.artemisArrowCount, 1, 50);
            ImGui::SliderFloat("Artemis Arrow Range",  &g_config.artemisRange,     0.5f, 5.0f, "%.1fx");
            ImGui::SliderFloat("Artemis Arrow Damage", &g_config.artemisDamage,    0.5f, 5.0f, "%.1fx");

            DrawSectionHeader("ECONOMY");
            if (ImGui::Button("+10 Coins",   ImVec2(100, 32))) DropCoins(10);
            ImGui::SameLine();
            if (ImGui::Button("+50 Coins",   ImVec2(100, 32))) DropCoins(50);
            ImGui::SameLine();
            if (ImGui::Button("+5 Gems",     ImVec2(90,  32))) AddGems(5);
            ImGui::SameLine();
            if (ImGui::Button("Fill Wallet", ImVec2(120, 32))) FillWallet();
            ImGui::SameLine();
            if (ImGui::Button("Clear Coins", ImVec2(110, 32))) ClearCoins();

            if (ImGui::SliderFloat("Coin Income Multiplier", &g_config.coinIncomeMult, 0.5f, 4.0f, "%.2fx")) {
                LogAction("Cheat", LogType::Action, "Coin income multiplier set to %.2fx", g_config.coinIncomeMult);
            }
            if (ImGui::SliderFloat("Bag Drop Rate", &g_config.bagDropMult, 0.5f, 4.0f, "%.2fx")) {
                LogAction("Cheat", LogType::Action, "Bag drop multiplier set to %.2fx", g_config.bagDropMult);
            }

            DrawSectionHeader("MILITARY");
            if (ImGui::Button("Kill All Enemies",    ImVec2(170, 32))) KillAllEnemies();
            ImGui::SameLine();
            if (ImGui::Button("Destroy Portals",     ImVec2(160, 32))) DestroyAllPortals();
            ImGui::SameLine();
            if (ImGui::Button("Recruit Vagrants",    ImVec2(160, 32))) RecruitAllVagrants();
            ImGui::SameLine();
            if (ImGui::Button("Max Army",            ImVec2(120, 32))) MaxArmy();

            if (ImGui::Button("Drop Archer Bow",     ImVec2(160, 32))) DropTool("Archer");
            ImGui::SameLine();
            if (ImGui::Button("Drop Builder Hammer", ImVec2(180, 32))) DropTool("Builder");

            DrawSectionHeader("CONSTRUCTION & BUILDERS");
            if (ImGui::Checkbox("Instant Construction", &g_config.hyperBuilders)) {
                LogAction("Cheat", LogType::Action, "Instant Construction %s", g_config.hyperBuilders ? "ON" : "OFF");
            }
            ImGui::SameLine(240);
            if (ImGui::Checkbox("Expand Vagrant Camps", &g_config.largerCamps)) {
                LogAction("Cheat", LogType::Action, "Expand Vagrant Camps %s", g_config.largerCamps ? "ON" : "OFF");
            }
            if (ImGui::SliderFloat("Builder Movement Speed", &g_config.builderSpeedMult, 0.1f, 10.0f, "%.1fx")) {
                LogAction("Cheat", LogType::Action, "Builder Speed Multiplier set to %.1fx", g_config.builderSpeedMult);
            }
            if (ImGui::SliderFloat("Builder Work Efficiency", &g_config.builderEfficiency, 0.01f, 3.0f, "%.2fx")) {
                LogAction("Cheat", LogType::Action, "Builder Efficiency set to %.2fx", g_config.builderEfficiency);
            }
        }

        // ── TAB 2: SPAWNER ───────────────────────────────────────────────
        else if (s_activeTab == 2) {
            DrawSectionHeader("SPAWNER CONTROLLER");
            if (ImGui::SliderInt("Spawn Amount (1 - 50)", &g_config.spawnUnitCount, 1, 50)) {
                LogAction("Spawner", LogType::Info, "Spawn count set to %d", g_config.spawnUnitCount);
            }

            ImGui::Text("Spawn Location:");
            ImGui::SameLine();
            if (ImGui::RadioButton("Campfire Center", g_config.spawnAtCampfire)) {
                g_config.spawnAtCampfire = true;
                LogAction("Spawner", LogType::Info, "Spawn location set to Campfire Center");
            }
            ImGui::SameLine(260);
            if (ImGui::RadioButton("Monarch Position", !g_config.spawnAtCampfire)) {
                g_config.spawnAtCampfire = false;
                LogAction("Spawner", LogType::Info, "Spawn location set to Monarch Position");
            }

            DrawSectionHeader("CITIZENS & WORKERS");
            if (ImGui::Button("Villager (Peasant)", ImVec2(180, 32))) SpawnUnit("Peasant",  g_config.spawnUnitCount, g_config.spawnAtCampfire);
            ImGui::SameLine();
            if (ImGui::Button("Vagrant (Beggar)",   ImVec2(180, 32))) SpawnUnit("Beggar",   g_config.spawnUnitCount, g_config.spawnAtCampfire);
            ImGui::SameLine();
            if (ImGui::Button("Archer",             ImVec2(130, 32))) SpawnUnit("Archer",   g_config.spawnUnitCount, g_config.spawnAtCampfire);

            if (ImGui::Button("Builder",            ImVec2(130, 32))) SpawnUnit("Worker",   g_config.spawnUnitCount, g_config.spawnAtCampfire);
            ImGui::SameLine();
            if (ImGui::Button("Farmer",             ImVec2(130, 32))) SpawnUnit("Farmer",   g_config.spawnUnitCount, g_config.spawnAtCampfire);
            ImGui::SameLine();
            if (ImGui::Button("Pikeman",            ImVec2(130, 32))) SpawnUnit("Pikeman",  g_config.spawnUnitCount, g_config.spawnAtCampfire);
            ImGui::SameLine();
            if (ImGui::Button("Knight",             ImVec2(130, 32))) SpawnUnit("Knight",   g_config.spawnUnitCount, g_config.spawnAtCampfire);

            if (ImGui::Button("Berserker (Norse)",  ImVec2(180, 32))) SpawnUnit("Berserker",g_config.spawnUnitCount, g_config.spawnAtCampfire);
            ImGui::SameLine();
            if (ImGui::Button("Ninja (Shogun)",     ImVec2(160, 32))) SpawnUnit("Ninja",    g_config.spawnUnitCount, g_config.spawnAtCampfire);

            DrawSectionHeader("HERMITS & SPECIALISTS");
            if (ImGui::Button("Bakery Hermit",      ImVec2(160, 32))) SpawnHermit("Bakery");
            ImGui::SameLine();
            if (ImGui::Button("Ballista Hermit",    ImVec2(160, 32))) SpawnHermit("Ballista");
            ImGui::SameLine();
            if (ImGui::Button("Stable Hermit",      ImVec2(160, 32))) SpawnHermit("Stable");

            if (ImGui::Button("Horn Hermit",        ImVec2(160, 32))) SpawnHermit("Horn");
            ImGui::SameLine();
            if (ImGui::Button("Fire Hermit",        ImVec2(160, 32))) SpawnHermit("Fire");
            ImGui::SameLine();
            if (ImGui::Button("Warrior Hermit",     ImVec2(160, 32))) SpawnHermit("Warrior");
            ImGui::SameLine();
            if (ImGui::Button("Berserker Hermit",   ImVec2(170, 32))) SpawnHermit("Berserker");

            DrawSectionHeader("GREED / ENEMY SPAWNER");
            if (ImGui::Button("Greedling",          ImVec2(140, 32))) SpawnEnemyType(0, g_config.spawnUnitCount);
            ImGui::SameLine();
            if (ImGui::Button("Flyer (Squid)",      ImVec2(140, 32))) SpawnEnemyType(4, g_config.spawnUnitCount);
            ImGui::SameLine();
            if (ImGui::Button("Crown Stealer",      ImVec2(150, 32))) SpawnEnemyType(7, g_config.spawnUnitCount);
            ImGui::SameLine();
            if (ImGui::Button("Breeder (Boss)",     ImVec2(150, 32))) SpawnEnemyType(5, g_config.spawnUnitCount);

            if (ImGui::Button("Crusher",            ImVec2(140, 32))) SpawnEnemyType(10, g_config.spawnUnitCount);
            ImGui::SameLine();
            if (ImGui::Button("Greed Knight",       ImVec2(140, 32))) SpawnEnemyType(11, g_config.spawnUnitCount);
            ImGui::SameLine();
            if (ImGui::Button("Greed Archer",       ImVec2(140, 32))) SpawnEnemyType(12, g_config.spawnUnitCount);
        }

        // ── TAB 3: LAB ───────────────────────────────────────────────────
        else if (s_activeTab == 3) {
            DrawSectionHeader("WORLD & SEASONS");
            if (ImGui::Button("Spring", ImVec2(100, 32))) SetSeason(0);
            ImGui::SameLine();
            if (ImGui::Button("Summer", ImVec2(100, 32))) SetSeason(1);
            ImGui::SameLine();
            if (ImGui::Button("Autumn", ImVec2(100, 32))) SetSeason(2);
            ImGui::SameLine();
            if (ImGui::Button("Winter", ImVec2(100, 32))) SetSeason(3);
            ImGui::SameLine();
            if (ImGui::Button("Skip Time Phase", ImVec2(160, 32))) SkipTimePhase();

            ImGui::Spacing();
            if (ImGui::Checkbox("Lock Summer Season", &g_config.lockSummer)) {
                LogAction("Lab", LogType::Info, "Lock Summer Season %s", g_config.lockSummer ? "ON" : "OFF");
            }
            ImGui::SameLine(280);
            if (ImGui::Checkbox("Clear Weather (No Rain/Snow)", &g_config.clearWeather)) {
                LogAction("Lab", LogType::Info, "Clear Weather %s", g_config.clearWeather ? "ON" : "OFF");
            }
            if (ImGui::Checkbox("Disable Blood Moons", &g_config.bloodMoonDisabled)) {
                SetBloodMoon(g_config.bloodMoonDisabled);
                LogAction("Lab", LogType::Info, "Disable Blood Moons %s", g_config.bloodMoonDisabled ? "ON" : "OFF");
            }
            ImGui::SameLine(280);
            if (ImGui::Checkbox("Buoyant Currency", &g_config.coinsStayDry)) {
                LogAction("Lab", LogType::Info, "Buoyant Currency %s", g_config.coinsStayDry ? "ON" : "OFF");
            }
            if (ImGui::Checkbox("Self-Repairing Walls", &g_config.indestructibleWalls)) {
                if (g_config.indestructibleWalls) ApplyIndestructibleWalls();
                LogAction("Lab", LogType::Info, "Self-Repairing Walls %s", g_config.indestructibleWalls ? "ON" : "OFF");
            }
            ImGui::SameLine(280);
            if (ImGui::Checkbox("Instant Castle Upgrade", &g_config.instantCastle)) {
                LogAction("Lab", LogType::Info, "Instant Castle Upgrade %s", g_config.instantCastle ? "ON" : "OFF");
            }
            if (ImGui::Checkbox("Instant Day Skip", &g_config.instantDaySkip)) {
                LogAction("Lab", LogType::Info, "Instant Day Skip %s", g_config.instantDaySkip ? "ON" : "OFF");
            }
            ImGui::SameLine(280);
            if (ImGui::Checkbox("Animal Spawn Boost", &g_config.animalSpawnBoost)) {
                LogAction("Lab", LogType::Info, "Animal Spawn Boost %s", g_config.animalSpawnBoost ? "ON" : "OFF");
            }

            if (ImGui::Button("Regrow All Trees",  ImVec2(200, 32))) RegrowTrees();
            if (ImGui::SliderFloat("Tree Regrowth Speed", &g_config.treeRegrowthMult, 0.1f, 5.0f, "%.1fx")) {
                LogAction("Lab", LogType::Info, "Tree Regrowth Speed set to %.1fx", g_config.treeRegrowthMult);
            }
            if (ImGui::SliderInt("Recruit Cap (0=default)", &g_config.recruitCap, 0, 50)) {
                LogAction("Lab", LogType::Info, "Recruit Cap set to %d", g_config.recruitCap);
            }

            DrawSectionHeader("UNIT BOOSTS");
            if (ImGui::Checkbox("Archer Fire Rate Boost", &g_config.archerFireBoost)) {
                LogAction("Lab", LogType::Info, "Archer Fire Rate Boost %s", g_config.archerFireBoost ? "ON" : "OFF");
            }
            ImGui::SameLine(280);
            if (ImGui::Checkbox("Rapid Citizen Housing", &g_config.rapidCitizenHousing)) {
                LogAction("Lab", LogType::Info, "Rapid Citizen Housing %s", g_config.rapidCitizenHousing ? "ON" : "OFF");
            }
            if (ImGui::Checkbox("Berserker Rage Mode", &g_config.berserkerRage)) {
                LogAction("Lab", LogType::Info, "Berserker Rage Mode %s", g_config.berserkerRage ? "ON" : "OFF");
            }
            ImGui::SameLine(280);
            if (ImGui::Checkbox("Ninja Speed Boost", &g_config.ninjaSpeedBoost)) {
                LogAction("Lab", LogType::Info, "Ninja Speed Boost %s", g_config.ninjaSpeedBoost ? "ON" : "OFF");
            }
            if (ImGui::Checkbox("Elite Knights", &g_config.eliteKnights)) {
                LogAction("Lab", LogType::Info, "Elite Knights %s", g_config.eliteKnights ? "ON" : "OFF");
            }
            ImGui::SameLine(280);
            if (ImGui::Checkbox("Farm Output Boost", &g_config.farmOutputBoost)) {
                LogAction("Lab", LogType::Info, "Farm Output Boost %s", g_config.farmOutputBoost ? "ON" : "OFF");
            }
            if (ImGui::Checkbox("Tower Fire Boost", &g_config.towerFireBoost)) {
                LogAction("Lab", LogType::Info, "Tower Fire Boost %s", g_config.towerFireBoost ? "ON" : "OFF");
            }

            DrawSectionHeader("BALLISTA CONTROLS");
            if (ImGui::Checkbox("Ballista Boost", &g_config.ballistaBoost)) {
                LogAction("Lab", LogType::Info, "Ballista Boost %s", g_config.ballistaBoost ? "ON" : "OFF");
            }
            if (ImGui::SliderFloat("Ballista Reload Time", &g_config.ballistaReloadMult, 0.001f, 2.0f, "%.3fx")) {
                LogAction("Lab", LogType::Info, "Ballista Reload Time set to %.3fx", g_config.ballistaReloadMult);
            }
            if (ImGui::SliderFloat("Ballista Bolt Speed", &g_config.ballistaFlightMult, 1.0f, 5.0f, "%.1fx")) {
                LogAction("Lab", LogType::Info, "Ballista Bolt Speed set to %.1fx", g_config.ballistaFlightMult);
            }

            DrawSectionHeader("CATAPULT CONTROLS");
            if (ImGui::Checkbox("Catapult Boost", &g_config.catapultBoost)) {
                LogAction("Lab", LogType::Info, "Catapult Boost %s", g_config.catapultBoost ? "ON" : "OFF");
            }
            if (ImGui::SliderFloat("Catapult Reload Time", &g_config.catapultReloadMult, 0.001f, 2.0f, "%.3fx")) {
                LogAction("Lab", LogType::Info, "Catapult Reload Time set to %.3fx", g_config.catapultReloadMult);
            }
            if (ImGui::SliderFloat("Catapult Stone Speed", &g_config.catapultFlightMult, 1.0f, 5.0f, "%.1fx")) {
                LogAction("Lab", LogType::Info, "Catapult Stone Speed set to %.1fx", g_config.catapultFlightMult);
            }

            DrawSectionHeader("STEED ABILITIES");
            if (ImGui::Checkbox("Charge Damage Boost", &g_config.chargeDmgBoost)) {
                LogAction("Lab", LogType::Info, "Charge Damage Boost %s", g_config.chargeDmgBoost ? "ON" : "OFF");
            }
            if (ImGui::SliderFloat("Buff Aura Duration", &g_config.buffAuraDuration, 1.0f, 10.0f, "%.1fx")) {
                LogAction("Lab", LogType::Info, "Buff Aura Duration set to %.1fx", g_config.buffAuraDuration);
            }
        }

        // ── TAB 4: HARD MODE ─────────────────────────────────────────────
        else if (s_activeTab == 4) {
            DrawSectionHeader("DIFFICULTY SLIDERS");
            if (ImGui::SliderFloat("Enemy Wave Size", &g_config.waveMultiplier, 0.1f, 10.0f, "%.1fx")) {
                LogAction("HardMode", LogType::Info, "Enemy Wave Size multiplier set to %.1fx", g_config.waveMultiplier);
            }
            if (ImGui::SliderFloat("Enemy Move Speed", &g_config.enemySpeedMultiplier, 0.5f, 3.0f, "%.1fx")) {
                LogAction("HardMode", LogType::Info, "Enemy Move Speed multiplier set to %.1fx", g_config.enemySpeedMultiplier);
            }
            if (ImGui::SliderFloat("Portal Spawn Rate", &g_config.portalSpawnRate, 0.1f, 5.0f, "%.1fx")) {
                LogAction("HardMode", LogType::Info, "Portal Spawn Rate multiplier set to %.1fx", g_config.portalSpawnRate);
            }
            if (ImGui::SliderFloat("Greed Queen HP", &g_config.greedQueenHPScale, 0.5f, 5.0f, "%.1fx")) {
                LogAction("HardMode", LogType::Info, "Greed Queen HP scale set to %.1fx", g_config.greedQueenHPScale);
            }
            if (ImGui::SliderFloat("Director Threat Scale", &g_config.directorThreatMult, 0.1f, 5.0f, "%.1fx")) {
                LogAction("HardMode", LogType::Info, "Director Threat scale set to %.1fx", g_config.directorThreatMult);
            }

            DrawSectionHeader("RULES & RESTRICTIONS");
            if (ImGui::Checkbox("Prevent Greed Crown Stealing", &g_config.noCrownStealing)) {
                LogAction("HardMode", LogType::Info, "Greed Crown Stealing prevention %s", g_config.noCrownStealing ? "ENABLED" : "DISABLED");
            }
            ImGui::TextColored(ImVec4(0.65f, 0.55f, 0.35f, 1.0f),
                "Note: Crown stealing prevention takes effect on the next wave attack.");
        }

        // ── TAB 5: KINGDOM MONITOR ───────────────────────────────────────
        else if (s_activeTab == 5) {
            DrawSectionHeader("KINGDOM STATUS MONITOR");
            KingdomStats stats = GetCurrentStats();

            ImGui::TextColored(ImVec4(0.94f, 0.75f, 0.15f, 1.0f), "Day: %d", stats.dayNumber);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Columns(2, "StatsCol", false);
            ImGui::Text("Citizens: %d",     stats.totalCitizens);
            ImGui::Text("Vagrants: %d",     stats.vagrants);
            ImGui::Text("Archers: %d",      stats.archers);
            ImGui::Text("Builders: %d",     stats.builders);
            ImGui::Text("Standing Walls: %d",stats.standingWalls);
            ImGui::Text("Active Portals: %d",stats.intactPortals);
            ImGui::NextColumn();
            ImGui::TextColored(ImVec4(0.94f, 0.85f, 0.30f, 1.0f), "Coins: %d",  stats.coins);
            ImGui::TextColored(ImVec4(0.40f, 0.95f, 0.40f, 1.0f), "Gems:  %d",  stats.gems);
            ImGui::TextColored(ImVec4(0.65f, 0.80f, 1.00f, 1.0f), "Steed: %s",  stats.steedName);
            ImGui::TextColored(ImVec4(0.65f, 0.80f, 1.00f, 1.0f), "Stamina: %.0f%%", stats.steedStaminaPct * 100.0f);
            ImGui::Columns(1);
        }

        // ── TAB 6: SETTINGS ──────────────────────────────────────────────
        else if (s_activeTab == 6) {
            DrawSectionHeader(TR("SEC_LANGUAGE"));
            int langIdx = (int)g_config.language;
            if (ImGui::RadioButton("English", &langIdx, 0)) {
                g_config.language = Lang::English;
                SetLanguage(Lang::English);
                LogAction("Language", LogType::Info, "Language switched to English");
                SaveConfig();
            }
            ImGui::SameLine(100);
            if (ImGui::RadioButton("简体中文", &langIdx, 1)) {
                g_config.language = Lang::Chinese;
                SetLanguage(Lang::Chinese);
                LogAction("Language", LogType::Info, "Language switched to Simplified Chinese (简体中文)");
                SaveConfig();
            }
            ImGui::SameLine(210);
            if (ImGui::RadioButton("한국어", &langIdx, 2)) {
                g_config.language = Lang::Korean;
                SetLanguage(Lang::Korean);
                LogAction("Language", LogType::Info, "Language switched to Korean (한국어)");
                SaveConfig();
            }
            ImGui::SameLine(300);
            if (ImGui::RadioButton("Español", &langIdx, 3)) {
                g_config.language = Lang::Spanish;
                SetLanguage(Lang::Spanish);
                LogAction("Language", LogType::Info, "Language switched to Spanish (Español)");
                SaveConfig();
            }
            ImGui::SameLine(400);
            if (ImGui::RadioButton("Deutsch", &langIdx, 4)) {
                g_config.language = Lang::German;
                SetLanguage(Lang::German);
                LogAction("Language", LogType::Info, "Language switched to German (Deutsch)");
                SaveConfig();
            }
            ImGui::SameLine(500);
            if (ImGui::RadioButton("Français", &langIdx, 5)) {
                g_config.language = Lang::French;
                SetLanguage(Lang::French);
                LogAction("Language", LogType::Info, "Language switched to French (Français)");
                SaveConfig();
            }

            DrawSectionHeader(TR("SEC_MODULES"));
            if (ImGui::Checkbox(TR("CHK_MINIMAP"), &g_config.enableMinimap)) {
                LogAction("Modules", LogType::Action, "Minimap %s", g_config.enableMinimap ? "ENABLED" : "DISABLED");
            }
            ImGui::SameLine(180);
            if (ImGui::Checkbox(TR("CHK_STAMINA_BAR"), &g_config.enableStaminaBar)) {
                LogAction("Modules", LogType::Action, "Stamina Bar %s", g_config.enableStaminaBar ? "ENABLED" : "DISABLED");
            }
            ImGui::SameLine(360);
            if (ImGui::Checkbox(TR("CHK_HOTKEYS"), &g_config.enableHotkeys)) {
                LogAction("Modules", LogType::Action, "Hotkeys %s", g_config.enableHotkeys ? "ENABLED" : "DISABLED");
            }
            ImGui::SameLine(520);
            if (ImGui::Checkbox(TR("CHK_BOLD_FONT"), &g_config.boldFont)) {
                LogAction("Display", LogType::Info, "Bolder text font %s", g_config.boldFont ? "ENABLED" : "DISABLED");
                SaveConfig();
            }

            DrawSectionHeader(TR("SEC_DISPLAY"));
            if (ImGui::SliderFloat(TR("SLD_UI_SCALE"), &g_config.uiScale, 1.0f, 2.5f, "%.2fx")) {
                SaveConfig();
            }
            if (ImGui::SliderFloat(TR("SLD_HUD_ALPHA"), &g_config.monitorBgAlpha, 0.20f, 1.0f, "%.2f")) {
                SaveConfig();
            }

            DrawSectionHeader(TR("SEC_SAVE_LOAD"));
            if (ImGui::Button(TR("BTN_SAVE_CONFIG"), ImVec2(200, 36))) SaveConfig();
            ImGui::SameLine();
            if (ImGui::Button(TR("BTN_LOAD_CONFIG"), ImVec2(200, 36))) LoadConfig();
        }

        // ── TAB 7: LIVE ACTION LOGS ──────────────────────────────────────
        else if (s_activeTab == 7) {
            DrawSectionHeader("LIVE MOD & ACTION LOGS");

            static bool s_autoScroll = true;
            static char s_filter[64] = "";

            ImGui::Checkbox("Auto-Scroll", &s_autoScroll);
            ImGui::SameLine();
            if (ImGui::Button("Clear Logs", ImVec2(100, 26))) {
                ClearLogBuffer();
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy All to Clipboard", ImVec2(180, 26))) {
                std::vector<LogMessage> logs = GetLogBufferCopy();
                std::string fullText;
                for (const auto& entry : logs) {
                    fullText += "[";
                    fullText += entry.time;
                    fullText += "] ";
                    fullText += entry.text;
                    fullText += "\r\n";
                }
                ImGui::SetClipboardText(fullText.c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("Open Log File", ImVec2(130, 26))) {
                ShellExecuteA(NULL, "open", GetLogFilePath(), NULL, NULL, SW_SHOWNORMAL);
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(140);
            ImGui::InputTextWithHint("##LogFilter", "Filter...", s_filter, sizeof(s_filter));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Render log viewer window
            ImGui::BeginChild("LogViewerScrollingRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

            std::vector<LogMessage> logList = GetLogBufferCopy();
            for (const auto& msg : logList) {
                if (s_filter[0] != '\0' && !strstr(msg.text, s_filter) && !strstr(msg.time, s_filter)) {
                    continue;
                }

                // Color code entry
                ImVec4 color(0.85f, 0.85f, 0.85f, 1.0f); // Default Info: light gray
                const char* badge = "[INFO]";
                if (msg.type == LogType::Success) {
                    color = ImVec4(0.35f, 0.95f, 0.45f, 1.0f); // Green
                    badge = "[SUCCESS]";
                } else if (msg.type == LogType::Error) {
                    color = ImVec4(0.95f, 0.35f, 0.35f, 1.0f); // Red
                    badge = "[FAILED]";
                } else if (msg.type == LogType::Action) {
                    color = ImVec4(0.95f, 0.80f, 0.25f, 1.0f); // Gold / Yellow
                    badge = "[ACTION]";
                }

                ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "[%s]", msg.time);
                ImGui::SameLine();
                ImGui::TextColored(color, "%s %s", badge, msg.text);
            }

            if (s_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }

            ImGui::EndChild();
        }

        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopFont();
    io.FontGlobalScale = 1.0f;
}
