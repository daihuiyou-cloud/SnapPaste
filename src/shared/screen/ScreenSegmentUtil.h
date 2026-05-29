#pragma once

#include "shared/types/ScreenCaptureTypes.h"

#include <QGuiApplication>
#include <QScreen>

namespace snappaste {

inline QVector<ScreenCaptureSegment> captureSegmentsFor(const QRect& region)
{
    QVector<ScreenCaptureSegment> segments;
    for (auto* screen : QGuiApplication::screens()) {
        if (screen == nullptr) {
            continue;
        }

        const auto logicalRegion = region.intersected(screen->geometry());
        if (!logicalRegion.isValid() || logicalRegion.width() < 1 || logicalRegion.height() < 1) {
            continue;
        }

        ScreenCaptureSegment segment;
        segment.logicalRegion = logicalRegion;
        segment.logicalScreenGeometry = screen->geometry();
        segment.screenName = screen->name();
        segment.devicePixelRatio = screen->devicePixelRatio();
        segments.push_back(segment);
    }
    return segments;
}

} // namespace snappaste
