#include "platform/windows/capture/GdiScreenCaptureService.h"

#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace snappaste {

namespace {

#ifdef Q_OS_WIN
Result<QImage> captureRectWithGdi(const QRect& region)
{
    if (!region.isValid() || region.width() < 1 || region.height() < 1) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Capture region is invalid."));
    }

    HDC screenDc = GetDC(nullptr);
    if (screenDc == nullptr) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to access the screen device context."));
    }

    HDC memoryDc = CreateCompatibleDC(screenDc);
    if (memoryDc == nullptr) {
        ReleaseDC(nullptr, screenDc);
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to create a capture device context."));
    }

    HBITMAP bitmap = CreateCompatibleBitmap(screenDc, region.width(), region.height());
    if (bitmap == nullptr) {
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to allocate a capture bitmap."));
    }

    auto* oldBitmap = SelectObject(memoryDc, bitmap);
    if (oldBitmap == nullptr || oldBitmap == HGDI_ERROR) {
        DeleteObject(bitmap);
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to select bitmap into capture DC."));
    }
    const auto copied = BitBlt(memoryDc,
                               0,
                               0,
                               region.width(),
                               region.height(),
                               screenDc,
                               region.x(),
                               region.y(),
                               SRCCOPY | CAPTUREBLT) != FALSE;
    SelectObject(memoryDc, oldBitmap);

    if (!copied) {
        DeleteObject(bitmap);
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to copy the selected screen region."));
    }

    QImage image(region.width(), region.height(), QImage::Format_RGB32);
    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = region.width();
    bitmapInfo.bmiHeader.biHeight = -region.height();
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    const auto lines = GetDIBits(memoryDc,
                                 bitmap,
                                 0,
                                 static_cast<UINT>(region.height()),
                                 image.bits(),
                                 &bitmapInfo,
                                 DIB_RGB_COLORS);

    DeleteObject(bitmap);
    DeleteDC(memoryDc);
    ReleaseDC(nullptr, screenDc);

    if (lines == 0) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to read the captured screen pixels."));
    }

    return Result<QImage>::success(std::move(image));
}
#endif

} // namespace

Result<QImage> GdiScreenCaptureService::capturePrimaryScreen()
{
#ifdef Q_OS_WIN
    const QRect virtualDesktop(GetSystemMetrics(SM_XVIRTUALSCREEN),
                               GetSystemMetrics(SM_YVIRTUALSCREEN),
                               GetSystemMetrics(SM_CXVIRTUALSCREEN),
                               GetSystemMetrics(SM_CYVIRTUALSCREEN));
    auto result = captureRectWithGdi(virtualDesktop);
    if (result.isOk()) {
        auto* screen = QGuiApplication::primaryScreen();
        auto dpr = screen ? screen->devicePixelRatio() : 1.0;
        if (dpr > 1.0) {
            auto image = result.value();
            image.setDevicePixelRatio(dpr);
            return Result<QImage>::success(std::move(image));
        }
    }
    return result;
#else
    auto* screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "No primary screen is available."));
    }

    const auto pixmap = screen->grabWindow(0);
    if (pixmap.isNull()) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to capture primary screen."));
    }

    return Result<QImage>::success(pixmap.toImage());
#endif
}

Result<QImage> GdiScreenCaptureService::captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments)
{
    Q_UNUSED(segments)
    return captureRegion(region);
}

Result<QImage> GdiScreenCaptureService::captureRegion(const QRect& physicalRegion, qreal devicePixelRatio)
{
    auto result = captureRectWithGdi(physicalRegion);
    if (result.isOk() && devicePixelRatio > 1.0) {
        auto image = result.value();
        image.setDevicePixelRatio(devicePixelRatio);
        result = Result<QImage>::success(std::move(image));
    }
    return result;
}

Result<QImage> GdiScreenCaptureService::captureRegion(const QRect& region)
{
#if defined(Q_OS_WIN)
    QRect physicalRegion;
    qreal dpr = 1.0;

    auto* singleScreen = QGuiApplication::screenAt(region.center());
    if (singleScreen && singleScreen->geometry().contains(region)) {
        dpr = singleScreen->devicePixelRatio();
        physicalRegion = QRect(qRound(region.x() * dpr),
                               qRound(region.y() * dpr),
                               qRound(region.width() * dpr),
                               qRound(region.height() * dpr));
    } else {
        bool mixedDpr = false;
        for (auto* screen : QGuiApplication::screens()) {
            if (screen == nullptr) continue;
            const auto screenGeo = screen->geometry();
            const auto intersection = screenGeo.intersected(region);
            if (!intersection.isValid() || intersection.isEmpty()) continue;

            const auto screenDpr = screen->devicePixelRatio();
            if (dpr != 1.0 && std::abs(dpr - screenDpr) > 0.01) {
                mixedDpr = true;
            }
            dpr = qMax(dpr, screenDpr);

            QRect physSeg(qRound(intersection.x() * screenDpr),
                          qRound(intersection.y() * screenDpr),
                          qRound(intersection.width() * screenDpr),
                          qRound(intersection.height() * screenDpr));
            if (physicalRegion.isNull()) {
                physicalRegion = physSeg;
            } else {
                physicalRegion = physicalRegion.united(physSeg);
            }
        }
        Q_UNUSED(mixedDpr)
    }

    if (physicalRegion.isNull()) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Capture region does not intersect any screen."));
    }

    auto result = captureRectWithGdi(physicalRegion);
    if (result.isOk() && dpr > 1.0) {
        auto image = result.value();
        image.setDevicePixelRatio(dpr);
        return Result<QImage>::success(std::move(image));
    }
    return result;
#else
    auto* screen = QGuiApplication::screenAt(region.center());
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "No screen is available for capture."));
    }

    const auto pixmap = screen->grabWindow(0, region.x(), region.y(), region.width(), region.height());
    if (pixmap.isNull()) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to capture selected region."));
    }

    return Result<QImage>::success(pixmap.toImage());
#endif
}

} // namespace snappaste
