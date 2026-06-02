#pragma once

#include "domain/capture/IScreenCaptureService.h"

namespace snappaste {

class GdiScreenCaptureService final : public IScreenCaptureService {
public:
    Result<QImage> capturePrimaryScreen() override;
    Result<QImage> captureRegion(const QRect& region) override;
    Result<QImage> captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments) override;
    Result<QImage> captureRegion(const QRect& physicalRegion, qreal devicePixelRatio);
};

} // namespace snappaste
