#include "il2cpp.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include "MinHook.h"

t_domain_get               il2cpp_domain_get = nullptr;
t_thread_attach            il2cpp_thread_attach = nullptr;
t_thread_detach            il2cpp_thread_detach = nullptr;
t_domain_assembly_open     il2cpp_domain_assembly_open = nullptr;
t_assembly_get_image       il2cpp_assembly_get_image = nullptr;
t_class_from_name          il2cpp_class_from_name = nullptr;
t_class_get_methods        il2cpp_class_get_methods = nullptr;
t_method_get_name          il2cpp_method_get_name = nullptr;
t_method_get_param_count   il2cpp_method_get_param_count = nullptr;
t_method_is_generic        il2cpp_method_is_generic = nullptr;
t_class_get_field_from_name il2cpp_class_get_field_from_name = nullptr;
t_field_get_offset         il2cpp_field_get_offset = nullptr;
t_field_static_get_value   il2cpp_field_static_get_value = nullptr;
t_field_static_set_value   il2cpp_field_static_set_value = nullptr;
t_string_new               il2cpp_string_new = nullptr;
t_array_length             il2cpp_array_length = nullptr;
t_class_get_type           il2cpp_class_get_type = nullptr;
t_type_get_object          il2cpp_type_get_object = nullptr;

t_GetComponentObj fn_get_gameObject = nullptr;
t_GetComponentObj fn_get_transform = nullptr;
t_GetPosition     fn_get_position = nullptr;
t_SetPosition     fn_set_position = nullptr;
t_FindObjectsOfTypeAll fn_FindAll = nullptr;
t_GetComponentT  fn_GetComponent = nullptr;
t_GetActiveInHierarchy fn_ActiveInHier = nullptr;
t_SpawnGO fn_SpawnGO = nullptr;
t_set_lockState fn_set_lockState = nullptr;
t_set_visible   fn_set_visible   = nullptr;

void *mi_get_gameObject = nullptr, *mi_get_transform = nullptr, *mi_get_position = nullptr;
void *mi_set_position = nullptr, *mi_FindAll = nullptr, *mi_GetComponent = nullptr, *mi_ActiveInHier = nullptr, *mi_SpawnGO = nullptr;
void *mi_set_lockState = nullptr, *mi_set_visible = nullptr;

static char g_logPath[MAX_PATH] = "";
static CRITICAL_SECTION g_logCs;
static bool g_logCsInited = false;
static std::vector<LogMessage> g_logHistory;
static const size_t MAX_LOG_HISTORY = 600;

static void InitLogPath() {
    if (g_logPath[0] != '\0') return;
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    char* slash = strrchr(path, '\\');
    if (slash) *(slash + 1) = '\0';
    snprintf(g_logPath, MAX_PATH, "%sKingdomEnhanced.log", path);
    if (!g_logCsInited) {
        InitializeCriticalSection(&g_logCs);
        g_logCsInited = true;
    }
}

void Log(const char* fmt, ...) {
    InitLogPath();
    EnterCriticalSection(&g_logCs);
    SYSTEMTIME t; GetLocalTime(&t);
    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t.wHour, t.wMinute, t.wSecond);

    char msgBuf[256];
    va_list ap; va_start(ap, fmt); vsnprintf(msgBuf, sizeof(msgBuf), fmt, ap); va_end(ap);

    FILE* f = fopen(g_logPath, "a");
    if (f) {
        fprintf(f, "[%s] %s\n", timeBuf, msgBuf);
        fclose(f);
    }

    LogMessage lm{};
    strcpy_s(lm.time, timeBuf);
    lm.type = LogType::Info;
    strcpy_s(lm.text, msgBuf);
    if (g_logHistory.size() >= MAX_LOG_HISTORY) {
        g_logHistory.erase(g_logHistory.begin(), g_logHistory.begin() + 50);
    }
    g_logHistory.push_back(lm);

    LeaveCriticalSection(&g_logCs);
}

void LogAction(const char* action, LogType type, const char* fmt, ...) {
    InitLogPath();
    EnterCriticalSection(&g_logCs);
    SYSTEMTIME t; GetLocalTime(&t);
    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t.wHour, t.wMinute, t.wSecond);

    char detailBuf[256];
    va_list ap; va_start(ap, fmt); vsnprintf(detailBuf, sizeof(detailBuf), fmt, ap); va_end(ap);

    const char* statusTag = "[INFO]";
    if (type == LogType::Success) statusTag = "[SUCCESS]";
    else if (type == LogType::Error) statusTag = "[FAILED]";
    else if (type == LogType::Action) statusTag = "[ACTION]";

    char lineBuf[384];
    snprintf(lineBuf, sizeof(lineBuf), "[%s] %s: %s", action, statusTag, detailBuf);

    FILE* f = fopen(g_logPath, "a");
    if (f) {
        fprintf(f, "[%s] %s\n", timeBuf, lineBuf);
        fclose(f);
    }

    LogMessage lm{};
    strcpy_s(lm.time, timeBuf);
    lm.type = type;
    snprintf(lm.text, sizeof(lm.text), "[%s] %s", action, detailBuf);
    if (g_logHistory.size() >= MAX_LOG_HISTORY) {
        g_logHistory.erase(g_logHistory.begin(), g_logHistory.begin() + 50);
    }
    g_logHistory.push_back(lm);

    LeaveCriticalSection(&g_logCs);
}

void LogAction(const char* action, bool success, const char* fmt, ...) {
    char detailBuf[256];
    va_list ap; va_start(ap, fmt); vsnprintf(detailBuf, sizeof(detailBuf), fmt, ap); va_end(ap);
    LogAction(action, success ? LogType::Success : LogType::Error, "%s", detailBuf);
}

std::vector<LogMessage> GetLogBufferCopy() {
    InitLogPath();
    EnterCriticalSection(&g_logCs);
    std::vector<LogMessage> copy = g_logHistory;
    LeaveCriticalSection(&g_logCs);
    return copy;
}

void ClearLogBuffer() {
    InitLogPath();
    EnterCriticalSection(&g_logCs);
    g_logHistory.clear();
    FILE* f = fopen(g_logPath, "w");
    if (f) {
        SYSTEMTIME t; GetLocalTime(&t);
        fprintf(f, "[%02d:%02d:%02d] --- Log Cleared by User ---\n", t.wHour, t.wMinute, t.wSecond);
        fclose(f);
    }
    LeaveCriticalSection(&g_logCs);
}

const char* GetLogFilePath() {
    InitLogPath();
    return g_logPath;
}


bool InitIL2CPP(HMODULE ga) {
    if (!ga) return false;

    #define BIND(name) name = (decltype(name))GetProcAddress(ga, #name); if(!name) Log("Missing il2cpp export: %s", #name);
    BIND(il2cpp_domain_get);
    BIND(il2cpp_thread_attach);
    BIND(il2cpp_thread_detach);
    BIND(il2cpp_domain_assembly_open);
    BIND(il2cpp_assembly_get_image);
    BIND(il2cpp_class_from_name);
    BIND(il2cpp_class_get_methods);
    BIND(il2cpp_method_get_name);
    BIND(il2cpp_method_get_param_count);
    BIND(il2cpp_method_is_generic);
    BIND(il2cpp_class_get_field_from_name);
    BIND(il2cpp_field_get_offset);
    BIND(il2cpp_field_static_get_value);
    BIND(il2cpp_field_static_set_value);
    BIND(il2cpp_string_new);
    BIND(il2cpp_array_length);
    BIND(il2cpp_class_get_type);
    BIND(il2cpp_type_get_object);
    #undef BIND

    EnsureAttached();


    void* domain = il2cpp_domain_get ? il2cpp_domain_get() : nullptr;
    if (domain && il2cpp_thread_attach) {
        il2cpp_thread_attach(domain);
    }

    // Resolve fundamental UnityEngine.Component / GameObject / Transform methods
    void* cComponent = FindClass("UnityEngine.CoreModule", "UnityEngine", "Component");
    if (cComponent) {
        mi_get_gameObject = FindMethod(cComponent, "get_gameObject", 0);
        mi_get_transform  = FindMethod(cComponent, "get_transform", 0);
        fn_get_gameObject = (t_GetComponentObj)MethodPtr(mi_get_gameObject);
        fn_get_transform  = (t_GetComponentObj)MethodPtr(mi_get_transform);
    }

    void* cTransform = FindClass("UnityEngine.CoreModule", "UnityEngine", "Transform");
    if (cTransform) {
        mi_get_position = FindMethod(cTransform, "get_position", 0);
        mi_set_position = FindMethod(cTransform, "set_position", 1);
        fn_get_position = (t_GetPosition)MethodPtr(mi_get_position);
        fn_set_position = (t_SetPosition)MethodPtr(mi_set_position);
    }

    void* cResources = FindClass("UnityEngine.CoreModule", "UnityEngine", "Resources");
    if (cResources) {
        mi_FindAll = FindMethod(cResources, "FindObjectsOfTypeAll", 1);
        fn_FindAll = (t_FindObjectsOfTypeAll)MethodPtr(mi_FindAll);
    }

    void* cGO = FindClass("UnityEngine.CoreModule", "UnityEngine", "GameObject");
    if (cGO) {
        mi_GetComponent = FindMethod(cGO, "GetComponent", 1);
        mi_ActiveInHier = FindMethod(cGO, "get_activeInHierarchy", 0);
        fn_GetComponent = (t_GetComponentT)MethodPtr(mi_GetComponent);
        fn_ActiveInHier = (t_GetActiveInHierarchy)MethodPtr(mi_ActiveInHier);
    }

    void* cCursor = FindClass("UnityEngine.CoreModule", "UnityEngine", "Cursor");
    if (cCursor) {
        mi_set_lockState = FindMethod(cCursor, "set_lockState", 1);
        mi_set_visible   = FindMethod(cCursor, "set_visible", 1);
        fn_set_lockState = (t_set_lockState)MethodPtr(mi_set_lockState);
        fn_set_visible   = (t_set_visible)MethodPtr(mi_set_visible);
        Log("InitIL2CPP: UnityEngine.Cursor found (lockState=%p, visible=%p)", fn_set_lockState, fn_set_visible);
    }

    Log("InitIL2CPP: core reflection initialized successfully");
    return true;
}


void* FindClass(const char* assemblyName, const char* namespaze, const char* className) {
    EnsureAttached();
    if (!il2cpp_domain_get || !il2cpp_domain_assembly_open || !il2cpp_assembly_get_image || !il2cpp_class_from_name)
        return nullptr;


    void* domain = il2cpp_domain_get();
    if (!domain) return nullptr;

    void* assembly = il2cpp_domain_assembly_open(domain, assemblyName);
    if (!assembly) return nullptr;

    void* image = il2cpp_assembly_get_image(assembly);
    if (!image) return nullptr;

    return il2cpp_class_from_name(image, namespaze, className);
}

void* FindMethod(void* klass, const char* name, int argc, bool allowGeneric) {
    if (!klass || !il2cpp_class_get_methods) return nullptr;
    void* iter = nullptr;
    void* m = nullptr;
    while ((m = il2cpp_class_get_methods(klass, &iter)) != nullptr) {
        if (strcmp(il2cpp_method_get_name(m), name) != 0) continue;
        if ((int)il2cpp_method_get_param_count(m) != argc) continue;
        if (!allowGeneric && il2cpp_method_is_generic(m)) continue;
        return m;
    }
    return nullptr;
}

void* MethodPtr(void* mi) {
    return mi ? *(void**)mi : nullptr;
}

size_t FieldOffset(void* klass, const char* name) {
    if (!klass || !il2cpp_class_get_field_from_name || !il2cpp_field_get_offset) return 0;
    void* f = il2cpp_class_get_field_from_name(klass, name);
    return f ? il2cpp_field_get_offset(f) : 0;
}

bool HookMethod(void* klass, const char* name, int argc, void* detour, void** orig) {
    void* mi = FindMethod(klass, name, argc, false);
    void* p = MethodPtr(mi);
    if (!p) {
        Log("HookMethod: method not found: %s/%d", name, argc);
        return false;
    }
    bool ok = (MH_CreateHook(p, detour, orig) == MH_OK && MH_EnableHook(p) == MH_OK);
    Log("HookMethod: %s/%d (%p) -> %s", name, argc, p, ok ? "OK" : "FAILED");
    return ok;
}
