#include "platform/windows/capture/DxgiScreenCaptureService.h"

#include "shared/screen/ScreenSegmentUtil.h"

#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QVector>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>

#ifdef Q_OS_WIN
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <windows.h>
#endif

namespace snappaste {

namespace {

#ifdef Q_OS_WIN
using Microsoft::WRL::ComPtr;

struct OutputMatch {
    ComPtr<IDXGIOutput1> output;
    DXGI_OUTPUT_DESC desc{};
};

bool rectContains(const RECT& rect, const POINT& point)
{
    return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
}

QString deviceNameFrom(const DXGI_OUTPUT_DESC& desc)
{
    return QString::fromWCharArray(desc.DeviceName);
}

Result<OutputMatch> findOutput(const QString& qtScreenName, const POINT& physicalCenter)
{
    ComPtr<IDXGIFactory1> factory;
    auto hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(factory.GetAddressOf()));
    if (FAILED(hr)) {
        return Result<OutputMatch>::failure(QCoreApplication::translate("AppErrors", "Failed to create DXGI factory."));
    }

    OutputMatch fallback;
    bool hasFallback = false;

    for (UINT adapterIndex = 0;; ++adapterIndex) {
        ComPtr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(adapterIndex, adapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND) {
            break;
        }

        for (UINT outputIndex = 0;; ++outputIndex) {
            ComPtr<IDXGIOutput> output;
            if (adapter->EnumOutputs(outputIndex, output.GetAddressOf()) == DXGI_ERROR_NOT_FOUND) {
                break;
            }

            ComPtr<IDXGIOutput1> output1;
            if (FAILED(output.As(&output1))) {
                continue;
            }

            DXGI_OUTPUT_DESC desc{};
            if (FAILED(output->GetDesc(&desc))) {
                continue;
            }

            OutputMatch match;
            match.output = output1;
            match.desc = desc;

            if (!qtScreenName.isEmpty() && deviceNameFrom(desc).compare(qtScreenName, Qt::CaseInsensitive) == 0) {
                return Result<OutputMatch>::success(std::move(match));
            }

            if (!hasFallback && rectContains(desc.DesktopCoordinates, physicalCenter)) {
                fallback = std::move(match);
                hasFallback = true;
            }
        }
    }

    if (hasFallback) {
        return Result<OutputMatch>::success(std::move(fallback));
    }

    return Result<OutputMatch>::failure(QCoreApplication::translate("AppErrors", "No matching DXGI output was found."));
}

QRect toPhysicalRegion(const QRect& logicalRegion,
                       const QRect& logicalScreenGeometry,
                       qreal devicePixelRatio,
                       const RECT& outputRect)
{
    const auto scale = std::max<qreal>(devicePixelRatio, 1.0);
    const auto left = outputRect.left + static_cast<int>(std::floor((logicalRegion.left() - logicalScreenGeometry.left()) * scale));
    const auto top = outputRect.top + static_cast<int>(std::floor((logicalRegion.top() - logicalScreenGeometry.top()) * scale));
    const auto right = outputRect.left + static_cast<int>(std::ceil((logicalRegion.right() + 1 - logicalScreenGeometry.left()) * scale));
    const auto bottom = outputRect.top + static_cast<int>(std::ceil((logicalRegion.bottom() + 1 - logicalScreenGeometry.top()) * scale));
    return QRect(QPoint(left, top), QPoint(right - 1, bottom - 1)).normalized();
}

QImage mappedTextureToImage(const D3D11_MAPPED_SUBRESOURCE& mapped, int width, int height, DXGI_FORMAT format)
{
    const auto srcRowBytes = static_cast<size_t>(mapped.RowPitch);
    QImage image(width, height, QImage::Format_RGB32);

    switch (format) {
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        for (int y = 0; y < height; ++y) {
            const auto* src = static_cast<const uchar*>(mapped.pData) + (y * mapped.RowPitch);
            std::memcpy(image.scanLine(y), src, std::min(static_cast<size_t>(width) * 4, srcRowBytes));
            auto* pixels = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < width; ++x) {
                pixels[x] |= 0xff000000;
            }
        }
        break;

    case DXGI_FORMAT_R10G10B10A2_UNORM:
        for (int y = 0; y < height; ++y) {
            const auto* src = reinterpret_cast<const uint32_t*>(static_cast<const uchar*>(mapped.pData) + (y * mapped.RowPitch));
            auto* pixels = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < width; ++x) {
                uint32_t p = src[x];
                int r = (p >> 20) & 0x3ff;
                int g = (p >> 10) & 0x3ff;
                int b = p & 0x3ff;
                int a = (p >> 30) & 0x3;
                int r8 = (r * 255 + 511) / 1023;
                int g8 = (g * 255 + 511) / 1023;
                int b8 = (b * 255 + 511) / 1023;
                int a8 = a ? 255 : 0;
                pixels[x] = qRgba(r8, g8, b8, a8);
            }
        }
        break;

    case DXGI_FORMAT_R16G16B16A16_UNORM:
        for (int y = 0; y < height; ++y) {
            const auto* src = reinterpret_cast<const uint16_t*>(static_cast<const uchar*>(mapped.pData) + (y * mapped.RowPitch));
            auto* pixels = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < width; ++x) {
                int r = src[x * 4 + 0];
                int g = src[x * 4 + 1];
                int b = src[x * 4 + 2];
                int a = src[x * 4 + 3];
                int r8 = (r * 255 + 32767) / 65535;
                int g8 = (g * 255 + 32767) / 65535;
                int b8 = (b * 255 + 32767) / 65535;
                int a8 = (a * 255 + 32767) / 65535;
                pixels[x] = qRgba(r8, g8, b8, a8);
            }
        }
        break;

    case DXGI_FORMAT_R8G8B8A8_UNORM:
        for (int y = 0; y < height; ++y) {
            const auto* src = static_cast<const uchar*>(mapped.pData) + (y * mapped.RowPitch);
            auto* pixels = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < width; ++x) {
                int r = src[x * 4 + 0];
                int g = src[x * 4 + 1];
                int b = src[x * 4 + 2];
                int a = src[x * 4 + 3];
                pixels[x] = qRgba(r, g, b, a);
            }
        }
        break;

    default:
        for (int y = 0; y < height; ++y) {
            const auto* src = static_cast<const uchar*>(mapped.pData) + (y * mapped.RowPitch);
            std::memcpy(image.scanLine(y), src, std::min(static_cast<size_t>(width) * 4, srcRowBytes));
            auto* pixels = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < width; ++x) {
                pixels[x] |= 0xff000000;
            }
        }
        break;
    }

    return image;
}

class ScopedFrameRelease {
public:
    explicit ScopedFrameRelease(IDXGIOutputDuplication* duplication)
        : duplication_(duplication)
    {
    }
    ~ScopedFrameRelease()
    {
        if (duplication_ != nullptr) {
            duplication_->ReleaseFrame();
        }
    }
    ScopedFrameRelease(const ScopedFrameRelease&) = delete;
    ScopedFrameRelease& operator=(const ScopedFrameRelease&) = delete;
    void dismiss() { duplication_ = nullptr; }
private:
    IDXGIOutputDuplication* duplication_;
};

Result<QImage> captureSegmentWithDxgi(const ScreenCaptureSegment& segment,
                                      ID3D11Device& device,
                                      ID3D11DeviceContext& context)
{
    const auto& region = segment.logicalRegion;
    if (!region.isValid() || region.width() < 1 || region.height() < 1) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Capture region is invalid."));
    }

    const auto approximateScale = std::max<qreal>(segment.devicePixelRatio, 1.0);
    const POINT approximateCenter{
        static_cast<LONG>(std::llround(region.center().x() * approximateScale)),
        static_cast<LONG>(std::llround(region.center().y() * approximateScale)),
    };

    auto outputResult = findOutput(segment.screenName, approximateCenter);
    if (outputResult.isError()) {
        return Result<QImage>::failure(outputResult.error());
    }

    auto output = outputResult.value();
    const auto physicalRegion = toPhysicalRegion(region,
                                                 segment.logicalScreenGeometry,
                                                 segment.devicePixelRatio,
                                                 output.desc.DesktopCoordinates)
                                    .intersected(QRect(QPoint(output.desc.DesktopCoordinates.left, output.desc.DesktopCoordinates.top),
                                                       QPoint(output.desc.DesktopCoordinates.right - 1,
                                                              output.desc.DesktopCoordinates.bottom - 1)));
    if (!physicalRegion.isValid()) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Capture region is outside the selected output."));
    }

    ComPtr<IDXGIOutputDuplication> duplication;
    auto hr = output.output->DuplicateOutput(&device, duplication.GetAddressOf());
    if (FAILED(hr)) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "DXGI desktop duplication is not available."));
    }

    DXGI_OUTDUPL_FRAME_INFO frameInfo{};
    ComPtr<IDXGIResource> desktopResource;
    hr = duplication->AcquireNextFrame(100, &frameInfo, desktopResource.GetAddressOf());
    if (FAILED(hr)) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to acquire a DXGI desktop frame."));
    }

    ScopedFrameRelease frameGuard(duplication.Get());

    ComPtr<ID3D11Texture2D> desktopTexture;
    hr = desktopResource.As(&desktopTexture);
    if (FAILED(hr)) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "DXGI desktop frame is not a D3D11 texture."));
    }

    const auto width = physicalRegion.width();
    const auto height = physicalRegion.height();

    D3D11_TEXTURE2D_DESC stagingDesc{};
    desktopTexture->GetDesc(&stagingDesc);
    stagingDesc.Width = static_cast<UINT>(width);
    stagingDesc.Height = static_cast<UINT>(height);
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> stagingTexture;
    hr = device.CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf());
    if (FAILED(hr)) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to allocate DXGI readback texture."));
    }

    const D3D11_BOX sourceBox{
        static_cast<UINT>(physicalRegion.left() - output.desc.DesktopCoordinates.left),
        static_cast<UINT>(physicalRegion.top() - output.desc.DesktopCoordinates.top),
        0,
        static_cast<UINT>(physicalRegion.right() + 1 - output.desc.DesktopCoordinates.left),
        static_cast<UINT>(physicalRegion.bottom() + 1 - output.desc.DesktopCoordinates.top),
        1,
    };

    context.CopySubresourceRegion(stagingTexture.Get(), 0, 0, 0, 0, desktopTexture.Get(), 0, &sourceBox);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    hr = context.Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to map DXGI readback texture."));
    }

    auto image = mappedTextureToImage(mapped, width, height, stagingDesc.Format);
    context.Unmap(stagingTexture.Get(), 0);

    if (!image.isNull()) {
        bool allBlack = true;
        const int stepY = qMax(1, height / 8);
        const int stepX = qMax(1, width / 8);
        for (int y = 0; y < height && allBlack; y += stepY) {
            auto* pixels = reinterpret_cast<const QRgb*>(image.constScanLine(y));
            for (int x = 0; x < width; x += stepX) {
                if (qRed(pixels[x]) != 0 || qGreen(pixels[x]) != 0 || qBlue(pixels[x]) != 0) {
                    allBlack = false;
                    break;
                }
            }
        }
        if (allBlack) {
            return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "DXGI returned a black frame."));
        }
    }

    return Result<QImage>::success(std::move(image));
}
#endif

} // namespace

Result<QImage> DxgiScreenCaptureService::capturePrimaryScreen()
{
    auto* screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "No primary screen is available."));
    }

    return captureRegion(screen->geometry());
}

Result<QImage> DxgiScreenCaptureService::captureRegion(const QRect& region)
{
    return captureRegion(region, captureSegmentsFor(region));
}

Result<QImage> DxgiScreenCaptureService::captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments)
{
    if (!region.isValid() || region.width() < 1 || region.height() < 1) {
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Capture region is invalid."));
    }

    if (segments.isEmpty()) {
        return fallback_.captureRegion(region);
    }

#ifdef Q_OS_WIN
    if (segments.size() > 1) {
        QRect physicalBounds;
        qreal compositeDpr = 1.0;
        for (const auto& seg : segments) {
            auto dpr = seg.devicePixelRatio;
            compositeDpr = qMax(compositeDpr, dpr);
            QRect physSeg(qRound(seg.logicalRegion.x() * dpr),
                          qRound(seg.logicalRegion.y() * dpr),
                          qRound(seg.logicalRegion.width() * dpr),
                          qRound(seg.logicalRegion.height() * dpr));
            if (physicalBounds.isNull()) {
                physicalBounds = physSeg;
            } else {
                physicalBounds = physicalBounds.united(physSeg);
            }
        }

        QImage composite(physicalBounds.size(), QImage::Format_RGB32);
        composite.fill(Qt::black);
        QPainter painter(&composite);

        for (const auto& segment : segments) {
            auto segmentResult = captureWithDxgi(segment);
            if (segmentResult.isError()) {
                segmentResult = fallback_.captureRegion(segment.logicalRegion);
            }
            if (segmentResult.isError()) {
                return Result<QImage>::failure(segmentResult.error());
            }

            auto segmentImage = segmentResult.value();
            auto dpr = segment.devicePixelRatio;
            QPoint physicalOffset(
                qRound(segment.logicalRegion.x() * dpr) - physicalBounds.left(),
                qRound(segment.logicalRegion.y() * dpr) - physicalBounds.top());
            painter.drawImage(physicalOffset, segmentImage);
        }

        composite.setDevicePixelRatio(compositeDpr);
        return Result<QImage>::success(std::move(composite));
    }

    auto dxgiResult = captureWithDxgi(segments.first());
    if (dxgiResult.isError()) {
        dxgiResult = fallback_.captureRegion(region);
    }
    if (dxgiResult.isOk()) {
        auto image = dxgiResult.value();
        image.setDevicePixelRatio(segments.first().devicePixelRatio);
        return Result<QImage>::success(std::move(image));
    }
    return dxgiResult;
#else
    if (segments.size() > 1) {
        QRect physicalBounds;
        qreal compositeDpr = 1.0;
        for (const auto& seg : segments) {
            auto dpr = seg.devicePixelRatio;
            compositeDpr = qMax(compositeDpr, dpr);
            QRect physSeg(qRound(seg.logicalRegion.x() * dpr),
                          qRound(seg.logicalRegion.y() * dpr),
                          qRound(seg.logicalRegion.width() * dpr),
                          qRound(seg.logicalRegion.height() * dpr));
            if (physicalBounds.isNull()) {
                physicalBounds = physSeg;
            } else {
                physicalBounds = physicalBounds.united(physSeg);
            }
        }

        QImage composite(physicalBounds.size(), QImage::Format_RGB32);
        composite.fill(Qt::black);
        QPainter painter(&composite);
        for (const auto& segment : segments) {
            auto segmentResult = fallback_.captureRegion(segment.logicalRegion);
            if (segmentResult.isError()) {
                return Result<QImage>::failure(segmentResult.error());
            }
            auto segmentImage = segmentResult.value();
            auto dpr = segment.devicePixelRatio;
            QPoint physicalOffset(
                qRound(segment.logicalRegion.x() * dpr) - physicalBounds.left(),
                qRound(segment.logicalRegion.y() * dpr) - physicalBounds.top());
            painter.drawImage(physicalOffset, segmentImage);
        }
        composite.setDevicePixelRatio(compositeDpr);
        return Result<QImage>::success(std::move(composite));
    }
    return fallback_.captureRegion(region);
#endif
}

Result<void> DxgiScreenCaptureService::ensureD3dDevice()
{
    if (d3dDeviceValid_) {
        return Result<void>::success();
    }

    constexpr D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    D3D_FEATURE_LEVEL selectedLevel{};
    const auto flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    auto hr = D3D11CreateDevice(nullptr,
                                D3D_DRIVER_TYPE_HARDWARE,
                                nullptr,
                                flags,
                                featureLevels,
                                ARRAYSIZE(featureLevels),
                                D3D11_SDK_VERSION,
                                d3dDevice_.GetAddressOf(),
                                &selectedLevel,
                                d3dContext_.GetAddressOf());
    if (FAILED(hr)) {
        hr = D3D11CreateDevice(nullptr,
                               D3D_DRIVER_TYPE_WARP,
                               nullptr,
                               flags,
                               featureLevels,
                               ARRAYSIZE(featureLevels),
                               D3D11_SDK_VERSION,
                               d3dDevice_.GetAddressOf(),
                               &selectedLevel,
                               d3dContext_.GetAddressOf());
    }

    if (FAILED(hr)) {
        return Result<void>::failure("Failed to create D3D11 device.");
    }

    d3dDeviceValid_ = true;
    return Result<void>::success();
}

Result<QImage> DxgiScreenCaptureService::captureWithDxgi(const ScreenCaptureSegment& segment)
{
    std::scoped_lock lock(mutex_);
    constexpr int kMaxDxgiRetries = 3;
    for (int attempt = 0; attempt < kMaxDxgiRetries; ++attempt) {
        if (attempt > 0) {
            d3dDeviceValid_ = false;
            d3dDevice_.Reset();
            d3dContext_.Reset();
        }
        auto deviceResult = ensureD3dDevice();
        if (deviceResult.isError()) {
            break;
        }
        auto result = captureSegmentWithDxgi(segment, *d3dDevice_.Get(), *d3dContext_.Get());
        if (result.isOk()) {
            return result;
        }
        if (attempt < kMaxDxgiRetries - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50 * (attempt + 1)));
        }
    }
    return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to capture screen with DXGI."));
}

} // namespace snappaste
