#pragma once

#include "shared/types/ScreenCaptureTypes.h"

#include <QRect>
#include <QVector>

namespace snappaste {

QVector<ScreenCaptureSegment> captureSegmentsFor(const QRect& region);

} // namespace snappaste
