#pragma once

#include "domain/capture/IScreenPixelSampler.h"

#include <QImage>
#include <QPoint>
#include <QScreen>

namespace snappaste {

class QtScreenPixelSampler final : public IScreenPixelSampler {
public:
    void refresh(const QRect& desktopBounds) override;
    std::optional<QColor> sample(const QPoint& globalPosition) const override;
    QImage sampleRegion(const QPoint& center, int halfSize) const override;

private:
    QPoint physicalFromLogical(const QPoint& logicalPos) const;

    QRect bounds_;
    QImage snapshot_;
    QPoint snapshotOrigin_;
};

} // namespace snappaste
