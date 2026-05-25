#pragma once

#include "shared/result/Result.h"
#include "shared/types/AppSettings.h"

#include <QApplication>

namespace snappaste {

class DarkModeDetector;

class AppStartup final {
public:
    static Result<void> applyTheme(QApplication& application,
                                   const AppSettings& settings,
                                   const DarkModeDetector& darkModeDetector);

private:
    static QString themePath(const AppSettings& settings, const DarkModeDetector& darkModeDetector);
};

} // namespace snappaste
