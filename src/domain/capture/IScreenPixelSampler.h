#pragma once

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QRect>

#include <optional>

namespace snappaste {

class IScreenPixelSampler {
public:
    virtual ~IScreenPixelSampler() = default;

    virtual void refresh(const QRect& desktopBounds) = 0;
    virtual std::optional<QColor> sample(const QPoint& globalPosition) const = 0;
    virtual QImage sampleRegion(const QPoint& center, int halfSize) const = 0;
};

} // namespace snappaste
