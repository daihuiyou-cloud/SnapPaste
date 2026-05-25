#pragma once

#include <QIcon>

namespace snappaste {

enum class IconName {
    App,
    Capture,
    Pin,
    Copy,
    Save,
    Edit,
    Close,
    RotateLeft,
    RotateRight,
    FlipHorizontal,
    FlipVertical,
    Opacity,
    ClickThrough,
    Rectangle,
    Arrow,
    Pen,
    Text,
    Mosaic
};

class IconProvider final {
public:
    static QIcon icon(IconName name);
};

} // namespace snappaste
