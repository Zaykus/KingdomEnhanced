#pragma once
#include <windows.h>
#include "imgui.h"

void RenderStaminaBar(ImDrawList* dl, int playerIdx);
void UpdateStaminaData();

void CycleStaminaBarStyle();
void CycleStaminaBarPosition();
const char* GetStaminaStyleName();
const char* GetStaminaPositionName();
