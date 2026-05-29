#pragma once

#include "domain/capture/IScreenPixelSampler.h"

#include <QImage>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QThread>
#include <QVector>
#include <QtGlobal>

namespace snappaste {

class QtScreenPixelSampler final : public IScreenPixelSampler {
public:
    void refresh(const QRect& desktopBounds) override;
    std::optional<QColor> sample(const QPoint& globalPosition) const override;
    QImage sampleRegion(const QPoint& center, int halfSize) const override;

private:
    QPoint physicalFromLogical(const QPoint& logicalPos) const;

    void assertMainThread() const
    {
        Q_ASSERT_X(QThread::currentThread() == qApp->thread(),
                   "QtScreenPixelSampler",
                   "Must be called from the main thread");
    }

    struct ScreenInfo {
        QRect geometry;
        qreal dpr = 1.0;
    };
    QVector<ScreenInfo> screenCache_;

    QRect bounds_;
    QImage snapshot_;
    QPoint snapshotOrigin_;
};

} // namespace snappaste
