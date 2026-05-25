#include "presentation/capture_actions/CaptureActionBar.h"

#include "presentation/icons/IconProvider.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>

#include <algorithm>

namespace snappaste {

namespace {

constexpr int kActionButtonSize = 36;
constexpr int kActionIconSize = 18;
constexpr int kActionBarMargin = 8;
constexpr int kActionBarPadding = 6;
constexpr int kActionBarSpacing = 5;

QPushButton* createIconButton(IconName iconName, const QString& tooltip, QWidget* parent)
{
    auto* button = new QPushButton(parent);
    button->setObjectName("CaptureActionButton");
    button->setIcon(IconProvider::icon(iconName));
    button->setIconSize(QSize(kActionIconSize, kActionIconSize));
    button->setToolTip(tooltip);
    button->setAccessibleName(tooltip);
    button->setFocusPolicy(Qt::NoFocus);
    button->setFixedSize(kActionButtonSize, kActionButtonSize);
    return button;
}

int clamped(int value, int minimum, int maximum)
{
    return std::max(minimum, std::min(value, maximum));
}

} // namespace

CaptureActionBar::CaptureActionBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("CaptureActionBar");
    if (parent == nullptr) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    }
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFocusPolicy(Qt::StrongFocus);

    auto* copyButton = createIconButton(IconName::Copy, "Copy (Enter)", this);
    auto* pinButton = createIconButton(IconName::Pin, "Pin (F3)", this);
    auto* saveButton = createIconButton(IconName::Save, "Save (Ctrl+S)", this);
    auto* editButton = createIconButton(IconName::Edit, "Edit (Space)", this);
    auto* ocrButton = createIconButton(IconName::Text, "OCR (O)", this);
    auto* closeButton = createIconButton(IconName::Close, "Cancel (Esc)", this);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(kActionBarPadding, kActionBarPadding, kActionBarPadding, kActionBarPadding);
    layout->setSpacing(kActionBarSpacing);
    layout->addWidget(copyButton);
    layout->addWidget(pinButton);
    layout->addWidget(saveButton);
    layout->addWidget(editButton);
    layout->addWidget(ocrButton);
    layout->addWidget(closeButton);
    setLayout(layout);

    connect(copyButton, &QPushButton::clicked, this, &CaptureActionBar::copyRequested);
    connect(pinButton, &QPushButton::clicked, this, &CaptureActionBar::pinRequested);
    connect(saveButton, &QPushButton::clicked, this, &CaptureActionBar::saveRequested);
    connect(editButton, &QPushButton::clicked, this, &CaptureActionBar::editRequested);
    connect(ocrButton, &QPushButton::clicked, this, &CaptureActionBar::ocrRequested);
    connect(closeButton, &QPushButton::clicked, this, &CaptureActionBar::cancelRequested);
}

void CaptureActionBar::showForRegion(const QRect& region, const QRect& availableGeometry)
{
    adjustSize();
    move(anchoredPosition(region, size(), availableGeometry));
    show();
    raise();
}

QPoint CaptureActionBar::anchoredPosition(const QRect& region, const QSize& barSize, const QRect& availableGeometry)
{
    if (!availableGeometry.isValid()) {
        return region.bottomLeft() + QPoint(0, kActionBarMargin);
    }

    const auto minX = availableGeometry.left() + kActionBarMargin;
    const auto maxX = availableGeometry.right() - barSize.width() - kActionBarMargin + 1;
    const auto minY = availableGeometry.top() + kActionBarMargin;
    const auto maxY = availableGeometry.bottom() - barSize.height() - kActionBarMargin + 1;

    auto x = clamped(region.right() - barSize.width() + 1, minX, std::max(minX, maxX));
    auto y = region.bottom() + kActionBarMargin;

    if (y > maxY) {
        y = region.top() - barSize.height() - kActionBarMargin;
    }
    if (y < minY || y > maxY) {
        x = region.right() + kActionBarMargin;
        if (x > maxX) {
            x = region.left() - barSize.width() - kActionBarMargin;
        }
        y = region.top();
    }

    x = clamped(x, minX, std::max(minX, maxX));
    y = clamped(y, minY, std::max(minY, maxY));
    return QPoint(x, y);
}

void CaptureActionBar::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit copyRequested();
        return;
    case Qt::Key_F3:
        emit pinRequested();
        return;
    case Qt::Key_S:
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            emit saveRequested();
            return;
        }
        break;
    case Qt::Key_Space:
        emit editRequested();
        return;
    case Qt::Key_O:
        emit ocrRequested();
        return;
    case Qt::Key_Escape:
        emit cancelRequested();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

} // namespace snappaste
