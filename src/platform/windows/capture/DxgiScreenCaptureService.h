#pragma once

#include "domain/capture/IScreenCaptureService.h"
#include "platform/windows/capture/GdiScreenCaptureService.h"

#include <QRect>

#include <functional>
#include <mutex>

#ifdef Q_OS_WIN
#include <d3d11.h>
#include <wrl/client.h>
#endif

namespace snappaste {

class DxgiScreenCaptureService final : public IScreenCaptureService {
public:
    Result<QImage> capturePrimaryScreen() override;
    Result<QImage> captureRegion(const QRect& region) override;
    Result<QImage> captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments) override;

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
#endif
};

} // namespace snappaste
