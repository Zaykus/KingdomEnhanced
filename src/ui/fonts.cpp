#include "fonts.h"
#include <windows.h>
#include <cstdio>
#include "il2cpp.h"

ImFont* g_fontDefault = nullptr;
ImFont* g_fontDefaultBold = nullptr;
ImFont* g_fontSymbols = nullptr;
ImFont* g_fontKorean = nullptr;
ImFont* g_fontKoreanBold = nullptr;
ImFont* g_fontChinese = nullptr;
ImFont* g_fontChineseBold = nullptr;

static bool FileExists(const char* p) {
    return p && (GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES);
}

void LoadAllFonts() {
    ImGuiIO& io = ImGui::GetIO();
    char winDir[MAX_PATH];
    GetWindowsDirectoryA(winDir, MAX_PATH);

    char segoeui[MAX_PATH], segoeuib[MAX_PATH], seguisym[MAX_PATH];
    char malgun[MAX_PATH], malgunbd[MAX_PATH];
    char msyh[MAX_PATH], msyhbd[MAX_PATH], simsunb[MAX_PATH], simhei[MAX_PATH];
    char arial[MAX_PATH], arialbd[MAX_PATH];

    snprintf(segoeui,  MAX_PATH, "%s\\Fonts\\segoeui.ttf", winDir);
    snprintf(segoeuib, MAX_PATH, "%s\\Fonts\\segoeuib.ttf", winDir);
    snprintf(seguisym, MAX_PATH, "%s\\Fonts\\seguisym.ttf", winDir);
    snprintf(malgun,   MAX_PATH, "%s\\Fonts\\malgun.ttf", winDir);
    snprintf(malgunbd, MAX_PATH, "%s\\Fonts\\malgunbd.ttf", winDir);
    snprintf(msyh,     MAX_PATH, "%s\\Fonts\\msyh.ttc", winDir);
    snprintf(msyhbd,   MAX_PATH, "%s\\Fonts\\msyhbd.ttc", winDir);
    snprintf(simsunb,  MAX_PATH, "%s\\Fonts\\simsunb.ttf", winDir);
    snprintf(simhei,   MAX_PATH, "%s\\Fonts\\simhei.ttf", winDir);
    snprintf(arial,    MAX_PATH, "%s\\Fonts\\arial.ttf", winDir);
    snprintf(arialbd,  MAX_PATH, "%s\\Fonts\\arialbd.ttf", winDir);

    // Latin Extended glyph ranges for English, Spanish, German, French
    static const ImWchar latinExtendedRanges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement (é, è, ê, à, ç, ä, ö, ü, ß, ñ, etc.)
        0x0100, 0x024F, // Latin Extended-A & B
        0x2000, 0x206F, // General Punctuation (¿, ¡, quotes, dashes)
        0
    };

    // 1. Default Latin Font (Regular)
    const char* defPath = FileExists(segoeui) ? segoeui : (FileExists(arial) ? arial : nullptr);
    if (defPath) {
        g_fontDefault = io.Fonts->AddFontFromFileTTF(defPath, 15.0f, nullptr, latinExtendedRanges);
    }
    if (!g_fontDefault) {
        g_fontDefault = io.Fonts->AddFontDefault();
    }

    // 2. Default Latin Font (Bold)
    const char* defBoldPath = FileExists(segoeuib) ? segoeuib : (FileExists(arialbd) ? arialbd : defPath);
    if (defBoldPath) {
        g_fontDefaultBold = io.Fonts->AddFontFromFileTTF(defBoldPath, 15.0f, nullptr, latinExtendedRanges);
    }
    if (!g_fontDefaultBold) g_fontDefaultBold = g_fontDefault;

    // 3. Symbols for Minimap
    static const ImWchar symRanges[] = {
        0x03A9, 0x03A9, 0x2227, 0x2227, 0x2248, 0x2248, 0x2302, 0x2302,
        0x25B2, 0x25B2, 0x2620, 0x2620, 0x265C, 0x265C, 0x2663, 0x2663,
        0x25C6, 0x25C6, 0x25C7, 0x25C7, 0
    };
    if (FileExists(seguisym)) {
        g_fontSymbols = io.Fonts->AddFontFromFileTTF(seguisym, 18.0f, nullptr, symRanges);
    }
    if (!g_fontSymbols) g_fontSymbols = g_fontDefault;

    // 4. Korean Font (Malgun Gothic)
    if (FileExists(malgun)) {
        ImFontConfig cfg;
        cfg.OversampleH = 2;
        g_fontKorean = io.Fonts->AddFontFromFileTTF(malgun, 16.0f, &cfg, io.Fonts->GetGlyphRangesKorean());
        Log("Fonts: Korean font (Malgun Gothic) loaded");
    }
    if (!g_fontKorean) g_fontKorean = g_fontDefault;

    if (FileExists(malgunbd)) {
        ImFontConfig cfg;
        cfg.OversampleH = 2;
        g_fontKoreanBold = io.Fonts->AddFontFromFileTTF(malgunbd, 16.0f, &cfg, io.Fonts->GetGlyphRangesKorean());
    }
    if (!g_fontKoreanBold) g_fontKoreanBold = g_fontKorean;

    // 5. Chinese Font (Microsoft YaHei or SimHei or SimSun)
    const char* cnPath = FileExists(msyh) ? msyh : (FileExists(simsunb) ? simsunb : (FileExists(simhei) ? simhei : nullptr));
    if (cnPath) {
        ImFontConfig cfg;
        cfg.FontNo = 0; // Essential for .ttc font collections
        cfg.OversampleH = 2;
        g_fontChinese = io.Fonts->AddFontFromFileTTF(cnPath, 16.0f, &cfg, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        Log("Fonts: Chinese font loaded from %s", cnPath);
    }
    if (!g_fontChinese) g_fontChinese = g_fontDefault;

    const char* cnBoldPath = FileExists(msyhbd) ? msyhbd : (FileExists(simsunb) ? simsunb : cnPath);
    if (cnBoldPath) {
        ImFontConfig cfg;
        cfg.FontNo = 0;
        cfg.OversampleH = 2;
        g_fontChineseBold = io.Fonts->AddFontFromFileTTF(cnBoldPath, 16.0f, &cfg, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    }
    if (!g_fontChineseBold) g_fontChineseBold = g_fontChinese;

    Log("Fonts: All font configurations loaded (regular & bold)");
}
