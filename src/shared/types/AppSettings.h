#pragma once

#include "shared/types/Hotkey.h"

#include <QString>

namespace snappaste {

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
    QString ocrLanguage;
    QString language;
    bool autoSaveOnCapture = false;
    Hotkey repeatCaptureHotkey;
};

} // namespace snappaste
