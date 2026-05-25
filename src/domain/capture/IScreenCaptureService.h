#pragma once

#include "shared/result/Result.h"

#include <QImage>
#include <QRect>
#include <QString>
#include <QtGlobal>
#include <QVector>

namespace snappaste {

struct ScreenCaptureSegment final {
    QRect logicalRegion;
    QRect logicalScreenGeometry;
    QString screenName;
    qreal devicePixelRatio = 1.0;
};

class IScreenCaptureService {
public:
    virtual ~IScreenCaptureService() = default;

    virtual Result<QImage> capturePrimaryScreen() = 0;
    virtual Result<QImage> captureRegion(const QRect& region) = 0;
    virtual Result<QImage> captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments)
    {
        Q_UNUSED(segments)
        return captureRegion(region);
    }
};

} // namespace snappaste
