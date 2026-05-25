#include "platform/windows/dpi/HighDpiManager.h"

#include <QCoreApplication>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace snappaste {

void HighDpiManager::configureBeforeApplication()
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
