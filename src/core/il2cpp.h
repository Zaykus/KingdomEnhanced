#pragma once
#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

// Common Unity Types
struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };
struct Vector4 { float x, y, z, w; };
struct Quaternion { float x, y, z, w; };
struct Color { float r, g, b, a; };

// Function Pointer Typedefs for IL2CPP C API
typedef void* (*t_domain_get)();
typedef void* (*t_thread_attach)(void*);
typedef void  (*t_thread_detach)(void*);
typedef void* (*t_domain_assembly_open)(void*, const char*);
typedef void* (*t_assembly_get_image)(void*);
typedef void* (*t_class_from_name)(void*, const char*, const char*);
typedef void* (*t_class_get_methods)(void*, void**);
typedef const char* (*t_method_get_name)(void*);
typedef unsigned (*t_method_get_param_count)(void*);
typedef bool (*t_method_is_generic)(void*);
typedef void* (*t_class_get_field_from_name)(void*, const char*);
typedef size_t (*t_field_get_offset)(void*);
typedef void (*t_field_static_get_value)(void* field, void* value);
typedef void (*t_field_static_set_value)(void* field, void* value);
typedef void* (*t_string_new)(const char*);
typedef unsigned (*t_array_length)(void*);
typedef void* (*t_class_get_type)(void*);
typedef void* (*t_type_get_object)(void*);
typedef int (*t_il2cpp_init)(const char*);

extern t_domain_get               il2cpp_domain_get;
extern t_thread_attach            il2cpp_thread_attach;
extern t_thread_detach            il2cpp_thread_detach;
extern t_domain_assembly_open     il2cpp_domain_assembly_open;

inline void EnsureAttached() {
    static thread_local void* s_attachedThread = nullptr;
    if (!s_attachedThread && il2cpp_domain_get && il2cpp_thread_attach) {
        void* domain = il2cpp_domain_get();
        if (domain) {
            s_attachedThread = il2cpp_thread_attach(domain);
        }
    }
}

extern t_assembly_get_image       il2cpp_assembly_get_image;
extern t_class_from_name          il2cpp_class_from_name;
extern t_class_get_methods        il2cpp_class_get_methods;
extern t_method_get_name          il2cpp_method_get_name;
extern t_method_get_param_count   il2cpp_method_get_param_count;
extern t_method_is_generic        il2cpp_method_is_generic;
extern t_class_get_field_from_name il2cpp_class_get_field_from_name;
extern t_field_get_offset         il2cpp_field_get_offset;
extern t_field_static_get_value   il2cpp_field_static_get_value;
extern t_field_static_set_value   il2cpp_field_static_set_value;
extern t_string_new               il2cpp_string_new;
extern t_array_length             il2cpp_array_length;
extern t_class_get_type           il2cpp_class_get_type;
extern t_type_get_object          il2cpp_type_get_object;

// Unity Game Component Method Types
typedef void*   (*t_GetComponentObj)(void* self, const void* mi);
typedef Vector3 (*t_GetPosition)(void* self, const void* mi);
typedef void    (*t_SetPosition)(void* self, Vector3 pos, const void* mi);
typedef void*   (*t_FindObjectsOfTypeAll)(void* type, const void* mi);
typedef void*   (*t_GetComponentT)(void* go, void* type, const void* mi);
typedef bool    (*t_GetActiveInHierarchy)(void* go, const void* mi);
typedef void*   (*t_SpawnGO)(void* prefab, Vector3 pos, Quaternion rot, void* parent,
                             bool allowInstantiate, bool allowCreatePool, bool assertNonNull, const void* mi);
typedef void    (*t_set_lockState)(int lockMode, const void* mi);
typedef void    (*t_set_visible)(bool visible, const void* mi);

extern t_GetComponentObj fn_get_gameObject;
extern t_GetComponentObj fn_get_transform;
extern t_GetPosition     fn_get_position;
extern t_SetPosition     fn_set_position;
extern t_FindObjectsOfTypeAll fn_FindAll;
extern t_GetComponentT  fn_GetComponent;
extern t_GetActiveInHierarchy fn_ActiveInHier;
extern t_SpawnGO        fn_SpawnGO;
extern t_set_lockState  fn_set_lockState;
extern t_set_visible    fn_set_visible;

extern void *mi_get_gameObject, *mi_get_transform, *mi_get_position, *mi_set_position, *mi_FindAll, *mi_GetComponent, *mi_ActiveInHier, *mi_SpawnGO;
extern void *mi_set_lockState, *mi_set_visible;


// Core Log Functions & Buffer
enum class LogType { Info, Action, Success, Error };
struct LogMessage {
    char time[16];
    LogType type;
    char text[384];
};

void Log(const char* fmt, ...);
void LogAction(const char* action, LogType type, const char* fmt, ...);
void LogAction(const char* action, bool success, const char* fmt, ...);
std::vector<LogMessage> GetLogBufferCopy();
void ClearLogBuffer();
const char* GetLogFilePath();

// Reflection & Hooking Utilities
bool InitIL2CPP(HMODULE ga);
void* FindClass(const char* assemblyName, const char* namespaze, const char* className);
void* FindMethod(void* klass, const char* name, int argc, bool allowGeneric = false);
void* MethodPtr(void* mi);
size_t FieldOffset(void* klass, const char* name);
bool HookMethod(void* klass, const char* name, int argc, void* detour, void** orig);

// Generic IL2CPP Field Access Template Helpers
template<typename T>
inline T GetField(void* obj, size_t offset) {
    if (!obj || offset == 0) return T{};
    return *(T*)((char*)obj + offset);
}

template<typename T>
inline void SetField(void* obj, size_t offset, T value) {
    if (!obj || offset == 0) return;
    *(T*)((char*)obj + offset) = value;
}
