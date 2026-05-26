#include "platform/windows/window/WindowInteractionService.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace snappaste {

void WindowInteractionService::setClickThrough(QWidget* widget, bool enabled) const
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

} // namespace snappaste
