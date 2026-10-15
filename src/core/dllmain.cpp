#include <windows.h>
#include <shlwapi.h>
#include <cstdio>
#include "MinHook.h"
#include "il2cpp.h"
#include "config.h"

// Forward declarations for other subsystem initializers
void InitFeatures();
void StartDX11Hook();

// ---------------- version.dll proxy ----------------
extern "C" void* g_real[17];
void* g_real[17] = {0};
static const char* kExports[17] = {
    "GetFileVersionInfoA", "GetFileVersionInfoByHandle", "GetFileVersionInfoExA",
    "GetFileVersionInfoExW", "GetFileVersionInfoSizeA", "GetFileVersionInfoSizeExA",
    "GetFileVersionInfoSizeExW", "GetFileVersionInfoSizeW", "GetFileVersionInfoW",
    "VerFindFileA", "VerFindFileW", "VerInstallFileA", "VerInstallFileW",
    "VerLanguageNameA", "VerLanguageNameW", "VerQueryValueA", "VerQueryValueW"
};

static void LoadRealVersion() {
    char path[MAX_PATH];
    GetSystemDirectoryA(path, MAX_PATH);
    strcat(path, "\\version.dll");
    HMODULE h = LoadLibraryA(path);
    if (!h) return;
    for (int i = 0; i < 17; i++) {
        g_real[i] = (void*)GetProcAddress(h, kExports[i]);
    }
}

static volatile LONG g_resolved = 0;
static t_il2cpp_init o_il2cpp_init = nullptr;



static void Resolve() {
    if (InterlockedCompareExchange(&g_resolved, 1, 0) != 0) return;

    HMODULE ga = GetModuleHandleA("GameAssembly.dll");
    if (!ga) {
        Log("Resolve: GameAssembly.dll not found in process memory");
        return;
    }

    Log("Resolve: initializing IL2CPP runtime hooks...");
    if (!InitIL2CPP(ga)) {
        Log("Resolve: InitIL2CPP failed!");
        return;
    }

    // Initialize all game feature hooks and subsystems
    InitFeatures();

    // Start DX11 swap chain overlay hook
    StartDX11Hook();

    Log("Kingdom Enhanced is READY!");
}

static int hk_il2cpp_init(const char* name) {
    int r = o_il2cpp_init(name);
    Log("il2cpp_init called by Unity engine");
    Resolve();
    return r;
}

typedef HMODULE (WINAPI *t_LoadLibraryExW)(LPCWSTR, HANDLE, DWORD);
static t_LoadLibraryExW o_LoadLibraryExW = nullptr;
static volatile LONG g_initHooked = 0;

static void TryHookInit(HMODULE ga) {
    if (!ga || InterlockedCompareExchange(&g_initHooked, 1, 0) != 0) return;
    void* p = (void*)GetProcAddress(ga, "il2cpp_init");
    if (p && MH_CreateHook(p, (void*)hk_il2cpp_init, (void**)&o_il2cpp_init) == MH_OK && MH_EnableHook(p) == MH_OK) {
        Log("il2cpp_init hook attached successfully");
    } else {
        Log("il2cpp_init hook attachment failed");
    }
}

static HMODULE WINAPI hk_LoadLibraryExW(LPCWSTR name, HANDLE file, DWORD flags) {
    HMODULE h = o_LoadLibraryExW(name, file, flags);
    if (h && name && !g_initHooked && wcsstr(name, L"GameAssembly")) {
        Log("GameAssembly.dll loaded via LoadLibraryExW");
        TryHookInit(h);
    }
    return h;
}

static DWORD WINAPI WatchdogThread(LPVOID) {
    DWORD seen = 0;
    for (int i = 0; i < 300 && !g_resolved; i++) {
        Sleep(1000);
        HMODULE ga = GetModuleHandleA("GameAssembly.dll");
        if (!seen && ga) {
            seen = GetTickCount();
            Log("Watchdog: GameAssembly.dll detected");
            TryHookInit(ga);
        }
        if (seen && !g_resolved && GetTickCount() - seen > 15000) {
            Log("Watchdog: resolving directly");
            Resolve();
            break;
        }
    }
    // Prevent thread termination while attached to IL2CPP GC
    while (g_resolved) {
        Sleep(60000);
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID) {
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(hinst);
    LoadRealVersion();

    char exe[MAX_PATH], data[MAX_PATH];
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    strcpy(data, exe);
    char* dot = strrchr(data, '.');
    if (dot) strcpy(dot, "_Data");
    const char* exeName = strrchr(exe, '\\') ? strrchr(exe, '\\') + 1 : exe;
    DWORD attr = GetFileAttributesA(data);
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY) || StrStrIA(exeName, "CrashHandler")) {
        return TRUE;
    }

    Log("Kingdom Enhanced injected into %s (PID %lu)", exeName, GetCurrentProcessId());
    
    // Load config from KingdomEnhanced.ini
    LoadConfig();

    if (MH_Initialize() != MH_OK) {
        Log("MH_Initialize failed!");
        return TRUE;
    }

    CreateThread(nullptr, 0, WatchdogThread, nullptr, 0, nullptr);

    HMODULE ga = GetModuleHandleA("GameAssembly.dll");
    if (ga) {
        TryHookInit(ga);
        return TRUE;
    }


    HMODULE kb = GetModuleHandleA("kernelbase.dll");
    void* pLL = kb ? (void*)GetProcAddress(kb, "LoadLibraryExW") : nullptr;
    if (pLL && MH_CreateHook(pLL, (void*)hk_LoadLibraryExW, (void**)&o_LoadLibraryExW) == MH_OK && MH_EnableHook(pLL) == MH_OK) {
        Log("Waiting for GameAssembly.dll via LoadLibraryExW hook...");
    } else {
        Log("LoadLibraryExW hook failed; watchdog will handle resolution.");
    }

    return TRUE;
}
