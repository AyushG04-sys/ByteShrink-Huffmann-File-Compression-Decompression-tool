#ifndef THEME_H
#define THEME_H

#include "../third_party/imgui/imgui.h"

enum class ThemeMode {
    Dark,
    Light
};

class Theme {
public:
    static void applyTheme(ThemeMode mode);
    static void applyDarkTheme();
    static void applyLightTheme();

    // Color definitions for UI badges and charts
    static ImVec4 getPrimaryColor();
    static ImVec4 getSuccessColor();
    static ImVec4 getWarningColor();
    static ImVec4 getDangerColor();
    static ImVec4 getCardBgColor();
    static ImVec4 getCategoryBadgeColor(const char* category);
};

#endif // THEME_H
