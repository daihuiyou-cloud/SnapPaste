#pragma once

#include "domain/capture/IScreenCaptureService.h"
#include "platform/windows/capture/GdiScreenCaptureService.h"

#include <QRect>
#include <QVector>

#include <functional>
#include <mutex>

#ifdef Q_OS_WIN
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#endif

namespace snappaste {

class DxgiScreenCaptureService final : public IScreenCaptureService {
public:
    Result<QImage> capturePrimaryScreen() override;
    Result<QImage> captureRegion(const QRect& region) override;
    Result<QImage> captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments) override;

    struct CachedOutput {
        Microsoft::WRL::ComPtr<IDXGIOutput1> output;
        DXGI_OUTPUT_DESC desc{};
    };

    struct CachedDuplication {
        Microsoft::WRL::ComPtr<IDXGIOutputDuplication> duplication;
        QString outputName;
    };

private:
    Result<QImage> captureWithDxgi(const ScreenCaptureSegment& segment);
    Result<QImage> composeSegments(const QVector<ScreenCaptureSegment>& segments,
        const std::function<Result<QImage>(const ScreenCaptureSegment&)>& captureOne);
    Result<void> ensureD3dDeviceImpl();
    Result<void> ensureD3dDevice();

    std::mutex mutex_;
    GdiScreenCaptureService fallback_;
#ifdef Q_OS_WIN
    Microsoft::WRL::ComPtr<ID3D11Device> d3dDevice_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3dContext_;
    bool d3dDeviceValid_ = false;

    Microsoft::WRL::ComPtr<IDXGIFactory1> cachedFactory_;
    QVector<CachedOutput> cachedOutputs_;
    bool cacheValid_ = false;
    void ensureFactoryCache();

    Microsoft::WRL::ComPtr<ID3D11Texture2D> cachedStagingTexture_;
    UINT cachedStagingWidth_ = 0;
    UINT cachedStagingHeight_ = 0;
    DXGI_FORMAT cachedStagingFormat_ = DXGI_FORMAT_UNKNOWN;

    CachedDuplication cachedDup_;
#endif
};

} // namespace snappaste
