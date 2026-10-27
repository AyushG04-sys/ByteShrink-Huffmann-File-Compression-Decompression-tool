#include "Theme.h"
#include <cstring>

void Theme::applyTheme(ThemeMode mode) {
    if (mode == ThemeMode::Dark) {
        applyDarkTheme();
    } else {
        applyLightTheme();
    }
}

void Theme::applyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Base geometry
    style.WindowRounding    = 10.0f;
    style.ChildRounding     = 8.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 8.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;

    style.WindowPadding     = ImVec2(16.0f, 16.0f);
    style.FramePadding      = ImVec2(12.0f, 8.0f);
    style.ItemSpacing       = ImVec2(12.0f, 10.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 14.0f;
    style.GrabMinSize       = 12.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.TabBorderSize     = 0.0f;

    // Color palette: Modern Deep Slate Dark Theme
    colors[ImGuiCol_Text]                  = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.54f, 0.62f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.08f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.12f, 0.13f, 0.18f, 0.95f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.10f, 0.11f, 0.16f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.23f, 0.32f, 0.80f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_FrameBg]               = ImVec4(0.15f, 0.17f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.20f, 0.23f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.25f, 0.29f, 0.40f, 1.00f);

    colors[ImGuiCol_TitleBg]               = ImVec4(0.08f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.10f, 0.12f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.06f, 0.07f, 0.10f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.11f, 0.15f, 1.00f);

    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.08f, 0.09f, 0.12f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.22f, 0.26f, 0.36f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.28f, 0.34f, 0.48f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.35f, 0.42f, 0.60f, 1.00f);

    colors[ImGuiCol_CheckMark]             = ImVec4(0.24f, 0.68f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.24f, 0.68f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.40f, 0.78f, 1.00f, 1.00f);

    // Primary Vibrant Button
    colors[ImGuiCol_Button]                = ImVec4(0.18f, 0.42f, 0.88f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.52f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.14f, 0.35f, 0.76f, 1.00f);

    colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.22f, 0.32f, 0.80f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.24f, 0.30f, 0.44f, 0.90f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.28f, 0.36f, 0.54f, 1.00f);

    colors[ImGuiCol_Separator]             = ImVec4(0.20f, 0.23f, 0.32f, 0.80f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.24f, 0.68f, 0.98f, 0.78f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.24f, 0.68f, 0.98f, 1.00f);

    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.20f, 0.23f, 0.32f, 0.40f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.24f, 0.68f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.24f, 0.68f, 0.98f, 0.95f);

    colors[ImGuiCol_Tab]                   = ImVec4(0.12f, 0.14f, 0.20f, 0.90f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.20f, 0.35f, 0.70f, 0.90f);
    colors[ImGuiCol_TabSelected]           = ImVec4(0.18f, 0.42f, 0.88f, 1.00f);
    colors[ImGuiCol_TabDimmed]             = ImVec4(0.10f, 0.11f, 0.16f, 0.80f);
    colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.15f, 0.22f, 0.38f, 1.00f);

    colors[ImGuiCol_PlotLines]             = ImVec4(0.24f, 0.68f, 0.98f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.18f, 0.78f, 0.62f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.28f, 0.90f, 0.72f, 1.00f);

    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.14f, 0.17f, 0.24f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.22f, 0.26f, 0.36f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.18f, 0.21f, 0.30f, 0.60f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);

    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.18f, 0.42f, 0.88f, 0.35f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.24f, 0.68f, 0.98f, 1.00f);
}

void Theme::applyLightTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding    = 10.0f;
    style.ChildRounding     = 8.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 8.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;

    style.WindowPadding     = ImVec2(16.0f, 16.0f);
    style.FramePadding      = ImVec2(12.0f, 8.0f);
    style.ItemSpacing       = ImVec2(12.0f, 10.0f);

    colors[ImGuiCol_Text]                  = ImVec4(0.12f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.55f, 0.58f, 0.64f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.96f, 0.97f, 0.99f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.85f, 0.88f, 0.92f, 1.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_FrameBg]               = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.88f, 0.91f, 0.95f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.82f, 0.86f, 0.92f, 1.00f);

    colors[ImGuiCol_TitleBg]               = ImVec4(0.94f, 0.95f, 0.98f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.90f, 0.92f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.94f, 0.95f, 0.98f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.94f, 0.95f, 0.98f, 1.00f);

    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.94f, 0.95f, 0.98f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.75f, 0.78f, 0.84f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.65f, 0.69f, 0.76f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.55f, 0.60f, 0.68f, 1.00f);

    colors[ImGuiCol_CheckMark]             = ImVec4(0.12f, 0.45f, 0.92f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.12f, 0.45f, 0.92f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.08f, 0.38f, 0.82f, 1.00f);

    colors[ImGuiCol_Button]                = ImVec4(0.12f, 0.45f, 0.92f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.18f, 0.52f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.08f, 0.38f, 0.82f, 1.00f);

    colors[ImGuiCol_Header]                = ImVec4(0.88f, 0.92f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.82f, 0.88f, 0.96f, 0.90f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.75f, 0.84f, 0.94f, 1.00f);

    colors[ImGuiCol_Separator]             = ImVec4(0.85f, 0.88f, 0.92f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.88f, 0.91f, 0.95f, 0.90f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.78f, 0.85f, 0.95f, 0.90f);
    colors[ImGuiCol_TabSelected]           = ImVec4(0.12f, 0.45f, 0.92f, 1.00f);
    colors[ImGuiCol_TabDimmed]             = ImVec4(0.92f, 0.94f, 0.97f, 0.80f);
    colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.85f, 0.90f, 0.98f, 1.00f);

    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.90f, 0.93f, 0.97f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.80f, 0.84f, 0.90f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.88f, 0.91f, 0.95f, 0.60f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.00f, 0.00f, 0.00f, 0.02f);
}

ImVec4 Theme::getPrimaryColor() { return ImVec4(0.24f, 0.62f, 0.98f, 1.0f); }
ImVec4 Theme::getSuccessColor() { return ImVec4(0.18f, 0.78f, 0.48f, 1.0f); }
ImVec4 Theme::getWarningColor() { return ImVec4(0.98f, 0.68f, 0.22f, 1.0f); }
ImVec4 Theme::getDangerColor()  { return ImVec4(0.92f, 0.30f, 0.30f, 1.0f); }
ImVec4 Theme::getCardBgColor()  { return ImVec4(0.14f, 0.16f, 0.22f, 0.95f); }

ImVec4 Theme::getCategoryBadgeColor(const char* category) {
    if (!category) return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    if (std::strstr(category, "Document")) return ImVec4(0.20f, 0.55f, 0.95f, 1.0f);
    if (std::strstr(category, "Image"))    return ImVec4(0.18f, 0.78f, 0.48f, 1.0f);
    if (std::strstr(category, "Video"))    return ImVec4(0.68f, 0.38f, 0.95f, 1.0f);
    if (std::strstr(category, "Audio"))    return ImVec4(0.98f, 0.58f, 0.20f, 1.0f);
    if (std::strstr(category, "Archive"))  return ImVec4(0.15f, 0.85f, 0.85f, 1.0f);
    return ImVec4(0.60f, 0.65f, 0.72f, 1.0f);
}
