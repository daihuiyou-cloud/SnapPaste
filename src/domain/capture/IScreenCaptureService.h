#pragma once

#include "shared/result/Result.h"
#include "shared/types/ScreenCaptureTypes.h"

#include <QImage>
#include <QRect>
#include <QString>
#include <QVector>

namespace snappaste {

class IScreenCaptureService {
public:
    virtual ~IScreenCaptureService() = default;

    virtual Result<QImage> capturePrimaryScreen() = 0;
    virtual Result<QImage> captureRegion(const QRect& region) = 0;
    virtual Result<QImage> captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments) = 0;
};

} // namespace snappaste
