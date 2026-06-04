#include "shared/screen/ScreenSegmentUtil.h"

#include <QGuiApplication>
#include <QScreen>

namespace snappaste {

namespace {

struct ScreenInfo {
    QString name;
    QRect geometry;
    qreal dpr = 1.0;
};

const QVector<ScreenInfo>& cachedScreenInfos()
{
    static QVector<ScreenInfo> cache;
    const auto screens = QGuiApplication::screens();
    if (cache.size() == screens.size()) {
        return cache;
    }
    cache.clear();
    for (auto* screen : screens) {
        if (screen == nullptr) continue;
        cache.push_back({screen->name(), screen->geometry(), screen->devicePixelRatio()});
    }
    return cache;
}

} // namespace

QVector<ScreenCaptureSegment> captureSegmentsFor(const QRect& region)
{
    QVector<ScreenCaptureSegment> segments;
    for (const auto& info : cachedScreenInfos()) {
        const auto logicalRegion = region.intersected(info.geometry);
        if (!logicalRegion.isValid() || logicalRegion.width() < 1 || logicalRegion.height() < 1) {
            continue;
        }

        ScreenCaptureSegment segment;
        segment.logicalRegion = logicalRegion;
        segment.logicalScreenGeometry = info.geometry;
        segment.screenName = info.name;
        segment.devicePixelRatio = info.dpr;
        segments.push_back(std::move(segment));
    }
    return segments;
}

} // namespace snappaste
