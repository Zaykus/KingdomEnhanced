#include "dx11_hook.h"
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <cstdio>
#include "MinHook.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_dx12.h"
#include "fonts.h"
#include "config.h"
#include "il2cpp.h"

bool g_menuVisible = false;
bool g_mapVisible = true;
bool g_staminaVisible = true;

// Forward render hooks
void RenderModMenu();
void RenderMinimap(ImDrawList* draw, float screenWidth);
void RenderStaminaBar(ImDrawList* draw, int playerIdx);
void RenderKingdomMonitorHud();
extern bool g_monitorHudVisible;

// D3DCompile dynamic link shim
typedef HRESULT (WINAPI *t_D3DCompile)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*,
                                       LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
extern "C" HRESULT WINAPI D3DCompile(LPCVOID src, SIZE_T size, LPCSTR name, const D3D_SHADER_MACRO* defs,
                                     ID3DInclude* inc, LPCSTR entry, LPCSTR target, UINT f1, UINT f2,
                                     ID3DBlob** code, ID3DBlob** errs) {
    static t_D3DCompile fn = nullptr;
    if (!fn) {
        const char* dlls[] = { "d3dcompiler_47.dll", "d3dcompiler_46.dll", "d3dcompiler_43.dll" };
        for (const char* d : dlls) {
            HMODULE h = LoadLibraryA(d);
            if (h && (fn = (t_D3DCompile)GetProcAddress(h, "D3DCompile"))) break;
        }
        if (!fn) {
            Log("Overlay: d3dcompiler not found!");
            return E_FAIL;
        }
    }
    return fn(src, size, name, defs, inc, entry, target, f1, f2, code, errs);
}

// ---------------- Swap Chain & Command Queue Hooks ----------------
typedef HRESULT (STDMETHODCALLTYPE *t_Present)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT (STDMETHODCALLTYPE *t_ResizeBuffers)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
typedef void    (STDMETHODCALLTYPE *t_ExecuteCommandLists)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);

static t_Present            o_Present = nullptr;
static t_ResizeBuffers      o_ResizeBuffers = nullptr;
static t_ExecuteCommandLists o_ExecuteCommandLists = nullptr;

static HWND                 g_hwnd = nullptr;
static WNDPROC              o_WndProc = nullptr;
static bool                 g_inited = false;
static bool                 g_isD3D12 = false;
static bool                 g_apiDetected = false;
static LARGE_INTEGER        g_qpf, g_lastQpc;

// D3D11 state
static ID3D11Device*           g_dev11 = nullptr;
static ID3D11DeviceContext*    g_ctx11 = nullptr;
static ID3D11RenderTargetView* g_rtv11 = nullptr;
static IDXGISwapChain*         g_sc11  = nullptr;

// D3D12 state
struct FrameContext12 {
    ID3D12CommandAllocator*     commandAllocator = nullptr;
    ID3D12GraphicsCommandList*  commandList = nullptr;
    ID3D12Resource*             renderTarget = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
    UINT64                      fenceValue = 0;
};

static FrameContext12          g_frames12[8] = {};
static UINT                    g_bufferCount12 = 0;
static ID3D12Device*           g_dev12 = nullptr;
static ID3D12DescriptorHeap*   g_rtvDescHeap12 = nullptr;
static ID3D12DescriptorHeap*   g_srvDescHeap12 = nullptr;
static ID3D12CommandQueue*     g_cmdQueue12 = nullptr;
static ID3D12Fence*            g_fence12 = nullptr;
static HANDLE                  g_fenceEvent12 = nullptr;
static UINT64                  g_fenceLastSignaled12 = 0;

// Win32 Cursor un-trap hooks
typedef BOOL (WINAPI *t_SetCursorPos)(int X, int Y);
typedef BOOL (WINAPI *t_ClipCursor)(const RECT* lpRect);
static t_SetCursorPos o_SetCursorPos = nullptr;
static t_ClipCursor   o_ClipCursor   = nullptr;

static BOOL WINAPI hk_SetCursorPos(int X, int Y) {
    if (g_menuVisible) {
        return TRUE; // Block Unity from centering/locking cursor while menu is open
    }
    return o_SetCursorPos ? o_SetCursorPos(X, Y) : SetCursorPos(X, Y);
}

static BOOL WINAPI hk_ClipCursor(const RECT* lpRect) {
    if (g_menuVisible) {
        return o_ClipCursor ? o_ClipCursor(nullptr) : TRUE; // Keep mouse completely unclipped
    }
    return o_ClipCursor ? o_ClipCursor(lpRect) : TRUE;
}

static const GUID kIID_ID3D12Device       = { 0x189819f1, 0x1db6, 0x4b57, { 0xbe, 0x54, 0x18, 0x21, 0x33, 0x9b, 0x85, 0xf7 } };
static const GUID kIID_ID3D12CommandQueue  = { 0x0ec870a6, 0x5d7e, 0x4c22, { 0x8c, 0xfc, 0x5b, 0xaa, 0xe0, 0x76, 0x16, 0xed } };
static const GUID kIID_ID3D12Resource      = { 0x696442be, 0xa72e, 0x4059, { 0xbc, 0x79, 0x5b, 0x5c, 0x98, 0x04, 0x0f, 0xad } };
static const GUID kIID_IDXGISwapChain3     = { 0x94d99bdb, 0xf1f8, 0x4ab0, { 0xb2, 0x36, 0x7d, 0xa0, 0x17, 0x0e, 0xda, 0xb1 } };
static const GUID kIID_IDXGIFactory        = { 0x7b7166ec, 0x21c7, 0x44ae, { 0xb2, 0x1a, 0xc9, 0xae, 0x32, 0x1a, 0xe3, 0x69 } };
static SRWLOCK    g_imguiLock              = SRWLOCK_INIT;

// In-Game Style Medieval Royal Cursor
static void DrawMedievalGameCursor(ImDrawList* draw, ImVec2 pos) {
    if (!draw) return;
    float x = pos.x;
    float y = pos.y;

    // Outer shadow / dark outline
    ImVec2 shadowPoints[] = {
        ImVec2(x + 1.0f, y + 1.0f),
        ImVec2(x + 1.0f, y + 19.0f),
        ImVec2(x + 5.0f, y + 15.0f),
        ImVec2(x + 9.0f, y + 23.0f),
        ImVec2(x + 13.0f, y + 21.0f),
        ImVec2(x + 9.0f, y + 13.0f),
        ImVec2(x + 15.0f, y + 13.0f)
    };
    draw->AddPolyline(shadowPoints, 7, IM_COL32(20, 15, 10, 200), ImDrawFlags_Closed, 3.5f);

    // Dark bronze border
    ImVec2 outerPoints[] = {
        ImVec2(x, y),
        ImVec2(x, y + 17.0f),
        ImVec2(x + 4.0f, y + 13.0f),
        ImVec2(x + 8.0f, y + 21.0f),
        ImVec2(x + 11.5f, y + 19.5f),
        ImVec2(x + 7.5f, y + 12.0f),
        ImVec2(x + 13.0f, y + 12.0f)
    };
    draw->AddConvexPolyFilled(outerPoints, 7, IM_COL32(35, 25, 15, 255));

    // Gold body
    ImVec2 innerPoints[] = {
        ImVec2(x + 1.5f, y + 2.5f),
        ImVec2(x + 1.5f, y + 14.5f),
        ImVec2(x + 4.5f, y + 11.5f),
        ImVec2(x + 8.5f, y + 19.5f),
        ImVec2(x + 10.2f, y + 18.5f),
        ImVec2(x + 6.5f, y + 11.0f),
        ImVec2(x + 11.0f, y + 11.0f)
    };
    draw->AddConvexPolyFilled(innerPoints, 7, IM_COL32(240, 191, 38, 255)); // Kingdom Gold

    // Bright core glint
    ImVec2 glintPoints[] = {
        ImVec2(x + 2.0f, y + 4.0f),
        ImVec2(x + 2.0f, y + 11.0f),
        ImVec2(x + 5.0f, y + 9.5f)
    };
    draw->AddConvexPolyFilled(glintPoints, 3, IM_COL32(255, 240, 160, 255));

    // Royal Amber Gem in the center
    draw->AddCircleFilled(ImVec2(x + 4.5f, y + 8.5f), 2.0f, IM_COL32(255, 120, 20, 255));
    draw->AddCircleFilled(ImVec2(x + 4.0f, y + 8.0f), 0.8f, IM_COL32(255, 230, 180, 255));
}

static LRESULT CALLBACK hk_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Window subclassing helpers (supports both Unicode and ANSI windows)
static inline LRESULT CallOrigWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!o_WndProc) return DefWindowProcW(hwnd, msg, wParam, lParam);
    if (IsWindowUnicode(hwnd))
        return CallWindowProcW(o_WndProc, hwnd, msg, wParam, lParam);
    else
        return CallWindowProcA(o_WndProc, hwnd, msg, wParam, lParam);
}

static inline void SubclassWindow(HWND hwnd) {
    if (!hwnd || hwnd == g_hwnd) return;
    if (o_WndProc && g_hwnd) {
        if (IsWindowUnicode(g_hwnd))
            SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)o_WndProc);
        else
            SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, (LONG_PTR)o_WndProc);
    }
    g_hwnd = hwnd;
    if (IsWindowUnicode(hwnd))
        o_WndProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)hk_WndProc);
    else
        o_WndProc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)hk_WndProc);
}

// Input hook
static LRESULT CALLBACK hk_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_inited) {
        if (msg == WM_KEYDOWN) {
            if (wParam == (WPARAM)g_config.menuKey) {
                g_menuVisible = !g_menuVisible;
                if (g_menuVisible) {
                    ClipCursor(nullptr);
                    ReleaseCapture();
                    SetCursor(LoadCursor(nullptr, IDC_ARROW));
                }
                Log("Overlay: Mod menu toggled %s", g_menuVisible ? "OPEN" : "CLOSED");
                return 0;
            }
            if (wParam == (WPARAM)g_config.mapKey && !(GetKeyState(VK_CONTROL) & 0x8000)) {
                g_mapVisible = !g_mapVisible;
            }
            if (wParam == 'F' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                extern bool g_mapFull;
                g_mapFull = !g_mapFull;
                Log("Overlay: Full map toggled %s", g_mapFull ? "ON" : "OFF");
            }
            if (wParam == (WPARAM)g_config.staminaKey) {
                g_staminaVisible = !g_staminaVisible;
            }
            if (wParam == VK_F4) {
                g_monitorHudVisible = !g_monitorHudVisible;
                Log("Overlay: Kingdom Monitor HUD toggled %s", g_monitorHudVisible ? "OPEN" : "CLOSED");
            }
        }

        if (g_menuVisible) {
            switch (msg) {
                case WM_SETCURSOR:
                    SetCursor(LoadCursor(nullptr, IDC_ARROW));
                    return TRUE;
                case WM_MOUSEMOVE: {
                    float x = (float)(short)LOWORD(lParam);
                    float y = (float)(short)HIWORD(lParam);
                    AcquireSRWLockExclusive(&g_imguiLock);
                    ImGui::GetIO().AddMousePosEvent(x, y);
                    ReleaseSRWLockExclusive(&g_imguiLock);
                    return 0; // Consume event so Unity doesn't rotate or click game
                }
                case WM_LBUTTONDOWN:
                case WM_LBUTTONDBLCLK:
                    AcquireSRWLockExclusive(&g_imguiLock);
                    ImGui::GetIO().AddMouseButtonEvent(0, true);
                    ReleaseSRWLockExclusive(&g_imguiLock);
                    return 0;
                case WM_LBUTTONUP:
                    AcquireSRWLockExclusive(&g_imguiLock);
                    ImGui::GetIO().AddMouseButtonEvent(0, false);
                    ReleaseSRWLockExclusive(&g_imguiLock);
                    return 0;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONDBLCLK:
                    AcquireSRWLockExclusive(&g_imguiLock);
                    ImGui::GetIO().AddMouseButtonEvent(1, true);
                    ReleaseSRWLockExclusive(&g_imguiLock);
                    return 0;
                case WM_RBUTTONUP:
                    AcquireSRWLockExclusive(&g_imguiLock);
                    ImGui::GetIO().AddMouseButtonEvent(1, false);
                    ReleaseSRWLockExclusive(&g_imguiLock);
                    return 0;
                case WM_MOUSEWHEEL:
                    AcquireSRWLockExclusive(&g_imguiLock);
                    ImGui::GetIO().AddMouseWheelEvent(0.0f, (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
                    ReleaseSRWLockExclusive(&g_imguiLock);
                    return 0;
                case WM_CHAR:
                    if (wParam > 0 && wParam < 0x10000) {
                        AcquireSRWLockExclusive(&g_imguiLock);
                        ImGui::GetIO().AddInputCharacter((unsigned short)wParam);
                        ReleaseSRWLockExclusive(&g_imguiLock);
                        return 0;
                    }
                    break;
            }
        }
    }
    return CallOrigWndProc(hwnd, msg, wParam, lParam);
}

// ---------------- D3D12 ImGui Implementation ----------------
static void STDMETHODCALLTYPE hk_ExecuteCommandLists(ID3D12CommandQueue* queue, UINT count, ID3D12CommandList* const* lists) {
    if (queue && !g_cmdQueue12) {
        D3D12_COMMAND_QUEUE_DESC desc = queue->GetDesc();
        if (desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
            g_cmdQueue12 = queue;
            Log("Overlay: Captured primary D3D12 DIRECT CommandQueue: %p", queue);
        }
    }
    o_ExecuteCommandLists(queue, count, lists);
}

static void CreateRenderTargetD3D12(IDXGISwapChain* sc) {
    if (!g_dev12 || !g_rtvDescHeap12 || !sc) return;
    SIZE_T step = g_dev12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = g_rtvDescHeap12->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < g_bufferCount12; i++) {
        g_frames12[i].rtvHandle = handle;
        HRESULT hr = sc->GetBuffer(i, kIID_ID3D12Resource, (void**)&g_frames12[i].renderTarget);
        if (SUCCEEDED(hr) && g_frames12[i].renderTarget) {
            g_dev12->CreateRenderTargetView(g_frames12[i].renderTarget, nullptr, handle);
        } else {
            Log("Overlay: D3D12 sc->GetBuffer(%u) failed: 0x%08lx", i, hr);
        }
        handle.ptr += step;
    }
}

static void CleanupRenderTargetD3D12() {
    if (g_fence12 && g_cmdQueue12) {
        g_fenceLastSignaled12++;
        g_cmdQueue12->Signal(g_fence12, g_fenceLastSignaled12);
        if (g_fence12->GetCompletedValue() < g_fenceLastSignaled12) {
            g_fence12->SetEventOnCompletion(g_fenceLastSignaled12, g_fenceEvent12);
            WaitForSingleObject(g_fenceEvent12, 1000);
        }
    }
    for (UINT i = 0; i < g_bufferCount12; i++) {
        if (g_frames12[i].renderTarget) {
            g_frames12[i].renderTarget->Release();
            g_frames12[i].renderTarget = nullptr;
        }
        g_frames12[i].fenceValue = 0;
    }
}

static bool InitImGuiD3D12(IDXGISwapChain* sc) {
    if (!g_cmdQueue12) {
        return false;
    }

    DXGI_SWAP_CHAIN_DESC desc;
    sc->GetDesc(&desc);
    g_bufferCount12 = desc.BufferCount > 8 ? 8 : desc.BufferCount;
    SubclassWindow(desc.OutputWindow);

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = g_bufferCount12;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(g_dev12->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&g_rtvDescHeap12)))) {
        Log("Overlay: D3D12 CreateDescriptorHeap RTV failed");
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.NumDescriptors = 1;
    srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(g_dev12->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&g_srvDescHeap12)))) {
        Log("Overlay: D3D12 CreateDescriptorHeap SRV failed");
        return false;
    }

    if (FAILED(g_dev12->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence12)))) {
        Log("Overlay: D3D12 CreateFence failed");
    }
    g_fenceEvent12 = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    g_fenceLastSignaled12 = 0;

    for (UINT i = 0; i < g_bufferCount12; i++) {
        if (FAILED(g_dev12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frames12[i].commandAllocator)))) {
            Log("Overlay: D3D12 CreateCommandAllocator [%u] failed", i);
            return false;
        }
        if (FAILED(g_dev12->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frames12[i].commandAllocator, nullptr, IID_PPV_ARGS(&g_frames12[i].commandList)))) {
            Log("Overlay: D3D12 CreateCommandList [%u] failed", i);
            return false;
        }
        g_frames12[i].commandList->Close();
    }

    CreateRenderTargetD3D12(sc);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    LoadAllFonts();
    ApplyCustomTheme();

    ImGui_ImplDX12_InitInfo initInfo{};
    initInfo.Device = g_dev12;
    initInfo.CommandQueue = g_cmdQueue12;
    initInfo.NumFramesInFlight = g_bufferCount12;
    initInfo.RTVFormat = (g_frames12[0].renderTarget) ? g_frames12[0].renderTarget->GetDesc().Format : desc.BufferDesc.Format;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.SrvDescriptorHeap = g_srvDescHeap12;
    initInfo.LegacySingleSrvCpuDescriptor = g_srvDescHeap12->GetCPUDescriptorHandleForHeapStart();
    initInfo.LegacySingleSrvGpuDescriptor = g_srvDescHeap12->GetGPUDescriptorHandleForHeapStart();

    if (!ImGui_ImplDX12_Init(&initInfo)) {
        Log("Overlay: ImGui_ImplDX12_Init failed!");
        return false;
    }

    // Pre-create GPU pipeline state, shaders and font texture at startup
    if (!ImGui_ImplDX12_CreateDeviceObjects()) {
        Log("Overlay: ImGui_ImplDX12_CreateDeviceObjects failed!");
        return false;
    }
    Log("Overlay: ImGui D3D12 device objects (shaders & font texture) initialized successfully");

    QueryPerformanceFrequency(&g_qpf);
    QueryPerformanceCounter(&g_lastQpc);

    Log("Overlay: DirectX 12 Hook initialized successfully on HWND %p (Buffers=%u)", g_hwnd, g_bufferCount12);
    return true;
}

static void RenderOverlayD3D12(IDXGISwapChain* sc) {
    if (!g_inited) {
        if (InitImGuiD3D12(sc)) {
            g_inited = true;
        } else {
            return;
        }
    }

    // Hotkeys are handled cleanly in hk_WndProc.
    // If no overlay is visible, DO NOT touch D3D12 state or backbuffers!
    if (!g_menuVisible && !g_mapVisible && !g_staminaVisible) {
        return;
    }

    IDXGISwapChain3* sc3 = nullptr;
    UINT backBufferIdx = 0;
    if (SUCCEEDED(sc->QueryInterface(kIID_IDXGISwapChain3, (void**)&sc3)) && sc3) {
        backBufferIdx = sc3->GetCurrentBackBufferIndex();
        sc3->Release();
    } else {
        static bool s_loggedQI = false;
        if (!s_loggedQI) {
            Log("Overlay: Warning - Failed to QueryInterface IDXGISwapChain3 from sc=%p", sc);
            s_loggedQI = true;
        }
    }
    if (backBufferIdx >= g_bufferCount12) backBufferIdx = 0;

    FrameContext12& ctx = g_frames12[backBufferIdx];
    if (!ctx.renderTarget || !ctx.commandAllocator || !ctx.commandList || !g_cmdQueue12) return;

    // 1. Wait for GPU if this specific buffer slot still has an in-flight submission from a past frame
    if (ctx.fenceValue != 0) {
        UINT64 waitVal = ctx.fenceValue;
        if (g_fence12 && g_fence12->GetCompletedValue() < waitVal) {
            g_fence12->SetEventOnCompletion(waitVal, g_fenceEvent12);
            DWORD waitRes = WaitForSingleObject(g_fenceEvent12, 1000);
            if (waitRes != WAIT_OBJECT_0) {
                Log("Overlay: D3D12 fence wait timed out (buffer=%u, val=%llu)", backBufferIdx, waitVal);
            }
        }
        ctx.fenceValue = 0;
    }

    DXGI_SWAP_CHAIN_DESC d;
    sc->GetDesc(&d);
    if (d.OutputWindow && d.OutputWindow != g_hwnd) {
        SubclassWindow(d.OutputWindow);
    }

    AcquireSRWLockExclusive(&g_imguiLock);

    ImGuiIO& io = ImGui::GetIO();
    RECT rect;
    if (g_hwnd && GetClientRect(g_hwnd, &rect) && (rect.right - rect.left > 0)) {
        io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
    } else {
        io.DisplaySize = ImVec2((float)d.BufferDesc.Width, (float)d.BufferDesc.Height);
    }

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    float dt = (float)(now.QuadPart - g_lastQpc.QuadPart) / (float)g_qpf.QuadPart;
    if (dt <= 0.0f || dt > 0.2f) dt = 1.0f / 60.0f;
    io.DeltaTime = dt;
    g_lastQpc = now;

    // Synchronize mouse position and state
    if (g_menuVisible && g_hwnd) {
        POINT pt;
        if (GetCursorPos(&pt) && ScreenToClient(g_hwnd, &pt)) {
            io.AddMousePosEvent((float)pt.x, (float)pt.y);
        }
        io.AddMouseButtonEvent(0, (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
        io.AddMouseButtonEvent(1, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
    }

    io.MouseDrawCursor = false;

    ImGui_ImplDX12_NewFrame();
    ImGui::NewFrame();

    // 1. Mod Menu (F1)
    if (g_menuVisible) {
        RenderModMenu();
    }

    // 2. Top Minimap (M / Ctrl+F)
    if (g_mapVisible && g_config.enableMinimap) {
        RenderMinimap(ImGui::GetForegroundDrawList(), io.DisplaySize.x);
    }

    // 3. Mount Stamina Bar (N)
    if (g_staminaVisible && g_config.enableStaminaBar) {
        RenderStaminaBar(ImGui::GetForegroundDrawList(), 0);
        RenderStaminaBar(ImGui::GetForegroundDrawList(), 1);
    }

    // 4. Kingdom Monitor HUD (F4)
    if (g_monitorHudVisible) {
        RenderKingdomMonitorHud();
    }

    // 5. In-Game Style Medieval Cursor
    if (g_menuVisible) {
        DrawMedievalGameCursor(ImGui::GetForegroundDrawList(), io.MousePos);
    }

    ImGui::Render();

    ReleaseSRWLockExclusive(&g_imguiLock);

    if (FAILED(ctx.commandAllocator->Reset())) {
        Log("Overlay: D3D12 commandAllocator->Reset failed for buffer %u", backBufferIdx);
        return;
    }
    if (FAILED(ctx.commandList->Reset(ctx.commandAllocator, nullptr))) {
        Log("Overlay: D3D12 commandList->Reset failed for buffer %u", backBufferIdx);
        return;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = ctx.renderTarget;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    ctx.commandList->ResourceBarrier(1, &barrier);

    ctx.commandList->OMSetRenderTargets(1, &ctx.rtvHandle, FALSE, nullptr);
    ID3D12DescriptorHeap* heaps[] = { g_srvDescHeap12 };
    ctx.commandList->SetDescriptorHeaps(1, heaps);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), ctx.commandList);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    ctx.commandList->ResourceBarrier(1, &barrier);

    if (FAILED(ctx.commandList->Close())) {
        Log("Overlay: D3D12 commandList->Close failed for buffer %u", backBufferIdx);
        return;
    }

    ID3D12CommandList* cmdLists[] = { ctx.commandList };
    o_ExecuteCommandLists(g_cmdQueue12, 1, cmdLists);

    // 2. Signal fence monotonically for this frame submission
    if (g_fence12 && g_cmdQueue12) {
        g_fenceLastSignaled12++;
        g_cmdQueue12->Signal(g_fence12, g_fenceLastSignaled12);
        ctx.fenceValue = g_fenceLastSignaled12;
    }
}

// ---------------- D3D11 Fallback Implementation ----------------
static void CreateRTV11(IDXGISwapChain* sc) {
    if (!g_dev11 || !sc) return;
    ID3D11Texture2D* bb = nullptr;
    if (SUCCEEDED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) && bb) {
        if (FAILED(g_dev11->CreateRenderTargetView(bb, nullptr, &g_rtv11))) g_rtv11 = nullptr;
        bb->Release();
    }
}

static bool InitImGuiD3D11(IDXGISwapChain* sc) {
    g_dev11->GetImmediateContext(&g_ctx11);
    g_sc11 = sc;
    CreateRTV11(sc);

    DXGI_SWAP_CHAIN_DESC desc;
    sc->GetDesc(&desc);
    SubclassWindow(desc.OutputWindow);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    LoadAllFonts();
    ApplyCustomTheme();

    if (!ImGui_ImplDX11_Init(g_dev11, g_ctx11)) {
        Log("Overlay: ImGui_ImplDX11_Init failed!");
        return false;
    }

    QueryPerformanceFrequency(&g_qpf);
    QueryPerformanceCounter(&g_lastQpc);

    Log("Overlay: DirectX 11 Hook initialized successfully on HWND %p", g_hwnd);
    return true;
}

static void RenderOverlayD3D11(IDXGISwapChain* sc) {
    if (!g_inited) {
        if (InitImGuiD3D11(sc)) {
            g_inited = true;
        } else {
            return;
        }
    }
    if (sc != g_sc11) {
        if (g_rtv11) { g_rtv11->Release(); g_rtv11 = nullptr; }
        g_sc11 = sc;
        DXGI_SWAP_CHAIN_DESC desc;
        sc->GetDesc(&desc);
        if (desc.OutputWindow && desc.OutputWindow != g_hwnd) {
            SubclassWindow(desc.OutputWindow);
        }
        CreateRTV11(sc);
    }
    if (!g_rtv11) CreateRTV11(sc);
    if (!g_rtv11) return;

    // Hotkeys are handled cleanly in hk_WndProc.
    if (!g_menuVisible && !g_mapVisible && !g_staminaVisible) {
        return;
    }

    DXGI_SWAP_CHAIN_DESC d;
    sc->GetDesc(&d);
    ImGuiIO& io = ImGui::GetIO();
    RECT rect;
    if (g_hwnd && GetClientRect(g_hwnd, &rect) && (rect.right - rect.left > 0)) {
        io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
    } else {
        io.DisplaySize = ImVec2((float)d.BufferDesc.Width, (float)d.BufferDesc.Height);
    }

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    float dt = (float)(now.QuadPart - g_lastQpc.QuadPart) / (float)g_qpf.QuadPart;
    io.DeltaTime = dt > 0 ? dt : 1.0f / 60.0f;
    g_lastQpc = now;

    // Synchronize mouse position and state
    if (g_menuVisible && g_hwnd) {
        POINT pt;
        if (GetCursorPos(&pt) && ScreenToClient(g_hwnd, &pt)) {
            io.AddMousePosEvent((float)pt.x, (float)pt.y);
        }
        io.AddMouseButtonEvent(0, (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
        io.AddMouseButtonEvent(1, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
    }


    io.MouseDrawCursor = false;

    AcquireSRWLockExclusive(&g_imguiLock);

    ImGui_ImplDX11_NewFrame();
    ImGui::NewFrame();

    if (g_menuVisible) {
        RenderModMenu();
    }
    if (g_mapVisible && g_config.enableMinimap) {
        RenderMinimap(ImGui::GetForegroundDrawList(), io.DisplaySize.x);
    }
    if (g_staminaVisible && g_config.enableStaminaBar) {
        RenderStaminaBar(ImGui::GetForegroundDrawList(), 0);
        RenderStaminaBar(ImGui::GetForegroundDrawList(), 1);
    }
    if (g_monitorHudVisible) {
        RenderKingdomMonitorHud();
    }

    // In-Game Style Medieval Cursor
    if (g_menuVisible) {
        DrawMedievalGameCursor(ImGui::GetForegroundDrawList(), io.MousePos);
    }

    ImGui::Render();

    ReleaseSRWLockExclusive(&g_imguiLock);

    ID3D11RenderTargetView* oldRtv = nullptr;
    ID3D11DepthStencilView* oldDsv = nullptr;
    g_ctx11->OMGetRenderTargets(1, &oldRtv, &oldDsv);
    g_ctx11->OMSetRenderTargets(1, &g_rtv11, nullptr);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_ctx11->OMSetRenderTargets(1, &oldRtv, oldDsv);
    if (oldRtv) oldRtv->Release();
    if (oldDsv) oldDsv->Release();
}

// ---------------- Present & ResizeBuffers Hooks ----------------
static HRESULT STDMETHODCALLTYPE hk_Present(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!g_apiDetected && sc) {

        ID3D12Device* dev12 = nullptr;
        if (SUCCEEDED(sc->GetDevice(kIID_ID3D12Device, (void**)&dev12)) && dev12) {
            g_isD3D12 = true;
            g_dev12 = dev12;
            g_apiDetected = true;
            Log("Overlay: Verified Direct3D 12 device: %p", dev12);
        } else {
            ID3D11Device* dev11 = nullptr;
            if (SUCCEEDED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&dev11)) && dev11) {
                g_isD3D12 = false;
                g_dev11 = dev11;
                g_apiDetected = true;
                Log("Overlay: Verified Direct3D 11 device: %p", dev11);
            }
        }
    }

    if (g_isD3D12) {
        RenderOverlayD3D12(sc);
    } else if (g_apiDetected) {
        RenderOverlayD3D11(sc);
    }

    HRESULT hr = o_Present(sc, sync, flags);
    if (FAILED(hr)) {
        static DWORD s_lastLog = 0;
        DWORD now = GetTickCount();
        if (now - s_lastLog > 2000) {
            Log("Overlay: o_Present returned FAILED 0x%08lx", hr);
            if (g_dev12) {
                Log("Overlay: D3D12 GetDeviceRemovedReason = 0x%08lx", g_dev12->GetDeviceRemovedReason());
            }
            s_lastLog = now;
        }
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE hk_ResizeBuffers(IDXGISwapChain* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f, UINT fl) {
    if (g_isD3D12) {
        CleanupRenderTargetD3D12();
        HRESULT hr = o_ResizeBuffers(sc, n, w, h, f, fl);
        CreateRenderTargetD3D12(sc);
        return hr;
    } else {
        if (g_rtv11 && sc == g_sc11) {
            g_rtv11->Release();
            g_rtv11 = nullptr;
        }
        return o_ResizeBuffers(sc, n, w, h, f, fl);
    }
}

// ---------------- Hook Setup Thread ----------------
typedef HRESULT (WINAPI *t_D3D12CreateDevice)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);
typedef HRESULT (WINAPI *t_CreateDXGIFactory)(REFIID, void**);
typedef HRESULT (WINAPI *t_D3D11CreateDeviceAndSwapChain)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
    const D3D_FEATURE_LEVEL*, UINT, UINT, const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**,
    D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

static DWORD WINAPI OverlayThread(LPVOID) {
    // Install Win32 cursor un-trap hooks
    HMODULE u32 = GetModuleHandleA("user32.dll");
    if (!u32) u32 = LoadLibraryA("user32.dll");
    if (u32) {
        void* pSetCursorPos = (void*)GetProcAddress(u32, "SetCursorPos");
        void* pClipCursor   = (void*)GetProcAddress(u32, "ClipCursor");
        if (pSetCursorPos) {
            MH_CreateHook(pSetCursorPos, (void*)hk_SetCursorPos, (void**)&o_SetCursorPos);
            MH_EnableHook(pSetCursorPos);
        }
        if (pClipCursor) {
            MH_CreateHook(pClipCursor, (void*)hk_ClipCursor, (void**)&o_ClipCursor);
            MH_EnableHook(pClipCursor);
        }
        Log("Overlay: Win32 Cursor lock bypass hooks installed");
    }

    HMODULE d3d12 = LoadLibraryA("d3d12.dll");
    HMODULE dxgi  = LoadLibraryA("dxgi.dll");
    bool d3d12Hooked = false;

    if (d3d12 && dxgi) {
        auto pD3D12CreateDevice = (t_D3D12CreateDevice)GetProcAddress(d3d12, "D3D12CreateDevice");
        auto pCreateDXGIFactory = (t_CreateDXGIFactory)GetProcAddress(dxgi, "CreateDXGIFactory");

        if (pD3D12CreateDevice && pCreateDXGIFactory) {
            ID3D12Device* dummyDevice = nullptr;
            if (SUCCEEDED(pD3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, kIID_ID3D12Device, (void**)&dummyDevice)) && dummyDevice) {
                D3D12_COMMAND_QUEUE_DESC qdesc{};
                qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                ID3D12CommandQueue* dummyQueue = nullptr;
                if (SUCCEEDED(dummyDevice->CreateCommandQueue(&qdesc, kIID_ID3D12CommandQueue, (void**)&dummyQueue)) && dummyQueue) {
                    void** queueVtbl = *(void***)dummyQueue;
                    void* pExecute = queueVtbl[10];

                    WNDCLASSEXA wc{};
                    wc.cbSize = sizeof(wc);
                    wc.lpfnWndProc = DefWindowProcA;
                    wc.hInstance = GetModuleHandleA(nullptr);
                    wc.lpszClassName = "KE_DummyDX12";
                    RegisterClassExA(&wc);
                    HWND hwnd = CreateWindowA(wc.lpszClassName, "", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);

                    IDXGIFactory* factory = nullptr;
                    if (SUCCEEDED(pCreateDXGIFactory(kIID_IDXGIFactory, (void**)&factory)) && factory) {
                        DXGI_SWAP_CHAIN_DESC sd{};
                        sd.BufferCount = 2;
                        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        sd.BufferDesc.Width = 64;
                        sd.BufferDesc.Height = 64;
                        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                        sd.OutputWindow = hwnd;
                        sd.SampleDesc.Count = 1;
                        sd.Windowed = TRUE;
                        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

                        IDXGISwapChain* dummySC = nullptr;
                        if (SUCCEEDED(factory->CreateSwapChain(dummyQueue, &sd, &dummySC)) && dummySC) {
                            void** scVtbl = *(void***)dummySC;
                            void* pPresent = scVtbl[8];
                            void* pResize  = scVtbl[13];

                            dummySC->Release();
                            dummyQueue->Release();
                            dummyDevice->Release();
                            factory->Release();
                            DestroyWindow(hwnd);
                            UnregisterClassA(wc.lpszClassName, wc.hInstance);

                            bool okE = (MH_CreateHook(pExecute, (void*)hk_ExecuteCommandLists, (void**)&o_ExecuteCommandLists) == MH_OK && MH_EnableHook(pExecute) == MH_OK);
                            bool okP = (MH_CreateHook(pPresent, (void*)hk_Present, (void**)&o_Present) == MH_OK && MH_EnableHook(pPresent) == MH_OK);
                            bool okR = (MH_CreateHook(pResize, (void*)hk_ResizeBuffers, (void**)&o_ResizeBuffers) == MH_OK && MH_EnableHook(pResize) == MH_OK);
                            Log("Overlay: D3D12 Hooks Installed: ExecuteCmds=%s, Present=%s, ResizeBuffers=%s",
                                okE ? "OK" : "FAILED", okP ? "OK" : "FAILED", okR ? "OK" : "FAILED");
                            d3d12Hooked = okP && okE;
                        } else {
                            DestroyWindow(hwnd);
                            UnregisterClassA(wc.lpszClassName, wc.hInstance);
                            factory->Release();
                            dummyQueue->Release();
                            dummyDevice->Release();
                        }
                    } else {
                        DestroyWindow(hwnd);
                        UnregisterClassA(wc.lpszClassName, wc.hInstance);
                        dummyQueue->Release();
                        dummyDevice->Release();
                    }
                } else {
                    dummyDevice->Release();
                }
            }
        }
    }

    if (!d3d12Hooked) {
        Log("Overlay: Falling back to D3D11 dummy setup...");
        HMODULE d3d11 = LoadLibraryA("d3d11.dll");
        auto create = d3d11 ? (t_D3D11CreateDeviceAndSwapChain)GetProcAddress(d3d11, "D3D11CreateDeviceAndSwapChain") : nullptr;
        if (!create) {
            Log("Overlay: Neither D3D12 nor D3D11 available");
            return 0;
        }

        WNDCLASSEXA wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = DefWindowProcA;
        wc.hInstance = GetModuleHandleA(nullptr);
        wc.lpszClassName = "KE_DummyDX11";
        RegisterClassExA(&wc);
        HWND hwnd = CreateWindowA(wc.lpszClassName, "", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);

        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.Width = 64;
        sd.BufferDesc.Height = 64;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        IDXGISwapChain* sc = nullptr;
        ID3D11Device* dev = nullptr;
        ID3D11DeviceContext* ctx = nullptr;
        HRESULT hr = create(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, nullptr, &ctx);
        if (FAILED(hr)) {
            hr = create(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, nullptr, &ctx);
        }
        if (FAILED(hr) || !sc) {
            Log("Overlay: D3D11 dummy swap chain creation failed 0x%08lx", hr);
            DestroyWindow(hwnd);
            UnregisterClassA(wc.lpszClassName, wc.hInstance);
            return 0;
        }

        void** vt = *(void***)sc;
        void* pPresent = vt[8];
        void* pResize  = vt[13];
        sc->Release();
        dev->Release();
        ctx->Release();
        DestroyWindow(hwnd);
        UnregisterClassA(wc.lpszClassName, wc.hInstance);

        bool ok1 = (MH_CreateHook(pPresent, (void*)hk_Present, (void**)&o_Present) == MH_OK && MH_EnableHook(pPresent) == MH_OK);
        bool ok2 = (MH_CreateHook(pResize, (void*)hk_ResizeBuffers, (void**)&o_ResizeBuffers) == MH_OK && MH_EnableHook(pResize) == MH_OK);
        Log("Overlay: D3D11 Hook Present=%s, ResizeBuffers=%s", ok1 ? "OK" : "FAILED", ok2 ? "OK" : "FAILED");
    }

    return 0;
}

void StartDX11Hook() {
    CreateThread(nullptr, 0, OverlayThread, nullptr, 0, nullptr);
}
