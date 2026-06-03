#pragma once

#include <QIcon>

namespace snappaste {

enum class IconName {
    App,
    Arrow,
    Capture,
    ClickThrough,
    Close,
    Copy,
    Crop,
    Edit,
    Ellipse,
    Eraser,
    Export,
    FlipHorizontal,
    FlipVertical,
    Highlight,
    Line,
    Mosaic,
    Numbered,
    Opacity,
    Pen,
    Pin,
    Rectangle,
    Redo,
    Rotate180,
    RotateLeft,
    RotateRight,
    Save,
    Select,
    Text,
    Undo
};

struct IIconProvider {
    virtual ~IIconProvider() = default;
    virtual QIcon icon(IconName name) = 0;
};

} // namespace snappaste
