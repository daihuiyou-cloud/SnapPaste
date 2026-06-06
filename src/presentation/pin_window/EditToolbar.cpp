#include "presentation/pin_window/EditToolbar.h"

#include <QPainter>

namespace snappaste {

namespace {

constexpr AnnotationTool kButtonToolMap[] = {
    AnnotationTool::Select,      // 0
    AnnotationTool::Rectangle,   // 1
    AnnotationTool::Ellipse,     // 2
    AnnotationTool::Arrow,       // 3
    AnnotationTool::Line,        // 4
    AnnotationTool::Pen,         // 5
    AnnotationTool::Text,        // 6
    AnnotationTool::Mosaic,      // 7
    AnnotationTool::Highlight    // 8
};

const QVector<QPixmap>& cachedToolbarPixmaps(IIconProvider& iconProvider)
{
    static QVector<QPixmap> cache;
    if (cache.isEmpty()) {
        cache.reserve(EditToolbar::kButtonCount);

        auto load = [&](IconName name) {
            cache.push_back(iconProvider.icon(name).pixmap(EditToolbar::kIconSize, EditToolbar::kIconSize));
        };

        load(IconName::Select);
        load(IconName::Rectangle);
        load(IconName::Ellipse);
        load(IconName::Arrow);
        load(IconName::Line);
        load(IconName::Pen);
        load(IconName::Text);
        load(IconName::Mosaic);
        load(IconName::Highlight);
        load(IconName::Undo);
        load(IconName::Redo);
        load(IconName::Edit);
    }
    return cache;
}

} // namespace

QRect EditToolbar::rect(int parentWidth)
{
    const int tbWidth = kButtonCount * (kBtnSize + kBtnPad) + kBtnPad;
    return QRect((parentWidth - tbWidth) / 2, 4, tbWidth, kHeight);
}

QVector<QRect> EditToolbar::buttonRects(int parentWidth)
{
    QVector<QRect> rects;
    rects.reserve(kButtonCount);
    const auto tb = rect(parentWidth);
    int x = tb.left() + kBtnPad;
    const int y = tb.top() + (tb.height() - kBtnSize) / 2;
    for (int i = 0; i < kButtonCount; ++i) {
        rects.append(QRect(x, y, kBtnSize, kBtnSize));
        x += kBtnSize + kBtnPad;
    }
    return rects;
}

bool EditToolbar::fits(int parentWidth, int parentHeight)
{
    const auto tb = rect(parentWidth);
    return parentWidth >= tb.width() && parentHeight >= tb.bottom() + 4;
}

void EditToolbar::draw(QPainter& painter, int parentWidth, int parentHeight, IIconProvider& iconProvider,
                       int hoveredButton, AnnotationTool currentTool)
{
    Q_UNUSED(parentHeight)

    const auto tb = rect(parentWidth);
    painter.setBrush(QColor(20, 26, 33, 200));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(tb, 4, 4);

    const auto btns = buttonRects(parentWidth);
    const auto& pixmaps = cachedToolbarPixmaps(iconProvider);

    for (int i = 0; i < btns.size(); ++i) {
        const bool isTool = (i < 9);
        auto btnTool = isTool ? kButtonToolMap[i] : AnnotationTool::Select;
        const bool active = isTool && btnTool == currentTool;

        if (active) {
            painter.fillRect(btns[i], QColor(47, 191, 159, 50));
        } else if (i == hoveredButton) {
            painter.fillRect(btns[i], QColor(255, 255, 255, 48));
        } else {
            painter.fillRect(btns[i], QColor(255, 255, 255, 24));
        }

        if (active) {
            painter.setPen(QPen(QColor(47, 191, 159), 2));
            painter.drawRoundedRect(btns[i].adjusted(1, 1, -1, -1), 3, 3);
        }

        const auto iconTopLeft = btns[i].center() - QPoint(kIconSize / 2, kIconSize / 2);
        painter.drawPixmap(iconTopLeft, pixmaps[i]);
    }
}

int EditToolbar::buttonAt(const QPoint& pos, int parentWidth)
{
    const auto btns = buttonRects(parentWidth);
    for (int i = 0; i < btns.size(); ++i) {
        if (btns[i].contains(pos)) {
            return i;
        }
    }
    return -1;
}

AnnotationTool EditToolbar::toolAt(int buttonIndex)
{
    if (buttonIndex >= 0 && buttonIndex < static_cast<int>(sizeof(kButtonToolMap) / sizeof(kButtonToolMap[0]))) {
        return kButtonToolMap[buttonIndex];
    }
    return AnnotationTool::Select;
}

} // namespace snappaste
