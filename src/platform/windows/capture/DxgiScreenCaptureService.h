#pragma once

#include "domain/capture/IScreenCaptureService.h"
#include "platform/windows/capture/GdiScreenCaptureService.h"

#include <QRect>

#include <mutex>

namespace nanosnap {

class DxgiScreenCaptureService final : public IScreenCaptureService {
public:
    Result<QImage> capturePrimaryScreen() override;
    Result<QImage> captureRegion(const QRect& region) override;
    Result<QImage> captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments) override;

private:
    std::mutex mutex_;
    GdiScreenCaptureService fallback_;
};

} // namespace nanosnap
