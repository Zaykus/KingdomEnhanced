#include "imgui.h"

void ApplyCustomTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding    = 4.0f;
    style.FrameRounding     = 3.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 3.0f;

    style.WindowBorderSize  = 1.5f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;

    style.WindowPadding     = ImVec2(10.0f, 10.0f);
    style.FramePadding      = ImVec2(8.0f, 5.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 6.0f);

    // Exact Medieval Kingdom Palette (Gold, Amber, Warm Parchment & Dark Wood from old/ModMenu.cs)
    const ImVec4 C_BG          = ImVec4(0.063f, 0.051f, 0.035f, 0.98f); // Deep wood / charcoal background
    const ImVec4 C_SIDEBAR     = ImVec4(0.086f, 0.071f, 0.047f, 1.00f); // Dark timber panel
    const ImVec4 C_CARD        = ImVec4(0.125f, 0.102f, 0.067f, 1.00f); // Wood card surface
    const ImVec4 C_BORDER      = ImVec4(0.550f, 0.420f, 0.180f, 0.85f); // Rich Bronze/Gold border (#8C6B2E)
    const ImVec4 C_GOLD        = ImVec4(0.940f, 0.750f, 0.150f, 1.00f); // Pure Kingdom Gold (#F0BF26)
    const ImVec4 C_GOLD_DIM    = ImVec4(0.700f, 0.550f, 0.200f, 1.00f); // Dimmed Gold
    const ImVec4 C_AMBER       = ImVec4(1.000f, 0.600f, 0.100f, 1.00f); // Hot Amber Accent (#FF991A)
    const ImVec4 C_TEXT        = ImVec4(0.940f, 0.900f, 0.780f, 1.00f); // Warm Parchment White
    const ImVec4 C_TEXT_DIM    = ImVec4(0.600f, 0.550f, 0.450f, 1.00f); // Muted Parchment (#998C73)
    const ImVec4 C_BTN         = ImVec4(0.160f, 0.129f, 0.086f, 1.00f); // Dark Wood Button
    const ImVec4 C_BTN_HOT     = ImVec4(0.240f, 0.192f, 0.118f, 1.00f); // Hovered Button
    const ImVec4 C_BTN_ACTIVE  = ImVec4(0.330f, 0.255f, 0.145f, 1.00f); // Active Button

    colors[ImGuiCol_Text]                  = C_TEXT;
    colors[ImGuiCol_TextDisabled]          = C_TEXT_DIM;
    colors[ImGuiCol_WindowBg]              = C_BG;
    colors[ImGuiCol_ChildBg]               = C_CARD;
    colors[ImGuiCol_PopupBg]               = ImVec4(0.090f, 0.075f, 0.051f, 0.98f);
    colors[ImGuiCol_Border]                = C_BORDER;
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.100f, 0.080f, 0.055f, 0.90f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.180f, 0.145f, 0.095f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.250f, 0.195f, 0.120f, 1.00f);
    colors[ImGuiCol_TitleBg]               = C_SIDEBAR;
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.130f, 0.105f, 0.070f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = C_SIDEBAR;
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.070f, 0.055f, 0.035f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.320f, 0.250f, 0.140f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = C_GOLD_DIM;
    colors[ImGuiCol_ScrollbarGrabActive]   = C_GOLD;
    colors[ImGuiCol_CheckMark]             = C_GOLD;
    colors[ImGuiCol_SliderGrab]            = C_GOLD;
    colors[ImGuiCol_SliderGrabActive]      = C_AMBER;
    colors[ImGuiCol_Button]                = C_BTN;
    colors[ImGuiCol_ButtonHovered]         = C_BTN_HOT;
    colors[ImGuiCol_ButtonActive]          = C_BTN_ACTIVE;
    colors[ImGuiCol_Header]                = ImVec4(0.200f, 0.155f, 0.095f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.280f, 0.220f, 0.130f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.360f, 0.280f, 0.160f, 1.00f);
    colors[ImGuiCol_Separator]             = C_BORDER;
    colors[ImGuiCol_Tab]                   = C_BTN;
    colors[ImGuiCol_TabHovered]            = C_BTN_HOT;
    colors[ImGuiCol_TabActive]             = ImVec4(0.240f, 0.185f, 0.110f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.100f, 0.080f, 0.055f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.160f, 0.130f, 0.090f, 1.00f);
}

