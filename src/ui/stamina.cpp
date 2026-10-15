#include "stamina.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include "il2cpp.h"
#include "cheats.h"
#include "fonts.h"

struct StaminaView {
    float x = 0, y = 0;
    float stamina = 1.0f;
    bool  tired = false;
    float wellFed = 0.0f;
};

static StaminaView g_stam[2];
static SRWLOCK     g_stamLock = SRWLOCK_INIT;
static size_t off_playerSteed = 0, off_steedStamina = 0, off_steedTired = 0, off_steedWellFed = 0;

static int s_staminaStyle = 0;    // 0=Classic, 1=RPG, 2=Retro, 3=Dual
static int s_staminaPosMode = 0;  // 0=Bottom (HUD), 1=Top-Left (HUD), 2=Top-Right (HUD)

void CycleStaminaBarStyle() {
    s_staminaStyle = (s_staminaStyle + 1) % 4;
}

void CycleStaminaBarPosition() {
    s_staminaPosMode = (s_staminaPosMode + 1) % 3;
}

const char* GetStaminaStyleName() {
    switch (s_staminaStyle) {
        case 0: return "Classic";
        case 1: return "RPG";
        case 2: return "Retro";
        case 3: return "Dual";
        default: return "Classic";
    }
}

const char* GetStaminaPositionName() {
    switch (s_staminaPosMode) {
        case 0: return "Bottom (HUD)";
        case 1: return "Top-Left (HUD)";
        case 2: return "Top-Right (HUD)";
        default: return "Bottom (HUD)";
    }
}

static void EnsureStaminaInited() {
    EnsureAttached();
    static bool inited = false;
    if (inited) return;

    void* cPlayer = FindClass("Assembly-CSharp", "", "Player");
    if (cPlayer) {
        off_playerSteed = FieldOffset(cPlayer, "_steed");
        if (!off_playerSteed) off_playerSteed = FieldOffset(cPlayer, "<steed>k__BackingField");
        if (!off_playerSteed) off_playerSteed = FieldOffset(cPlayer, "steed");
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
    inited = true;
}

void UpdateStaminaData() {
    EnsureStaminaInited();
    void* player = GetCurrentPlayer();
    if (!player || !off_playerSteed) return;

    void* steed = *(void**)((char*)player + off_playerSteed);
    if (!steed) return;

    float curStamina = 100.0f;
    float tiredTime = 0.0f;
    float wellFed = 0.0f;

    if (off_steedStamina) curStamina = *(float*)((char*)steed + off_steedStamina);
    if (off_steedTired)   tiredTime  = *(float*)((char*)steed + off_steedTired);
    if (off_steedWellFed) wellFed    = *(float*)((char*)steed + off_steedWellFed);

    AcquireSRWLockExclusive(&g_stamLock);
    g_stam[0].stamina = std::clamp(curStamina / 100.0f, 0.0f, 1.0f);
    g_stam[0].tired = (tiredTime > 0.0f);
    g_stam[0].wellFed = wellFed;
    ReleaseSRWLockExclusive(&g_stamLock);
}

void RenderStaminaBar(ImDrawList* dl, int playerIdx) {
    if (playerIdx != 0) return;
    StaminaView v;
    AcquireSRWLockShared(&g_stamLock);
    v = g_stam[0];
    ReleaseSRWLockShared(&g_stamLock);

    float barW = 140.0f;
    float barH = 10.0f;
    ImGuiIO& io = ImGui::GetIO();
    float x = io.DisplaySize.x * 0.5f - barW * 0.5f;
    float y = io.DisplaySize.y - 70.0f;

    if (s_staminaPosMode == 1) { // Top-Left
        x = 50.0f;
        y = 70.0f;
    } else if (s_staminaPosMode == 2) { // Top-Right
        x = io.DisplaySize.x - barW - 50.0f;
        y = 70.0f;
    }

    // Color based on status
    uint32_t col = IM_COL32(50, 205, 80, 240); // Green
    const char* statusTxt = "100%";
    char bufTxt[32];
    sprintf(bufTxt, "%d%%", (int)(v.stamina * 100.0f));
    statusTxt = bufTxt;

    if (v.tired) {
        col = IM_COL32(235, 50, 50, 240); // Red
        statusTxt = "TIRED";
    } else if (v.stamina < 0.35f) {
        col = IM_COL32(245, 180, 40, 240); // Yellow/Orange
    } else if (v.wellFed > 0.0f) {
        col = IM_COL32(70, 160, 255, 240); // Blue (well-fed boost)
        statusTxt = "BUFF";
    }

    if (s_staminaStyle == 0) {
        // ── Style 0: Classic ───────────────────────────────────────────────
        dl->AddRectFilled(ImVec2(x - 2, y - 2), ImVec2(x + barW + 2, y + barH + 2), IM_COL32(10, 12, 16, 220), 4.0f);
        dl->AddRect(ImVec2(x - 2, y - 2), ImVec2(x + barW + 2, y + barH + 2), IM_COL32(70, 80, 110, 160), 4.0f);
        float fillW = barW * v.stamina;
        if (fillW > 0.0f) {
            dl->AddRectFilled(ImVec2(x, y), ImVec2(x + fillW, y + barH), col, 2.0f);
        }
    } else if (s_staminaStyle == 1) {
        // ── Style 1: RPG ───────────────────────────────────────────────────
        dl->AddRectFilled(ImVec2(x - 2, y - 2), ImVec2(x + barW + 2, y + barH + 2), IM_COL32(12, 12, 14, 230), 2.0f);
        dl->AddRect(ImVec2(x - 2, y - 2), ImVec2(x + barW + 2, y + barH + 2), IM_COL32(120, 100, 60, 220), 2.0f);
        float fillW = barW * v.stamina;
        if (fillW > 0.0f) {
            dl->AddRectFilled(ImVec2(x, y), ImVec2(x + fillW, y + barH), col, 1.0f);
        }
        // Label centered below
        ImVec2 tsz = ImGui::CalcTextSize(statusTxt);
        dl->AddText(ImVec2(x + (barW - tsz.x) * 0.5f, y + barH + 2), col, statusTxt);
    } else if (s_staminaStyle == 2) {
        // ── Style 2: Retro (10 discrete segments) ──────────────────────────
        int totalBlocks = 10;
        int activeBlocks = (int)std::ceil(v.stamina * totalBlocks);
        float blockW = (barW - (totalBlocks - 1) * 3.0f) / totalBlocks;

        dl->AddRectFilled(ImVec2(x - 3, y - 3), ImVec2(x + barW + 3, y + barH + 3), IM_COL32(5, 5, 8, 220), 2.0f);
        for (int b = 0; b < totalBlocks; b++) {
            float bx = x + b * (blockW + 3.0f);
            uint32_t bcol = (b < activeBlocks) ? col : IM_COL32(40, 42, 50, 180);
            dl->AddRectFilled(ImVec2(bx, y), ImVec2(bx + blockW, y + barH), bcol, 1.0f);
        }
    } else if (s_staminaStyle == 3) {
        // ── Style 3: Dual ──────────────────────────────────────────────────
        dl->AddRectFilled(ImVec2(x - 2, y - 2), ImVec2(x + barW + 2, y + barH + 2), IM_COL32(10, 12, 16, 220), 3.0f);
        dl->AddRect(ImVec2(x - 2, y - 2), ImVec2(x + barW + 2, y + barH + 2), IM_COL32(80, 90, 120, 180), 3.0f);
        float fillW = barW * v.stamina;
        if (fillW > 0.0f) {
            dl->AddRectFilled(ImVec2(x, y), ImVec2(x + fillW, y + 6), col, 2.0f);
        }
        // Sub-bar for recovery / stamina reserve
        uint32_t subCol = v.tired ? IM_COL32(200, 60, 60, 200) : IM_COL32(90, 100, 120, 160);
        dl->AddRectFilled(ImVec2(x, y + 7), ImVec2(x + barW, y + barH), subCol, 1.0f);
    }
}
