#pragma once
#include <windows.h>
#include <functional>

void InitFeatures();
void* GetCurrentPlayer();
void QueueAction(std::function<void()> act);

// Cheat Actions

void DropCoins(int count);
void AddGems(int count);
void FillWallet();
void LaunchBoulder();
void KillAllEnemies();
void DestroyAllPortals();
void RecruitAllVagrants();
void MaxArmy();
void DropTool(const char* toolType); // "Archer" or "Builder"
