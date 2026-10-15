#pragma once

bool SpawnUnit(const char* tag, int amount = 1, bool atCampfire = true);
bool SpawnHermit(const char* hermitName);
bool SpawnEnemy(const char* enemyName, int amount = 1);
bool SpawnEnemyType(int enemyType, int amount = 1);
void ClearCoins();
void SpawnPeasantHotkey();

