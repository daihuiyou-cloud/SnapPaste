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
    Line,
    Pen,
    Text,
    Mosaic,
    Undo,
    Redo,
    Export
};

struct IIconProvider {
    virtual ~IIconProvider() = default;
    virtual QIcon icon(IconName name) = 0;
};

} // namespace snappaste
