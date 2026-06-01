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
    Line,
    Pen,
    Text,
    Mosaic,
    Ellipse,
    Highlight,
    Eraser,
    Numbered,
    Crop
};

constexpr int kDefaultNumberedSize = 28;

enum class ArrowStyle {
    DefaultArrow,
    CircleArrow,
    SquareArrow
};

struct Annotation final {
    AnnotationTool tool = AnnotationTool::Rectangle;
    QRect bounds;
    QVector<QPoint> points;
    QString text;
    QColor color = QColor("#ff3b30");
    int strokeWidth = 3;
    int blurRadius = 0; // 0=pixel-block mosaic, >0=gaussian blur radius
    int number = 0;
    int textFontSize = 14;
    bool filled = false;
    bool textOutline = false;
    ArrowStyle arrowStyle = ArrowStyle::DefaultArrow;
    int cornerRadius = 0; // 0=sharp, >0 for rounded rect corners
    QString fontFamily;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    int textAlignment = -1; // -1 = default (AlignLeft|AlignTop), otherwise Qt::AlignmentFlag

    QColor fillColor;
    bool textBackground = false;
    QColor textBackgroundColor = QColor(0, 0, 0, 80);
};

} // namespace snappaste
