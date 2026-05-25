#include "platform/windows/capture/QtScreenPixelSampler.h"

#include <QGuiApplication>
#include <QPainter>
#include <QScreen>

namespace snappaste {

void QtScreenPixelSampler::refresh(const QRect& desktopBounds)
{
    bounds_ = desktopBounds;
    snapshot_ = {};
    if (!bounds_.isValid()) {
        return;
    }

    snapshot_ = QImage(bounds_.size(), QImage::Format_ARGB32_Premultiplied);
    snapshot_.fill(Qt::transparent);

    QPainter painter(&snapshot_);
    for (auto* screen : QGuiApplication::screens()) {
        if (screen == nullptr) {
            continue;
        }

        const auto screenGeometry = screen->geometry().intersected(bounds_);
        if (!screenGeometry.isValid()) {
            continue;
        }

        const auto pixmap = screen->grabWindow(0);
        if (pixmap.isNull()) {
            continue;
        }

        const auto target = screenGeometry.translated(-bounds_.topLeft());
        painter.drawImage(target, pixmap.toImage());
    }
}

std::optional<QColor> QtScreenPixelSampler::sample(const QPoint& globalPosition) const
{
    if (snapshot_.isNull() || !bounds_.contains(globalPosition)) {
        return std::nullopt;
    }

    const auto local = globalPosition - bounds_.topLeft();
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
    const auto local = center - bounds_.topLeft();
    const auto rect = QRect(local.x() - halfSize, local.y() - halfSize,
                            2 * halfSize + 1, 2 * halfSize + 1);
    return snapshot_.copy(rect);
}

} // namespace snappaste
