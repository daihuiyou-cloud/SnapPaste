#pragma once

#include <QPoint>
#include <QRect>
#include <QVector>

namespace snappaste {

class IScreenRegionDetector {
public:
    virtual ~IScreenRegionDetector() = default;

    virtual QVector<QRect> regionsAt(const QPoint& globalPosition, const QRect& desktopBounds) = 0;
};

} // namespace snappaste
