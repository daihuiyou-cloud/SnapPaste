#include "presentation/pin_window/EditToolbarWidget.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QToolButton>

namespace snappaste {

namespace {

constexpr int kButtonWidth = 48;
constexpr int kButtonHeight = 40;
constexpr int kIconSize = 14;
constexpr int kToolbarPadding = 6;
constexpr int kToolbarSpacing = 3;
constexpr int kGroupSpacing = 10;

struct ToolDef {
    IconName icon;
    const char* text;
    const char* tooltip;
    AnnotationTool tool;
};

const ToolDef kToolDefs[] = {
    {IconName::Select,     "Select",     "Select",     AnnotationTool::Select},
    {IconName::Rectangle,  "Rect",       "Rectangle",  AnnotationTool::Rectangle},
    {IconName::Ellipse,    "Oval",       "Ellipse",    AnnotationTool::Ellipse},
    {IconName::Arrow,      "Arrow",      "Arrow",      AnnotationTool::Arrow},
    {IconName::Line,       "Line",       "Line",       AnnotationTool::Line},
    {IconName::Pen,        "Pen",        "Pen",        AnnotationTool::Pen},
    {IconName::Text,       "Text",       "Text",       AnnotationTool::Text},
    {IconName::Mosaic,     "Mosaic",     "Mosaic",     AnnotationTool::Mosaic},
    {IconName::Highlight,  "Marker",     "Highlight",  AnnotationTool::Highlight},
};

constexpr int kToolCount = sizeof(kToolDefs) / sizeof(kToolDefs[0]);

} // namespace

EditToolbarWidget::EditToolbarWidget(IIconProvider& iconProvider, QWidget* parent)
    : QWidget(parent)
    , iconProvider_(iconProvider)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(kToolbarPadding, kToolbarPadding, kToolbarPadding, kToolbarPadding);
    layout->setSpacing(kToolbarSpacing);

    for (int i = 0; i < kToolCount; ++i) {
        const auto& def = kToolDefs[i];
        auto* btn = createToolButton(def.icon, tr(def.text), tr(def.tooltip));
        connect(btn, &QToolButton::clicked, this, [this, i] {
            setCurrentTool(kToolDefs[i].tool);
            emit toolSelected(kToolDefs[i].tool);
        });
        toolButtons_.append(btn);
        layout->addWidget(btn);
    }

    auto* spacer1 = new QWidget(this);
    spacer1->setFixedWidth(kGroupSpacing);
    layout->addWidget(spacer1);

    undoBtn_ = createToolButton(IconName::Undo, tr("Undo"), tr("Undo (Ctrl+Z)"));
    connect(undoBtn_, &QToolButton::clicked, this, &EditToolbarWidget::undoRequested);
    layout->addWidget(undoBtn_);

    redoBtn_ = createToolButton(IconName::Redo, tr("Redo"), tr("Redo (Ctrl+Shift+Z)"));
    connect(redoBtn_, &QToolButton::clicked, this, &EditToolbarWidget::redoRequested);
    layout->addWidget(redoBtn_);

    auto* spacer2 = new QWidget(this);
    spacer2->setFixedWidth(kGroupSpacing);
    layout->addWidget(spacer2);

    doneBtn_ = createToolButton(IconName::Edit, tr("Done"), tr("Done (Esc)"));
    connect(doneBtn_, &QToolButton::clicked, this, &EditToolbarWidget::doneRequested);
    layout->addWidget(doneBtn_);

    setLayout(layout);
    adjustSize();
}

QToolButton* EditToolbarWidget::createToolButton(IconName icon, const QString& text, const QString& tooltip)
{
    auto* btn = new QToolButton(this);
    btn->setIcon(iconProvider_.icon(icon));
    btn->setIconSize(QSize(kIconSize, kIconSize));
    btn->setText(text);
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setToolTip(tooltip);
    btn->setFixedSize(kButtonWidth, kButtonHeight);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        "QToolButton {"
        " color: #f4fbff; font: 10px;"
        " font-family: 'Microsoft YaHei UI','Segoe UI',sans-serif;"
        " border: 1px solid transparent; border-radius: 4px;"
        " background: transparent;"
        "}"
        "QToolButton:hover {"
        " background: rgba(255,255,255,40);"
        " border-color: rgba(255,255,255,30);"
        "}"
        "QToolButton:pressed {"
        " background: rgba(47,191,159,60);"
        "}");
    return btn;
}

void EditToolbarWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(QColor(20, 26, 33, 220));
    painter.setPen(QPen(QColor(255, 255, 255, 30), 1));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 6, 6);
}

void EditToolbarWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = true;
        dragOffset_ = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void EditToolbarWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging_) {
        move(event->globalPos() - dragOffset_);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void EditToolbarWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && dragging_) {
        dragging_ = false;
        emit dragFinished();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void EditToolbarWidget::setCurrentTool(AnnotationTool tool)
{
    currentTool_ = tool;
    updateButtonStates();
}

void EditToolbarWidget::setCanUndo(bool enable)
{
    undoBtn_->setEnabled(enable);
}

void EditToolbarWidget::setCanRedo(bool enable)
{
    redoBtn_->setEnabled(enable);
}

void EditToolbarWidget::updateButtonStates()
{
    for (int i = 0; i < toolButtons_.size(); ++i) {
        const bool active = kToolDefs[i].tool == currentTool_;
        if (active) {
            toolButtons_[i]->setStyleSheet(
                "QToolButton {"
                " color: #f4fbff; font: 10px;"
                " font-family: 'Microsoft YaHei UI','Segoe UI',sans-serif;"
                " border: 1px solid #2fbf9f; border-radius: 4px;"
                " background: rgba(47,191,159,50);"
                "}"
                "QToolButton:hover {"
                " background: rgba(47,191,159,80);"
                "}");
        } else {
            toolButtons_[i]->setStyleSheet(
                "QToolButton {"
                " color: #f4fbff; font: 10px;"
                " font-family: 'Microsoft YaHei UI','Segoe UI',sans-serif;"
                " border: 1px solid transparent; border-radius: 4px;"
                " background: transparent;"
                "}"
                "QToolButton:hover {"
                " background: rgba(255,255,255,40);"
                " border-color: rgba(255,255,255,30);"
                "}"
                "QToolButton:pressed {"
                " background: rgba(47,191,159,60);"
                "}");
        }
    }
}

} // namespace snappaste
