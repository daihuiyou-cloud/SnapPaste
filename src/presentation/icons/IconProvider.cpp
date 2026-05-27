#include "presentation/icons/IconProvider.h"

namespace snappaste {

namespace {

QString pathFor(IconName name)
{
    switch (name) {
    case IconName::App:
        return ":/icons/app.svg";
    case IconName::Capture:
        return ":/icons/capture.svg";
    case IconName::Pin:
        return ":/icons/pin.svg";
    case IconName::Copy:
        return ":/icons/copy.svg";
    case IconName::Save:
        return ":/icons/save.svg";
    case IconName::Edit:
        return ":/icons/edit.svg";
    case IconName::Close:
        return ":/icons/close.svg";
    case IconName::RotateLeft:
        return ":/icons/rotate-left.svg";
    case IconName::RotateRight:
        return ":/icons/rotate-right.svg";
    case IconName::FlipHorizontal:
        return ":/icons/flip-horizontal.svg";
    case IconName::FlipVertical:
        return ":/icons/flip-vertical.svg";
    case IconName::Opacity:
        return ":/icons/opacity.svg";
    case IconName::ClickThrough:
        return ":/icons/click-through.svg";
    case IconName::Rectangle:
        return ":/icons/rectangle.svg";
    case IconName::Arrow:
        return ":/icons/arrow.svg";
    case IconName::Line:
        return ":/icons/line.svg";
    case IconName::Pen:
        return ":/icons/pen.svg";
    case IconName::Text:
        return ":/icons/text.svg";
    case IconName::Mosaic:
        return ":/icons/mosaic.svg";
    case IconName::Undo:
        return ":/icons/undo.svg";
    case IconName::Redo:
        return ":/icons/redo.svg";
    case IconName::Export:
        return ":/icons/export.svg";
    }
    return ":/icons/app.svg";
}

} // namespace

QIcon IconProvider::icon(IconName name)
{
    return QIcon(pathFor(name));
}

} // namespace snappaste
