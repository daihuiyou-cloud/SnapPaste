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
        return Result<QImage>::failure("Capture region is invalid.");
    }

    HDC screenDc = GetDC(nullptr);
    if (screenDc == nullptr) {
        return Result<QImage>::failure("Failed to access the screen device context.");
    }

    HDC memoryDc = CreateCompatibleDC(screenDc);
    if (memoryDc == nullptr) {
        ReleaseDC(nullptr, screenDc);
        return Result<QImage>::failure("Failed to create a capture device context.");
    }

    HBITMAP bitmap = CreateCompatibleBitmap(screenDc, region.width(), region.height());
    if (bitmap == nullptr) {
        DeleteDC(memoryDc);
        ReleaseDC(nullptr, screenDc);
        return Result<QImage>::failure("Failed to allocate a capture bitmap.");
    }

    auto* oldBitmap = SelectObject(memoryDc, bitmap);
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
        return Result<QImage>::failure("Failed to copy the selected screen region.");
    }

    QImage image(region.width(), region.height(), QImage::Format_RGB32);
    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = region.width();
    bitmapInfo.bmiHeader.biHeight = -region.height();
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    const auto lines = GetDIBits(screenDc,
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
        return Result<QImage>::failure("Failed to read the captured screen pixels.");
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
    return captureRectWithGdi(virtualDesktop);
#else
    auto* screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) {
        return Result<QImage>::failure("No primary screen is available.");
    }

    const auto pixmap = screen->grabWindow(0);
    if (pixmap.isNull()) {
        return Result<QImage>::failure("Failed to capture primary screen.");
    }

    return Result<QImage>::success(pixmap.toImage());
#endif
}

Result<QImage> GdiScreenCaptureService::captureRegion(const QRect& region)
{
#if defined(Q_OS_WIN)
    auto* screen = QGuiApplication::screenAt(region.center());
    const auto dpr = screen ? screen->devicePixelRatio() : 1.0;
    QRect physicalRegion(
        qRound(region.x() * dpr),
        qRound(region.y() * dpr),
        qRound(region.width() * dpr),
        qRound(region.height() * dpr)
    );
    auto result = captureRectWithGdi(physicalRegion);
    if (result.isOk() && dpr > 1.0) {
        auto scaled = result.value().scaled(region.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        return Result<QImage>::success(std::move(scaled));
    }
    return result;
#else
    auto* screen = QGuiApplication::screenAt(region.center());
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        return Result<QImage>::failure("No screen is available for capture.");
    }

    const auto pixmap = screen->grabWindow(0, region.x(), region.y(), region.width(), region.height());
    if (pixmap.isNull()) {
        return Result<QImage>::failure("Failed to capture selected region.");
    }

    return Result<QImage>::success(pixmap.toImage());
#endif
}

} // namespace snappaste
