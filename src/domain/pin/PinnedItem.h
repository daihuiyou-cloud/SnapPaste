#pragma once

#include <QDateTime>
#include <QImage>
#include <QPoint>
#include <QSize>

#include <algorithm>

namespace snappaste {

enum class PinSource {
    Screenshot,
    Clipboard,
    File
};

struct PinTransform final {
    double scale = 1.0;
    int rotationDegrees = 0;
    bool flippedHorizontally = false;
    bool flippedVertically = false;
};

struct PinWindowOptions final {
    bool alwaysOnTop = true;
    bool clickThrough = false;
    bool visible = true;
};

struct PinnedImageState final {
    QPoint position;
    QSize size;
    double opacity = 1.0;
    PinTransform transform;
    PinWindowOptions options;
};

inline PinnedImageState normalizedState(PinnedImageState state)
{
    state.transform.scale = std::max(0.1, std::min(state.transform.scale, 8.0));
    state.opacity = std::max(0.2, std::min(state.opacity, 1.0));

    state.transform.rotationDegrees %= 360;
    if (state.transform.rotationDegrees < 0) {
        state.transform.rotationDegrees += 360;
    }

    if (state.size.width() < 24 || state.size.height() < 24) {
        state.size = QSize(std::max(24, state.size.width()), std::max(24, state.size.height()));
    }

    return state;
}

struct PinnedItem final {
    qint64 id = 0;
    QImage image;
    PinSource source = PinSource::Screenshot;
    PinnedImageState state;
    QDateTime createdAt;
    QDateTime updatedAt;
};

} // namespace snappaste
