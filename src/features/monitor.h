#pragma once
#include <cstring>
#include "imgui.h"

struct KingdomStats {
    int  totalCitizens    = 0;
    int  vagrants         = 0;
    int  archers          = 0;
    int  builders         = 0;
    int  standingWalls    = 0;
    int  intactPortals    = 0;
    int  dayNumber        = 1;
    int  coins            = 0;
    int  gems             = 0;
    char steedName[64]    = "Unknown";
    float steedStaminaPct = 0.0f;
};

extern bool g_monitorHudVisible;

void UpdateKingdomStats();
KingdomStats GetCurrentStats();

void RenderKingdomMonitorHud();
void CycleMonitorStyle();
const char* GetMonitorStyleName();
