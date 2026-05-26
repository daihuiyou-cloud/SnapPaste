#include "presentation/capture_actions/CaptureActionBar.h"

#include "presentation/icons/IconProvider.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace snappaste {

namespace {

constexpr int kActionIconSize = 18;
constexpr int kActionBarMargin = 8;
constexpr int kActionBarPadding = 6;
constexpr int kActionBarSpacing = 5;

struct ActionDef {
    IconName icon;
    QString tooltip;
    QString label;
};

QString verticalText(const QString& text)
{
    if (text.isEmpty()) return text;
    QStringList chars;
    chars.reserve(text.size());
    for (const QChar& ch : text) {
        chars << QString(ch);
    }
    return chars.join('\n');
}

QPushButton* createActionButton(const ActionDef& def, QWidget* parent)
{
    auto* btn = new QPushButton(verticalText(def.label), parent);
    btn->setObjectName("CaptureActionButton");
    btn->setIcon(IconProvider::icon(def.icon));
    btn->setIconSize(QSize(kActionIconSize, kActionIconSize));
    btn->setToolTip(def.tooltip);
    btn->setAccessibleName(def.tooltip);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFixedHeight(40);
    btn->setStyleSheet(
        "QPushButton#CaptureActionButton {"
        " color: #f4fbff; font-size: 10px; padding: 2px 8px;"
        " border: 1px solid rgba(255,255,255,20); border-radius: 4px;"
        " background: rgba(255,255,255,12);"
        "}"
        "QPushButton#CaptureActionButton:hover {"
        " background: rgba(47,191,159,40);"
        " border-color: #2fbf9f;"
        "}"
        "QPushButton#CaptureActionButton:pressed {"
        " background: rgba(47,191,159,80);"
        "}");
    return btn;
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

    const ActionDef actions[] = {
        {IconName::Copy, "Copy (Enter)", "Copy"},
        {IconName::Pin, "Pin (F3)", "Pin"},
        {IconName::Save, "Save (Ctrl+S)", "Save"},
        {IconName::Edit, "Edit (Space)", "Edit"},
        {IconName::Text, "OCR (O)", "OCR"},
        {IconName::Close, "Cancel (Esc)", "Cancel"}
    };

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(kActionBarPadding, kActionBarPadding, kActionBarPadding, kActionBarPadding);
    layout->setSpacing(kActionBarSpacing);

    auto* copyBtn = createActionButton(actions[0], this);
    auto* pinBtn = createActionButton(actions[1], this);
    auto* saveBtn = createActionButton(actions[2], this);
    auto* editBtn = createActionButton(actions[3], this);
    auto* ocrBtn = createActionButton(actions[4], this);
    auto* closeBtn = createActionButton(actions[5], this);

    layout->addWidget(copyBtn);
    layout->addWidget(pinBtn);
    layout->addWidget(saveBtn);
    layout->addWidget(editBtn);
    layout->addWidget(ocrBtn);
    layout->addWidget(closeBtn);
    setLayout(layout);

    connect(copyBtn, &QPushButton::clicked, this, &CaptureActionBar::copyRequested);
    connect(pinBtn, &QPushButton::clicked, this, &CaptureActionBar::pinRequested);
    connect(saveBtn, &QPushButton::clicked, this, &CaptureActionBar::saveRequested);
    connect(editBtn, &QPushButton::clicked, this, &CaptureActionBar::editRequested);
    connect(ocrBtn, &QPushButton::clicked, this, &CaptureActionBar::ocrRequested);
    connect(closeBtn, &QPushButton::clicked, this, &CaptureActionBar::cancelRequested);
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
