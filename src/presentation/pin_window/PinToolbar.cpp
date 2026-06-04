#include "presentation/pin_window/PinToolbar.h"

#include <QPainter>

namespace snappaste {

namespace {

const QVector<QPixmap>& cachedToolbarPixmaps(IIconProvider& iconProvider)
{
    static QVector<QPixmap> cache;
    if (cache.isEmpty()) {
        const IconName icons[] = {
            IconName::Close,
            IconName::RotateLeft,
            IconName::RotateRight,
            IconName::FlipHorizontal,
            IconName::FlipVertical,
            IconName::Copy,
            IconName::ClickThrough,
            IconName::Pin
        };
        cache.reserve(8);
        for (auto name : icons)
            cache.push_back(iconProvider.icon(name).pixmap(PinToolbar::kIconSize, PinToolbar::kIconSize));
    }
    return cache;
}

} // namespace

QRect PinToolbar::rect(int parentWidth)
{
    const int tbWidth = kButtonCount * (kBtnSize + kBtnPad) + kBtnPad;
    return QRect((parentWidth - tbWidth) / 2, 4, tbWidth, kHeight);
}

QVector<QRect> PinToolbar::buttonRects(int parentWidth)
{
    const auto tb = rect(parentWidth);
    QVector<QRect> rects;
    rects.reserve(kButtonCount);
    int x = tb.left() + kBtnPad;
    const int y = tb.top() + (tb.height() - kBtnSize) / 2;
    for (int i = 0; i < kButtonCount; ++i) {
        rects.append(QRect(x, y, kBtnSize, kBtnSize));
        x += kBtnSize + kBtnPad;
    }
    return rects;
}

QRect PinToolbar::overflowRect(int parentWidth, int parentHeight)
{
    return QRect(parentWidth - kOverflowBtnSize - 4, parentHeight - kOverflowBtnSize - 4,
                 kOverflowBtnSize, kOverflowBtnSize);
}

bool PinToolbar::fits(int parentWidth, int parentHeight)
{
    const auto tb = rect(parentWidth);
    return parentWidth >= tb.width() && parentHeight >= tb.bottom() + 4;
}

void PinToolbar::draw(QPainter& painter, int parentWidth, int parentHeight, IIconProvider& iconProvider,
                      int hoveredButton, bool clickThroughActive, bool alwaysOnTopActive)
{
    Q_UNUSED(parentHeight)

    const auto tb = rect(parentWidth);
    painter.setBrush(QColor(20, 26, 33, 200));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(tb, 4, 4);

    const auto btns = buttonRects(parentWidth);
    const auto& pixmaps = cachedToolbarPixmaps(iconProvider);
    constexpr int kToggleIdxClickThrough = 6;
    constexpr int kToggleIdxAlwaysOnTop = 7;

    for (int i = 0; i < btns.size(); ++i) {
        const bool active = (i == kToggleIdxClickThrough && clickThroughActive)
                         || (i == kToggleIdxAlwaysOnTop && alwaysOnTopActive);

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

int PinToolbar::buttonAt(const QPoint& pos, int parentWidth)
{
    const auto btns = buttonRects(parentWidth);
    for (int i = 0; i < btns.size(); ++i) {
        if (btns[i].contains(pos)) {
            return i;
        }
    }
    return -1;
}

} // namespace snappaste
