#include "platform/windows/capture/QtScreenPixelSampler.h"

#include <QApplication>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QThread>

#include <algorithm>
#include <cmath>

namespace snappaste {

void QtScreenPixelSampler::assertMainThread() const
{
    Q_ASSERT_X(QThread::currentThread() == qApp->thread(),
               "QtScreenPixelSampler",
               "Must be called from the main thread");
}

void QtScreenPixelSampler::refresh(const QRect& desktopBounds)
{
    assertMainThread();
    bounds_ = desktopBounds;
    snapshot_ = {};
    snapshotOrigin_ = {};
    if (!bounds_.isValid()) {
        return;
    }

    struct GrabInfo {
        QRect logicalIntersect;
        qreal dpr = 1.0;
        QRect physSeg;
    };

    screenCache_.clear();
    QVector<GrabInfo> grabs;
    QRect physicalBounds;

    for (auto* screen : QGuiApplication::screens()) {
        if (screen == nullptr) continue;

        const auto intersection = screen->geometry().intersected(bounds_);
        if (!intersection.isValid()) continue;

        auto dpr = screen->devicePixelRatio();
        screenCache_.push_back({screen->geometry(), dpr});

        QRect physSeg(
            static_cast<int>(std::floor(intersection.x() * dpr)),
            static_cast<int>(std::floor(intersection.y() * dpr)),
            static_cast<int>(std::ceil((intersection.x() + intersection.width()) * dpr)) - static_cast<int>(std::floor(intersection.x() * dpr)),
            static_cast<int>(std::ceil((intersection.y() + intersection.height()) * dpr)) - static_cast<int>(std::floor(intersection.y() * dpr)));
        grabs.push_back({intersection, dpr, physSeg});

        if (physicalBounds.isNull()) {
            physicalBounds = physSeg;
        } else {
            physicalBounds = physicalBounds.united(physSeg);
        }
    }

    snapshot_ = QImage(physicalBounds.size(), QImage::Format_ARGB32_Premultiplied);
    snapshot_.fill(Qt::transparent);
    snapshotOrigin_ = physicalBounds.topLeft();

    QPainter painter(&snapshot_);
    for (const auto& g : grabs) {
        const auto pixmap = QGuiApplication::screenAt(g.logicalIntersect.center())->grabWindow(
            0, g.logicalIntersect.x(), g.logicalIntersect.y(),
            g.logicalIntersect.width(), g.logicalIntersect.height());
        if (pixmap.isNull()) continue;

        auto image = pixmap.toImage();

        QPoint physTarget(g.physSeg.x() - physicalBounds.left(),
                          g.physSeg.y() - physicalBounds.top());

        painter.drawImage(physTarget, image);
    }
}

QPoint QtScreenPixelSampler::physicalFromLogical(const QPoint& logicalPos) const
{
    for (const auto& info : screenCache_) {
        if (info.geometry.contains(logicalPos)) {
            return QPoint(
                static_cast<int>(std::floor(logicalPos.x() * info.dpr)),
                static_cast<int>(std::floor(logicalPos.y() * info.dpr)));
        }
    }
    if (!screenCache_.isEmpty()) {
        const auto& fallback = screenCache_.first();
        return QPoint(
            static_cast<int>(std::floor(logicalPos.x() * fallback.dpr)),
            static_cast<int>(std::floor(logicalPos.y() * fallback.dpr)));
    }
    return QPoint(
        static_cast<int>(std::floor(logicalPos.x())),
        static_cast<int>(std::floor(logicalPos.y())));
}

std::optional<QColor> QtScreenPixelSampler::sample(const QPoint& globalPosition) const
{
    if (snapshot_.isNull() || !bounds_.contains(globalPosition)) {
        return std::nullopt;
    }

    const auto physicalPos = physicalFromLogical(globalPosition);
    const auto local = physicalPos - snapshotOrigin_;
    if (!snapshot_.rect().contains(local)) {
        return std::nullopt;
    }
    QRgb pixel = reinterpret_cast<const QRgb*>(snapshot_.constScanLine(local.y()))[local.x()];
    return QColor::fromRgba(pixel);
}

QImage QtScreenPixelSampler::sampleRegion(const QPoint& center, int halfSize) const
{
    if (snapshot_.isNull() || !bounds_.isValid()) {
        return {};
    }
    const auto physicalCenter = physicalFromLogical(center);
    const auto local = physicalCenter - snapshotOrigin_;
    auto rect = QRect(local.x() - halfSize, local.y() - halfSize,
                      2 * halfSize + 1, 2 * halfSize + 1);
    rect = rect.intersected(snapshot_.rect());
    if (rect.width() < 1 || rect.height() < 1) {
        return {};
    }
    return snapshot_.copy(rect);
}

} // namespace snappaste
