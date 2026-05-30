#include "platform/windows/WindowsPlatformService.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>
#include <QWidget>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace snappaste {

bool WindowsPlatformService::isSystemDarkMode() const
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                       QSettings::NativeFormat);
    return settings.value("AppsUseLightTheme", 1).toInt() == 0;
#else
    return false;
#endif
}

void WindowsPlatformService::setClickThrough(QWidget* widget, bool enabled) const
{
    if (widget == nullptr || !widget->winId()) {
        return;
    }

#ifdef Q_OS_WIN
    const auto hwnd = reinterpret_cast<HWND>(widget->winId());
    auto style = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (enabled) {
        style |= WS_EX_TRANSPARENT | WS_EX_LAYERED;
    } else {
        style &= ~(WS_EX_TRANSPARENT | WS_EX_LAYERED);
    }
    SetWindowLong(hwnd, GWL_EXSTYLE, style);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
#else
    Q_UNUSED(enabled)
#endif
}

void WindowsPlatformService::revealInExplorer(const QString& filePath) const
{
    const QFileInfo info(filePath);
    if (info.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
    }
}

void WindowsPlatformService::configureHighDpi()
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

#ifdef Q_OS_WIN
    using SetDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    auto* user32 = GetModuleHandleW(L"user32.dll");
    if (user32 != nullptr) {
        auto* setContext = reinterpret_cast<SetDpiAwarenessContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setContext != nullptr) {
            setContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }
#endif
}

} // namespace snappaste