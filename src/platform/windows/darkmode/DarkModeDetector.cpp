#include "platform/windows/darkmode/DarkModeDetector.h"

#include <QSettings>

namespace nanosnap {

bool DarkModeDetector::isSystemDarkMode() const
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                       QSettings::NativeFormat);
    return settings.value("AppsUseLightTheme", 1).toInt() == 0;
#else
    return false;
#endif
}

} // namespace nanosnap
