#include <QCoreApplication>
#include "app/AppStartup.h"

#include <QFile>

namespace snappaste {

Result<void> AppStartup::applyTheme(QApplication& application,
                                    const AppSettings& settings,
                                    const IPlatformService& platformService)
{
    QFile file(themePath(settings, platformService));
    if (!file.open(QIODevice::ReadOnly)) {
        return Result<void>::failure(QCoreApplication::translate("AppErrors", "Failed to load theme resource."));
    }

    application.setStyleSheet(QString::fromUtf8(file.readAll()));
    return Result<void>::success();
}

QString AppStartup::themePath(const AppSettings& settings, const IPlatformService& platformService)
{
    if (settings.themeMode == ThemeMode::Dark) {
        return ":/themes/dark.qss";
    }
    if (settings.themeMode == ThemeMode::Light) {
        return ":/themes/light.qss";
    }
    return platformService.isSystemDarkMode() ? ":/themes/dark.qss" : ":/themes/light.qss";
}

} // namespace snappaste
