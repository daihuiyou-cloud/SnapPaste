#pragma once

#include <QPoint>
#include <QRect>
#include <QVector>

class QPainter;

namespace snappaste {

struct PinToolbar {
    static constexpr int kHeight = 28;
    static constexpr int kBtnSize = 20;
    static constexpr int kIconSize = 14;
    static constexpr int kBtnPad = 4;
    static constexpr int kButtonCount = 7;
    static constexpr int kOverflowBtnSize = 18;

    static QRect rect(int parentWidth);
    static QVector<QRect> buttonRects(int parentWidth);
    static QRect overflowRect(int parentWidth, int parentHeight);
    static bool fits(int parentWidth, int parentHeight);
    static void draw(QPainter& painter, int parentWidth, int parentHeight);
    static int buttonAt(const QPoint& pos, int parentWidth);
};

} // namespace snappaste
