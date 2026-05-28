#include "platform/windows/capture/QtScreenPixelSampler.h"

#include <QGuiApplication>
#include <QPainter>
#include <QScreen>

#include <algorithm>
#include <cmath>

namespace snappaste {

void QtScreenPixelSampler::refresh(const QRect& desktopBounds)
{
    bounds_ = desktopBounds;
    snapshot_ = {};
    snapshotOrigin_ = {};
    if (!bounds_.isValid()) {
        return;
    }

    QRect physicalBounds;
    for (auto* screen : QGuiApplication::screens()) {
        if (screen == nullptr) continue;

        const auto intersection = screen->geometry().intersected(bounds_);
        if (!intersection.isValid()) continue;

        auto dpr = screen->devicePixelRatio();
        QRect physSeg(
            static_cast<int>(std::floor(intersection.x() * dpr)),
            static_cast<int>(std::floor(intersection.y() * dpr)),
            static_cast<int>(std::ceil((intersection.x() + intersection.width()) * dpr)) - static_cast<int>(std::floor(intersection.x() * dpr)),
            static_cast<int>(std::ceil((intersection.y() + intersection.height()) * dpr)) - static_cast<int>(std::floor(intersection.y() * dpr)));
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
    for (auto* screen : QGuiApplication::screens()) {
        if (screen == nullptr) continue;

        const auto intersection = screen->geometry().intersected(bounds_);
        if (!intersection.isValid()) continue;

        const auto pixmap = screen->grabWindow(0);
        if (pixmap.isNull()) continue;

        auto dpr = screen->devicePixelRatio();
        auto image = pixmap.toImage();
        image.setDevicePixelRatio(1.0);

        QPoint physTarget(
            static_cast<int>(std::floor(intersection.x() * dpr)) - physicalBounds.left(),
            static_cast<int>(std::floor(intersection.y() * dpr)) - physicalBounds.top());
        QSize physSize(
            static_cast<int>(std::ceil((intersection.x() + intersection.width()) * dpr)) - static_cast<int>(std::floor(intersection.x() * dpr)),
            static_cast<int>(std::ceil((intersection.y() + intersection.height()) * dpr)) - static_cast<int>(std::floor(intersection.y() * dpr)));

        painter.drawImage(physTarget, image, QRect(QPoint(0, 0), physSize));
    }
}

QPoint QtScreenPixelSampler::physicalFromLogical(const QPoint& logicalPos) const
{
    for (auto* screen : QGuiApplication::screens()) {
        if (screen == nullptr) continue;
        if (screen->geometry().contains(logicalPos)) {
            auto dpr = screen->devicePixelRatio();
            return QPoint(
                static_cast<int>(std::floor(logicalPos.x() * dpr)),
                static_cast<int>(std::floor(logicalPos.y() * dpr)));
        }
    }
    auto* primary = QGuiApplication::primaryScreen();
    auto dpr = primary ? primary->devicePixelRatio() : 1.0;
    return QPoint(
        static_cast<int>(std::floor(logicalPos.x() * dpr)),
        static_cast<int>(std::floor(logicalPos.y() * dpr)));
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
    return QColor::fromRgba(snapshot_.pixel(local));
}

QImage QtScreenPixelSampler::sampleRegion(const QPoint& center, int halfSize) const
{
    if (snapshot_.isNull() || !bounds_.isValid()) {
        return {};
    }
    const auto physicalCenter = physicalFromLogical(center);
    const auto local = physicalCenter - snapshotOrigin_;
    const auto rect = QRect(local.x() - halfSize, local.y() - halfSize,
                            2 * halfSize + 1, 2 * halfSize + 1);
    return snapshot_.copy(rect);
}

} // namespace snappaste
