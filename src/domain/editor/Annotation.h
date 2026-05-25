#pragma once

#include <QColor>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QVector>

namespace snappaste {

enum class AnnotationTool {
    Select,
    Rectangle,
    Arrow,
    Pen,
    Text,
    Mosaic,
    Ellipse,
    Highlight,
    Eraser
};

struct Annotation final {
    AnnotationTool tool = AnnotationTool::Rectangle;
    QRect bounds;
    QVector<QPoint> points;
    QString text;
    QColor color = QColor("#ff3b30");
    int strokeWidth = 3;
    int blurRadius = 0; // 0=pixel-block mosaic, >0=gaussian blur radius
};

} // namespace snappaste
