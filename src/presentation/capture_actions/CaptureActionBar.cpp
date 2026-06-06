#include "presentation/capture_actions/CaptureActionBar.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace snappaste {

namespace {

constexpr int kActionIconSize = 18;
constexpr int kActionBarMargin = 16;
constexpr int kActionBarPadding = 6;
constexpr int kActionBarSpacing = 5;

struct ActionDef {
    IconName icon;
    QString tooltip;
    QString label;
};

QToolButton* createActionButton(const ActionDef& def, IIconProvider& iconProvider, QWidget* parent)
{
    auto* btn = new QToolButton(parent);
    btn->setObjectName("CaptureActionButton");
    btn->setIcon(iconProvider.icon(def.icon));
    btn->setIconSize(QSize(kActionIconSize, kActionIconSize));
    btn->setText(def.label);
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setToolTip(def.tooltip);
    btn->setAccessibleName(def.tooltip);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFixedSize(56, 48);
    btn->setStyleSheet(
        "QToolButton#CaptureActionButton {"
        " color: #f4fbff; font: 11px;"
        " font-family: 'Microsoft YaHei UI','Segoe UI',sans-serif; padding: 2px 4px;"
        " border: 1px solid rgba(255,255,255,20); border-radius: 4px;"
        " background: rgba(255,255,255,12);"
        "}"
        "QToolButton#CaptureActionButton:hover {"
        " background: rgba(47,191,159,40);"
        " border-color: #2fbf9f;"
        "}"
        "QToolButton#CaptureActionButton:pressed {"
        " background: rgba(47,191,159,80);"
        "}");
    return btn;
}

int clamped(int value, int minimum, int maximum)
{
    return std::max(minimum, std::min(value, maximum));
}

} // namespace

CaptureActionBar::CaptureActionBar(IIconProvider& iconProvider, QWidget* parent)
    : QWidget(parent)
    , iconProvider_(iconProvider)
{
    setObjectName("CaptureActionBar");
    if (parent == nullptr) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    }
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFocusPolicy(Qt::StrongFocus);

    const ActionDef actions[] = {
        {IconName::Copy, tr("Copy (Enter)"), tr("Copy")},
        {IconName::Pin, tr("Pin (F3)"), tr("Pin")},
        {IconName::Save, tr("Save (Ctrl+S)"), tr("Save")},
        {IconName::Edit, tr("Edit (Space)"), tr("Edit")},
        {IconName::Close, tr("Cancel (Esc)"), tr("Cancel")}
    };

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(kActionBarPadding, kActionBarPadding, kActionBarPadding, kActionBarPadding);
    layout->setSpacing(kActionBarSpacing);

    auto* copyBtn = createActionButton(actions[0], iconProvider_, this);
    auto* pinBtn = createActionButton(actions[1], iconProvider_, this);
    auto* saveBtn = createActionButton(actions[2], iconProvider_, this);
    auto* editBtn = createActionButton(actions[3], iconProvider_, this);
    auto* closeBtn = createActionButton(actions[4], iconProvider_, this);

    layout->addWidget(copyBtn);
    layout->addWidget(pinBtn);
    layout->addWidget(saveBtn);
    layout->addWidget(editBtn);
    layout->addWidget(closeBtn);
    setLayout(layout);

    connect(copyBtn, &QToolButton::clicked, this, &CaptureActionBar::copyRequested);
    connect(pinBtn, &QToolButton::clicked, this, &CaptureActionBar::pinRequested);
    connect(saveBtn, &QToolButton::clicked, this, &CaptureActionBar::saveRequested);
    connect(editBtn, &QToolButton::clicked, this, &CaptureActionBar::editRequested);
    connect(closeBtn, &QToolButton::clicked, this, &CaptureActionBar::cancelRequested);
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
    case Qt::Key_Escape:
        emit cancelRequested();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

} // namespace snappaste
