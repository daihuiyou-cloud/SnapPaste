#pragma once

#include "domain/capture/IScreenRegionDetector.h"

#include <QRect>
#include <QVector>

#include <mutex>
#include <thread>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace snappaste {

namespace detail {

class ComInitializer {
public:
    ComInitializer();
    ~ComInitializer();
    ComInitializer(const ComInitializer&) = delete;
    ComInitializer& operator=(const ComInitializer&) = delete;
private:
    bool initialized_ = false;
};

} // namespace detail

class WinScreenRegionDetector final : public IScreenRegionDetector {
public:
    WinScreenRegionDetector();
    ~WinScreenRegionDetector() override;

    QVector<QRect> regionsAt(const QPoint& globalPosition, const QRect& desktopBounds) override;

private:
#ifdef Q_OS_WIN
    void rebuildCache(HWND hwnd, const QRect& desktopBounds);
    void launchUiScan(HWND hwnd);

    HWND cachedHwnd_ = nullptr;
    QRect cachedBounds_;
    QVector<QRect> cachedChildRects_;
    QVector<QRect> cachedUiRects_;
    std::thread uiWorker_;
    std::mutex uiCacheMutex_;
#endif
    detail::ComInitializer comInit_;
};

} // namespace snappaste
