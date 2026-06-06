#pragma once

#include "domain/editor/Annotation.h"
#include "presentation/icons/IIconProvider.h"

#include <QPoint>
#include <QRect>
#include <QVector>

class QPainter;

namespace snappaste {

struct EditToolbar {
    static constexpr int kHeight = 30;
    static constexpr int kBtnSize = 22;
    static constexpr int kIconSize = 14;
    static constexpr int kBtnPad = 4;
    static constexpr int kButtonCount = 12;

    static QRect rect(int parentWidth);
    static QVector<QRect> buttonRects(int parentWidth);
    static bool fits(int parentWidth, int parentHeight);
    static void draw(QPainter& painter, int parentWidth, int parentHeight, IIconProvider& iconProvider,
                     int hoveredButton, AnnotationTool currentTool);
    static int buttonAt(const QPoint& pos, int parentWidth);
};

} // namespace snappaste
