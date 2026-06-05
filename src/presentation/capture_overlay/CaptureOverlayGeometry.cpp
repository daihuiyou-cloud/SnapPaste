#include "presentation/capture_overlay/CaptureOverlayGeometry.h"

#include <QGuiApplication>
#include <QScreen>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace snappaste {

QRect normalizedWithMinimum(QRect rect)
{
    rect = rect.normalized();
    if (rect.width() < kMinSelectionSize) {
        rect.setWidth(kMinSelectionSize);
    }
    if (rect.height() < kMinSelectionSize) {
        rect.setHeight(kMinSelectionSize);
    }
    return rect;
}

QRect clampedTo(QRect rect, const QRect& bounds)
{
    if (rect.width() > bounds.width()) {
        rect.setWidth(bounds.width());
    }
    if (rect.height() > bounds.height()) {
        rect.setHeight(bounds.height());
    }
    if (rect.left() < bounds.left()) {
        rect.moveLeft(bounds.left());
    }
    if (rect.top() < bounds.top()) {
        rect.moveTop(bounds.top());
    }
    if (rect.right() > bounds.right()) {
        rect.moveRight(bounds.right());
    }
    if (rect.bottom() > bounds.bottom()) {
        rect.moveBottom(bounds.bottom());
    }
    return rect;
}

QRect insetIfFullScreen(QRect rect, const QRect& desktopBounds)
{
    rect = rect.normalized();
    if (!rect.isValid()) {
        return rect;
    }

    auto inset = [](const QRect& region) {
        if (region.width() <= kFullScreenSelectionInset * 2
            || region.height() <= kFullScreenSelectionInset * 2) {
            return region;
        }
        return region.adjusted(kFullScreenSelectionInset,
                               kFullScreenSelectionInset,
                               -kFullScreenSelectionInset,
                               -kFullScreenSelectionInset);
    };

    if (rect.contains(desktopBounds.adjusted(kFullScreenSelectionInset, kFullScreenSelectionInset, -kFullScreenSelectionInset, -kFullScreenSelectionInset))) {
        return inset(rect);
    }

    return rect;
}

QRect selectableRegion(QRect rect, const QRect& bounds)
{
    return insetIfFullScreen(clampedTo(rect, bounds), bounds);
}

void applyNativeDesktopBounds(QWidget& widget)
{
#ifdef Q_OS_WIN
    const auto hwnd = reinterpret_cast<HWND>(widget.winId());
    if (hwnd == nullptr) {
        return;
    }
    static const int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    static const int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    static const int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    static const int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    SetWindowPos(hwnd,
                 HWND_TOPMOST,
                 vx, vy, vw, vh,
                 SWP_SHOWWINDOW);
#else
    Q_UNUSED(widget)
#endif
}

}
