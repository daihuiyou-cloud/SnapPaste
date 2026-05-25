#pragma once

#include "domain/capture/IScreenRegionDetector.h"

#include <QRect>
#include <QVector>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace nanosnap {

class WinScreenRegionDetector final : public IScreenRegionDetector {
public:
    WinScreenRegionDetector();
    ~WinScreenRegionDetector() override;

    QVector<QRect> regionsAt(const QPoint& globalPosition, const QRect& desktopBounds) override;

private:
#ifdef Q_OS_WIN
    void rebuildCache(HWND hwnd, const QRect& desktopBounds);

    HWND cachedHwnd_ = nullptr;
    QRect cachedBounds_;
    QVector<QRect> cachedChildRects_;
    QVector<QRect> cachedUiRects_;
#endif
};

} // namespace nanosnap
