#pragma once

#include "domain/capture/IScreenPixelSampler.h"

#include <QImage>
#include <QPoint>

namespace snappaste {

class QtScreenPixelSampler final : public IScreenPixelSampler {
public:
    void refresh(const QRect& desktopBounds) override;
    std::optional<QColor> sample(const QPoint& globalPosition) const override;
    QImage sampleRegion(const QPoint& center, int halfSize) const override;

private:
    QRect bounds_;
    QImage snapshot_;
};

} // namespace snappaste
