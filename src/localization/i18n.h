#pragma once

enum class Lang {
    English = 0,
    Chinese = 1,
    Korean  = 2,
    Spanish = 3,
    German  = 4,
    French  = 5
};

void SetLanguage(Lang lang);
Lang GetLanguage();
const char* TR(const char* key);
const char* GetLanguageName(Lang lang);
