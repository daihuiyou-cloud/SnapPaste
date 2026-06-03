#include "presentation/editor/EditorIconFactory.h"

#include <QPainter>
#include <QPixmap>

namespace snappaste {

QIcon iconForTool(AnnotationTool tool, IIconProvider& iconProvider)
{
    switch (tool) {
    case AnnotationTool::Rectangle: return iconProvider.icon(IconName::Rectangle);
    case AnnotationTool::Ellipse:   return iconProvider.icon(IconName::Ellipse);
    case AnnotationTool::Arrow:     return iconProvider.icon(IconName::Arrow);
    case AnnotationTool::Line:      return iconProvider.icon(IconName::Line);
    case AnnotationTool::Pen:       return iconProvider.icon(IconName::Pen);
    case AnnotationTool::Text:      return iconProvider.icon(IconName::Text);
    case AnnotationTool::Highlight: return iconProvider.icon(IconName::Highlight);
    case AnnotationTool::Numbered:  return iconProvider.icon(IconName::Numbered);
    case AnnotationTool::Mosaic:    return iconProvider.icon(IconName::Mosaic);
    case AnnotationTool::Eraser:    return iconProvider.icon(IconName::Eraser);
    case AnnotationTool::Select:    return iconProvider.icon(IconName::Select);
    case AnnotationTool::Crop:      return iconProvider.icon(IconName::Crop);
    default: return {};
    }
}

QIcon makeColorIcon(const QColor& color)
{
    QPixmap pix(16, 16);
    pix.fill(color);
    QPainter p(&pix);
    p.setPen(QPen(QColor(255, 255, 255, 48), 1));
    p.drawRect(QRectF(0.5, 0.5, 15, 15));
    p.end();
    return QIcon(pix);
}

} // namespace snappaste
