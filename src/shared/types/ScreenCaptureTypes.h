#pragma once

#include <QRect>
#include <QString>
#include <QtGlobal>

namespace snappaste {

struct ScreenCaptureSegment final {
    QRect logicalRegion;
    QRect logicalScreenGeometry;
    QString screenName;
    qreal devicePixelRatio = 1.0;
};

} // namespace snappaste
