#pragma once

#include "shared/types/Hotkey.h"

#include <QString>

namespace nanosnap {

enum class ThemeMode {
    System,
    Light,
    Dark
};

struct AppSettings final {
    QString saveDirectory;
    QString imageFormat = "png";
    ThemeMode themeMode = ThemeMode::System;
    Hotkey captureHotkey;
    Hotkey pasteHotkey;
    Hotkey hidePinsHotkey;
};

} // namespace nanosnap
