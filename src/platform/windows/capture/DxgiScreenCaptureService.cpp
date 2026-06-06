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

#if defined(_M_X64) || defined(_M_IX86)
#include <emmintrin.h>
#endif

namespace snappaste {

#ifdef Q_OS_WIN
using Microsoft::WRL::ComPtr;
#endif

namespace {

QRect logicalToPhysical(const QRect& logical, qreal dpr)
{
    return QRect(qRound(logical.x() * dpr),
                 qRound(logical.y() * dpr),
                 qRound(logical.width() * dpr),
                 qRound(logical.height() * dpr));
}

#ifdef Q_OS_WIN

bool rectContains(const RECT& rect, const POINT& point)
{
    return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
}

QString deviceNameFrom(const DXGI_OUTPUT_DESC& desc)
{
    return QString::fromWCharArray(desc.DeviceName);
}

struct OutputMatch {
    ComPtr<IDXGIOutput1> output;
    DXGI_OUTPUT_DESC desc{};
};

Result<OutputMatch> findOutput(const QVector<DxgiScreenCaptureService::CachedOutput>& cache,
                                const QString& qtScreenName,
                                const POINT& physicalCenter)
{
    OutputMatch fallback;
    bool hasFallback = false;

    for (const auto& entry : cache) {
        auto name = deviceNameFrom(entry.desc);
        if (!qtScreenName.isEmpty() && name.compare(qtScreenName, Qt::CaseInsensitive) == 0) {
            OutputMatch match;
            match.output = entry.output;
            match.desc = entry.desc;
            return Result<OutputMatch>::success(std::move(match));
        }

        if (!hasFallback && rectContains(entry.desc.DesktopCoordinates, physicalCenter)) {
            fallback.output = entry.output;
            fallback.desc = entry.desc;
            hasFallback = true;
        }
    }

    if (hasFallback) {
        return Result<OutputMatch>::success(std::move(fallback));
    }

    return Result<OutputMatch>::failure(QCoreApplication::translate("AppErrors", "No matching DXGI output was found."));
}

#endif

} // anonymous namespace

#ifdef Q_OS_WIN

void DxgiScreenCaptureService::ensureFactoryCache()
{
    if (cacheValid_) return;

    cachedOutputs_.clear();
    cachedFactory_.Reset();

    auto hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(cachedFactory_.GetAddressOf()));
    if (FAILED(hr)) return;

    for (UINT adapterIndex = 0;; ++adapterIndex) {
        ComPtr<IDXGIAdapter1> adapter;
        if (cachedFactory_->EnumAdapters1(adapterIndex, adapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND) {
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

            cachedOutputs_.push_back({std::move(output1), desc});
        }
    }

    cacheValid_ = true;
}

#endif

#ifdef Q_OS_WIN

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
        }
        {
            auto* pixels = reinterpret_cast<uint32_t*>(image.bits());
            auto count = static_cast<size_t>(width) * height;
            for (size_t i = 0; i < count; ++i) pixels[i] |= 0xFF000000u;
        }
        break;

    case DXGI_FORMAT_R10G10B10A2_UNORM:
        for (int y = 0; y < height; ++y) {
            const auto* src = reinterpret_cast<const uint32_t*>(static_cast<const uchar*>(mapped.pData) + (y * mapped.RowPitch));
            auto* pixels = reinterpret_cast<QRgb*>(image.scanLine(y));
            int x = 0;
#if defined(_M_X64) || defined(_M_IX86)
            const auto magic255 = _mm_set1_epi32(255);
            const auto magic511_02 = _mm_set_epi32(0, 511, 0, 511);
            const auto mask10 = _mm_set1_epi32(0x3FF);
            for (; x + 4 <= width; x += 4) {
                auto v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + x));
                auto r10 = _mm_and_si128(_mm_srli_epi32(v, 20), mask10);
                auto g10 = _mm_and_si128(_mm_srli_epi32(v, 10), mask10);
                auto b10 = _mm_and_si128(v, mask10);
                auto a2 = _mm_srli_epi32(v, 30);
                auto aMask = _mm_cmpeq_epi32(a2, _mm_setzero_si128());
                auto a8 = _mm_andnot_si128(aMask, _mm_set1_epi32(255));
                auto scale = [&](__m128i vv) {
                    auto even = _mm_mul_epu32(vv, magic255);
                    even = _mm_add_epi32(even, magic511_02);
                    even = _mm_srli_epi64(even, 10);
                    auto shifted = _mm_srli_si128(vv, 4);
                    auto odd = _mm_mul_epu32(shifted, magic255);
                    odd = _mm_add_epi32(odd, magic511_02);
                    odd = _mm_srli_epi64(odd, 10);
                    auto lo = _mm_unpacklo_epi32(even, odd);
                    auto hi = _mm_unpackhi_epi32(even, odd);
                    return _mm_unpacklo_epi64(lo, hi);
                };
                auto r8 = scale(r10);
                auto g8 = scale(g10);
                auto b8 = scale(b10);
                r8 = _mm_slli_epi32(r8, 16);
                g8 = _mm_slli_epi32(g8, 8);
                a8 = _mm_slli_epi32(a8, 24);
                auto result = _mm_or_si128(b8, _mm_or_si128(g8, _mm_or_si128(r8, a8)));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pixels + x), result);
            }
#endif
            for (; x < width; ++x) {
                uint32_t p = src[x];
                int r = (p >> 20) & 0x3ff;
                int g = (p >> 10) & 0x3ff;
                int b = p & 0x3ff;
                int a = (p >> 30) & 0x3;
                int r8 = (r * 255 + 511) >> 10;
                int g8 = (g * 255 + 511) >> 10;
                int b8 = (b * 255 + 511) >> 10;
                int a8 = a ? 255 : 0;
                pixels[x] = qRgba(r8, g8, b8, a8);
            }
        }
        break;

    case DXGI_FORMAT_R16G16B16A16_UNORM:
        for (int y = 0; y < height; ++y) {
            const auto* src = reinterpret_cast<const uint16_t*>(static_cast<const uchar*>(mapped.pData) + (y * mapped.RowPitch));
            auto* pixels = reinterpret_cast<QRgb*>(image.scanLine(y));
            int x = 0;
#if defined(_M_X64) || defined(_M_IX86)
            const auto magic255_16 = _mm_set1_epi32(255);
            const auto magicHalf_02 = _mm_set_epi32(0, 32767, 0, 32767);
            const auto zero = _mm_setzero_si128();
            for (; x + 4 <= width; x += 4) {
                auto v0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + x * 4));
                auto v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + x * 4 + 8));
                auto pix0 = _mm_unpacklo_epi16(v0, zero);
                auto pix1 = _mm_unpackhi_epi16(v0, zero);
                auto pix2 = _mm_unpacklo_epi16(v1, zero);
                auto pix3 = _mm_unpackhi_epi16(v1, zero);
                auto t0 = _mm_unpacklo_epi32(pix0, pix1);
                auto t1 = _mm_unpackhi_epi32(pix0, pix1);
                auto t2 = _mm_unpacklo_epi32(pix2, pix3);
                auto t3 = _mm_unpackhi_epi32(pix2, pix3);
                auto R = _mm_unpacklo_epi64(t0, t2);
                auto G = _mm_unpackhi_epi64(t0, t2);
                auto B = _mm_unpacklo_epi64(t1, t3);
                auto A = _mm_unpackhi_epi64(t1, t3);
                auto scale16 = [&](__m128i vv) {
                    auto even = _mm_mul_epu32(vv, magic255_16);
                    even = _mm_add_epi32(even, magicHalf_02);
                    even = _mm_srli_epi64(even, 16);
                    auto shifted = _mm_srli_si128(vv, 4);
                    auto odd = _mm_mul_epu32(shifted, magic255_16);
                    odd = _mm_add_epi32(odd, magicHalf_02);
                    odd = _mm_srli_epi64(odd, 16);
                    auto lo = _mm_unpacklo_epi32(even, odd);
                    auto hi = _mm_unpackhi_epi32(even, odd);
                    return _mm_unpacklo_epi64(lo, hi);
                };
                R = scale16(R);
                G = scale16(G);
                B = scale16(B);
                A = scale16(A);
                R = _mm_slli_epi32(R, 16);
                G = _mm_slli_epi32(G, 8);
                A = _mm_slli_epi32(A, 24);
                auto result = _mm_or_si128(B, _mm_or_si128(G, _mm_or_si128(R, A)));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pixels + x), result);
            }
#endif
            for (; x < width; ++x) {
                int r = src[x * 4 + 0];
                int g = src[x * 4 + 1];
                int b = src[x * 4 + 2];
                int a = src[x * 4 + 3];
                int r8 = (r * 255 + 32767) >> 16;
                int g8 = (g * 255 + 32767) >> 16;
                int b8 = (b * 255 + 32767) >> 16;
                int a8 = (a * 255 + 32767) >> 16;
                pixels[x] = qRgba(r8, g8, b8, a8);
            }
        }
        break;

    case DXGI_FORMAT_R8G8B8A8_UNORM:
        for (int y = 0; y < height; ++y) {
            const auto* src = static_cast<const uchar*>(mapped.pData) + (y * mapped.RowPitch);
            auto* pixels = reinterpret_cast<uint32_t*>(image.scanLine(y));
            std::memcpy(pixels, src, std::min(static_cast<size_t>(width) * 4, srcRowBytes));
            // R8G8B8A8 memory order = R G B A (little-endian uint32 = 0xAABBGGRR)
            // Format_RGB32 on LE = B G R A (uint32 = 0xAARRGGBB)
            // Swap byte 0 <-> byte 2 in each pixel
            int x = 0;
#if defined(_M_X64) || defined(_M_IX86)
            const auto maskGA = _mm_set1_epi32(static_cast<int>(0xFF00FF00));
            const auto maskR = _mm_set1_epi32(0x00FF0000);
            const auto maskB = _mm_set1_epi32(0x000000FF);
            for (; x + 4 <= width; x += 4) {
                auto v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pixels + x));
                auto ga = _mm_and_si128(v, maskGA);
                auto r = _mm_and_si128(_mm_slli_epi32(v, 16), maskR);
                auto b = _mm_and_si128(_mm_srli_epi32(v, 16), maskB);
                auto result = _mm_or_si128(ga, _mm_or_si128(r, b));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pixels + x), result);
            }
#endif
            for (; x < width; ++x) {
                auto px = pixels[x];
                pixels[x] = (px & 0xFF00FF00u) | ((px & 0x000000FFu) << 16) | ((px >> 16) & 0x000000FFu);
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

Result<QImage> captureSegmentWithDxgi(const QVector<DxgiScreenCaptureService::CachedOutput>& cachedOutputs,
                                      const ScreenCaptureSegment& segment,
                                      ID3D11Device& device,
                                      ID3D11DeviceContext& context,
                                      DxgiScreenCaptureService::CachedDuplication& cachedDup,
                                      Microsoft::WRL::ComPtr<ID3D11Texture2D>& cachedStaging,
                                      UINT& cachedW, UINT& cachedH,
                                      DXGI_FORMAT& cachedFmt)
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

    auto outputResult = findOutput(cachedOutputs, segment.screenName, approximateCenter);
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

    auto outputName = deviceNameFrom(output.desc);
    if (cachedDup.duplication && cachedDup.outputName != outputName) {
        cachedDup.duplication.Reset();
        cachedDup.outputName.clear();
    }

    if (!cachedDup.duplication) {
        auto hr = output.output->DuplicateOutput(&device, cachedDup.duplication.GetAddressOf());
        if (FAILED(hr)) {
            return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "DXGI desktop duplication is not available."));
        }
        cachedDup.outputName = outputName;
    }

    DXGI_OUTDUPL_FRAME_INFO frameInfo{};
    ComPtr<IDXGIResource> desktopResource;
    auto hr = cachedDup.duplication->AcquireNextFrame(16, &frameInfo, desktopResource.GetAddressOf());
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_DEVICE_REMOVED) {
            cachedDup.duplication.Reset();
            cachedDup.outputName.clear();
        }
        return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to acquire a DXGI desktop frame."));
    }

    ScopedFrameRelease frameGuard(cachedDup.duplication.Get());

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
    bool reuseStaging = (cachedStaging != nullptr &&
                         cachedW == stagingDesc.Width &&
                         cachedH == stagingDesc.Height &&
                         cachedFmt == stagingDesc.Format);
    if (reuseStaging) {
        stagingTexture = cachedStaging;
    } else {
        hr = device.CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf());
        if (FAILED(hr)) {
            return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "Failed to allocate DXGI readback texture."));
        }
        cachedStaging = stagingTexture;
        cachedW = stagingDesc.Width;
        cachedH = stagingDesc.Height;
        cachedFmt = stagingDesc.Format;
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

    {
        auto isPixelBlack = [&](int px, int py) -> bool {
            if (px < 0 || px >= width || py < 0 || py >= height)
                return false;
            const auto* row = static_cast<const uchar*>(mapped.pData) + (static_cast<long long>(py) * mapped.RowPitch);
            switch (stagingDesc.Format) {
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8X8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                return (reinterpret_cast<const uint32_t*>(row)[px] & 0x00FFFFFF) == 0;
            case DXGI_FORMAT_R10G10B10A2_UNORM:
                return (reinterpret_cast<const uint32_t*>(row)[px] & 0x3FFFFFFF) == 0;
            case DXGI_FORMAT_R16G16B16A16_UNORM: {
                const auto* p = reinterpret_cast<const uint16_t*>(row) + static_cast<long long>(px) * 4;
                return p[0] == 0 && p[1] == 0 && p[2] == 0;
            }
            default:
                return false;
            }
        };
        bool allBlack = isPixelBlack(0, 0) &&
                        isPixelBlack(width - 1, 0) &&
                        isPixelBlack(0, height - 1) &&
                        isPixelBlack(width - 1, height - 1) &&
                        isPixelBlack(width / 2, height / 2);
        if (allBlack) {
            context.Unmap(stagingTexture.Get(), 0);
            return Result<QImage>::failure(QCoreApplication::translate("AppErrors", "DXGI returned an all-black frame; falling back to GDI."));
        }
    }

    auto image = mappedTextureToImage(mapped, width, height, stagingDesc.Format);
    context.Unmap(stagingTexture.Get(), 0);

    return Result<QImage>::success(std::move(image));
}
#endif // Q_OS_WIN

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
        return composeSegments(segments, [this](const ScreenCaptureSegment& seg) -> Result<QImage> {
            auto result = captureWithDxgi(seg);
            if (result.isError()) {
                result = fallback_.captureRegion(
                    logicalToPhysical(seg.logicalRegion, seg.devicePixelRatio), seg.devicePixelRatio);
            }
            return result;
        });
    }

    auto dxgiResult = captureWithDxgi(segments.first());
    if (dxgiResult.isError()) {
        const auto& seg = segments.first();
        dxgiResult = fallback_.captureRegion(logicalToPhysical(seg.logicalRegion, seg.devicePixelRatio), seg.devicePixelRatio);
    }
    if (dxgiResult.isOk()) {
        auto image = dxgiResult.value();
        image.setDevicePixelRatio(segments.first().devicePixelRatio);
        return Result<QImage>::success(std::move(image));
    }
    return dxgiResult;
#else
    if (segments.size() > 1) {
        return composeSegments(segments, [this](const ScreenCaptureSegment& seg) -> Result<QImage> {
            return fallback_.captureRegion(
                logicalToPhysical(seg.logicalRegion, seg.devicePixelRatio), seg.devicePixelRatio);
        });
    }
    return fallback_.captureRegion(region);
#endif
}

Result<void> DxgiScreenCaptureService::ensureD3dDeviceImpl()
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

Result<void> DxgiScreenCaptureService::ensureD3dDevice()
{
    std::scoped_lock lock(mutex_);
    return ensureD3dDeviceImpl();
}

Result<QImage> DxgiScreenCaptureService::composeSegments(
    const QVector<ScreenCaptureSegment>& segments,
    const std::function<Result<QImage>(const ScreenCaptureSegment&)>& captureOne)
{
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
        auto segmentResult = captureOne(segment);
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

Result<QImage> DxgiScreenCaptureService::captureWithDxgi(const ScreenCaptureSegment& segment)
{
    constexpr int kMaxDxgiRetries = 3;
    for (int attempt = 0; attempt < kMaxDxgiRetries; ++attempt) {
        if (attempt > 0) {
            std::scoped_lock lock(mutex_);
            d3dDeviceValid_ = false;
            d3dContext_.Reset();
            d3dDevice_.Reset();
            cacheValid_ = false;
            cachedFactory_.Reset();
            cachedOutputs_.clear();
            cachedStagingTexture_.Reset();
            cachedStagingWidth_ = 0;
            cachedStagingHeight_ = 0;
            cachedStagingFormat_ = DXGI_FORMAT_UNKNOWN;
            cachedDup_ = CachedDuplication{};
        }

        Result<QImage> result = Result<QImage>::failure({});
        {
            std::scoped_lock lock(mutex_);
            auto deviceResult = ensureD3dDeviceImpl();
            if (deviceResult.isError()) {
                break;
            }
            ensureFactoryCache();
            result = captureSegmentWithDxgi(cachedOutputs_, segment, *d3dDevice_.Get(), *d3dContext_.Get(),
                                            cachedDup_,
                                            cachedStagingTexture_, cachedStagingWidth_, cachedStagingHeight_,
                                            cachedStagingFormat_);
        }

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
