#pragma once

#include "domain/capture/IScreenPixelSampler.h"

#include <QImage>
#include <QPoint>

namespace nanosnap {

class QtScreenPixelSampler final : public IScreenPixelSampler {
public:
    void refresh(const QRect& desktopBounds) override;
    std::optional<QColor> sample(const QPoint& globalPosition) const override;

private:
    QRect bounds_;
    QImage snapshot_;
};

} // namespace nanosnap
