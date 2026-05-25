#include "app/AppStartup.h"

#include "platform/windows/darkmode/DarkModeDetector.h"

#include <QFile>

namespace snappaste {

Result<void> AppStartup::applyTheme(QApplication& application,
                                    const AppSettings& settings,
                                    const DarkModeDetector& darkModeDetector)
{
    QFile file(themePath(settings, darkModeDetector));
    if (!file.open(QIODevice::ReadOnly)) {
        return Result<void>::failure("Failed to load theme resource.");
    }

    application.setStyleSheet(QString::fromUtf8(file.readAll()));
    return Result<void>::success();
}

QString AppStartup::themePath(const AppSettings& settings, const DarkModeDetector& darkModeDetector)
{
    if (settings.themeMode == ThemeMode::Dark) {
        return ":/themes/dark.qss";
    }
    if (settings.themeMode == ThemeMode::Light) {
        return ":/themes/light.qss";
    }
    return darkModeDetector.isSystemDarkMode() ? ":/themes/dark.qss" : ":/themes/light.qss";
}

} // namespace snappaste
