#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>

#include "presentation/editor/AnnotationCanvas.h"
#include "presentation/editor/EditorWindow.h"

#include "presentation/icons/IconProvider.h"

#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QDockWidget>
#include <QFrame>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QSettings>
#include <QStatusBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace snappaste {

namespace {

QIcon makeEllipseIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#bcbec6"), 2));
    p.drawEllipse(QRectF(3, 3, 14, 14));
    p.end();
    return QIcon(pix);
}

QIcon makeHighlightIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(QRectF(3, 6, 14, 8), QColor(255, 230, 0, 140));
    p.setPen(QPen(QColor("#bcbec6"), 1));
    p.drawRect(QRectF(3, 6, 14, 8));
    p.end();
    return QIcon(pix);
}

QIcon makeSelectIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#bcbec6"), 1.5));
    p.drawRect(QRectF(3, 3, 14, 14));
    p.drawRect(QRectF(5, 5, 10, 10));
    // corner handles
    constexpr QPointF handles[] = {{3,3},{17,3},{3,17},{17,17}};
    p.setBrush(QColor("#bcbec6"));
    for (auto& pt : handles) p.drawRect(QRectF(pt.x()-2, pt.y()-2, 4, 4));
    p.end();
    return QIcon(pix);
}

QIcon makeEraserIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#ff3b30"), 2));
    p.drawLine(4, 4, 16, 16);
    p.drawLine(16, 4, 4, 16);
    p.end();
    return QIcon(pix);
}

QIcon makeCropIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#bcbec6"), 2));
    p.drawLine(3, 10, 3, 3);
    p.drawLine(3, 3, 10, 3);
    p.drawLine(17, 10, 17, 17);
    p.drawLine(10, 17, 17, 17);
    p.end();
    return QIcon(pix);
}

QIcon makeNumberedIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#bcbec6"), 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(2, 2, 16, 16));
    p.setPen(QColor("#bcbec6"));
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.drawText(QRectF(2, 2, 16, 16), Qt::AlignCenter, "1");
    p.end();
    return QIcon(pix);
}

QIcon makeEyedropperIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#bcbec6"), 2, Qt::SolidLine, Qt::RoundCap));
    p.drawEllipse(QPointF(6, 16), 3, 3);
    p.drawLine(7, 15, 15, 4);
    p.setBrush(QColor("#bcbec6"));
    p.drawRect(QRectF(12, 1, 6, 5));
    p.end();
    return QIcon(pix);
}

QIcon makeColorIcon(const QColor& color)
{
    QPixmap pix(16, 16);
    pix.fill(color);
    QPainter p(&pix);
    p.setPen(QPen(QColor(255, 255, 255, 48), 1));
    p.drawRect(QRectF(0.5, 0.5, 15, 15));
    p.end();
    return QIcon(pix);
}

} // namespace


EditorWindow::EditorWindow(QWidget* parent)
    : QMainWindow(parent)
    , canvas_(new AnnotationCanvas(this))
{
    connect(canvas_, &AnnotationCanvas::toolChanged, this, &EditorWindow::onToolChanged);

    setWindowTitle("SnapPaste Editor");
    resize(980, 680);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(canvas_);
    scrollArea->setAlignment(Qt::AlignCenter);
    setCentralWidget(scrollArea);

    createToolPanel();

    auto* undoAction = new QAction("Undo", this);
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, [this] { canvas_->undo(); });
    addAction(undoAction);

    auto* redoAction = new QAction("Redo", this);
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, [this] { canvas_->redo(); });
    addAction(redoAction);

    auto* copyImageAction = new QAction("Copy Image", this);
    copyImageAction->setShortcuts({QKeySequence::Copy, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C)});
    connect(copyImageAction, &QAction::triggered, this, [this] {
        emit imageEdited(canvas_->renderedImage());
        emit copyRequested();
        statusBar()->showMessage("Copied to clipboard", 3000);
    });
    addAction(copyImageAction);

    auto* pasteAction = new QAction("Paste Image", this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, [this] {
        auto pix = QApplication::clipboard()->pixmap();
        if (!pix.isNull()) {
            canvas_->setImage(pix.toImage());
            statusBar()->showMessage("Image pasted from clipboard", 3000);
        }
    });
    addAction(pasteAction);

    auto* saveAsAction = new QAction("Export", this);
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
    connect(saveAsAction, &QAction::triggered, this, [this] {
        QSettings settings;
        auto dir = settings.value("editor/lastSaveDir").toString();
        auto path = QFileDialog::getSaveFileName(this, "Export", dir,
            "PNG (*.png);;JPEG (*.jpg *.jpeg)");
        if (!path.isEmpty()) {
            if (canvas_->renderedImage().save(path)) {
                QSettings().setValue("editor/lastSaveDir", QFileInfo(path).absolutePath());
                statusBar()->showMessage("Saved to " + path, 3000);
            } else {
                statusBar()->showMessage("Failed to save image", 3000);
            }
        }
    });
    addAction(saveAsAction);

    auto* closeAction = new QAction("Close", this);
    closeAction->setShortcut(QKeySequence(Qt::Key_Escape));
    connect(closeAction, &QAction::triggered, this, &QWidget::close);
    addAction(closeAction);

    auto* saveAction = new QAction("Save", this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, [this] {
        canvas_->clearModified();
        emit imageEdited(canvas_->renderedImage());
        emit saveRequested();
        statusBar()->showMessage("Saved", 3000);
    });
    addAction(saveAction);

    auto* pinAction = new QAction("Pin", this);
    pinAction->setShortcut(QKeySequence(Qt::Key_F3));
    connect(pinAction, &QAction::triggered, this, [this] {
        auto img = canvas_->renderedImage();
        emit imageEdited(img);
        emit pinRequested(img);
        statusBar()->showMessage("Image pinned", 3000);
    });
    addAction(pinAction);
}

void EditorWindow::closeEvent(QCloseEvent* event)
{
    if (canvas_ && canvas_->isModified()) {
        auto ret = QMessageBox::question(this, "Unsaved Changes",
            "You have unsaved annotations. Save before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            auto img = canvas_->renderedImage();
            emit imageEdited(img);
            emit saveRequested();
            event->accept();
        } else if (ret == QMessageBox::Discard) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void EditorWindow::setImage(const QImage& image)
{
    canvas_->setImage(image);
    show();
    raise();
    activateWindow();
}

void EditorWindow::onToolChanged(AnnotationTool tool)
{
    if (updateToolActions_) {
        updateToolActions_(tool);
    }
}

void EditorWindow::updateColorWell(const QColor& c)
{
    QPixmap px(18, 18);
    px.fill(c);
    QPainter p(&px);
    p.setPen(QPen(QColor(255, 255, 255, 48), 1));
    p.drawRect(QRectF(0.5, 0.5, 17, 17));
    p.end();
    colorBtn_->setIcon(QIcon(px));
}

void EditorWindow::rebuildColorMenu()
{
    const QColor fixedColors[] = {
        QColor("#ff3b30"), QColor("#ff9500"), QColor("#ffcc00"),
        QColor("#34c759"), QColor("#007aff"), QColor("#af52de"),
        QColor("#ffffff"), QColor("#000000")
    };

    auto actions = colorBtn_->menu()->actions();
    for (auto* a : actions) {
        if (a != eyeAction_) {
            colorBtn_->menu()->removeAction(a);
            delete a;
        }
    }
    auto recent = canvas_->recentColors();
    if (!recent.isEmpty()) {
        for (const auto& c : recent) {
            auto* a = new QAction(makeColorIcon(c), c.name(QColor::HexRgb).toUpper(), colorBtn_->menu());
            colorBtn_->menu()->insertAction(eyeAction_, a);
            connect(a, &QAction::triggered, this, [this, c] {
                canvas_->setColor(c);
                updateColorWell(c);
            });
        }
        colorBtn_->menu()->insertSeparator(eyeAction_);
    }
    for (const auto& c : fixedColors) {
        auto* a = new QAction(makeColorIcon(c), c.name(QColor::HexRgb).toUpper(), colorBtn_->menu());
        colorBtn_->menu()->insertAction(eyeAction_, a);
        connect(a, &QAction::triggered, this, [this, c] {
            canvas_->setColor(c);
            updateColorWell(c);
        });
    }
    colorBtn_->menu()->insertSeparator(eyeAction_);
    auto* customAction = new QAction("Custom Color...", colorBtn_->menu());
    colorBtn_->menu()->insertAction(eyeAction_, customAction);
    connect(customAction, &QAction::triggered, this, [this] {
        auto color = QColorDialog::getColor(Qt::white, this, "Choose Color");
        if (color.isValid()) {
            canvas_->setColor(color);
            canvas_->addRecentColor(color);
            updateColorWell(color);
        }
    });
    colorBtn_->menu()->insertSeparator(eyeAction_);
}

void EditorWindow::createToolPanel()
{
    auto* dock = new QDockWidget("Tools", this);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setAllowedAreas(Qt::RightDockWidgetArea);
    dock->setTitleBarWidget(new QWidget());
    addDockWidget(Qt::RightDockWidgetArea, dock);

    auto* scrollArea = new QScrollArea(dock);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* content = new QWidget(scrollArea);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(8, 10, 8, 10);
    layout->setSpacing(8);

    // ── Styles ──
    const QString sectionHeaderStyle =
        "QLabel { color: #8e8e93; font: bold 9px 'Microsoft YaHei UI','Segoe UI',sans-serif;"
        "  padding: 0; }";

    const QString toggleBtnStyle =
        "QToolButton { font: bold 10px; color: #999; background: transparent;"
        "  border: none; border-radius: 4px; padding: 3px 5px; }"
        "QToolButton:hover { background: rgba(47,191,159,0.1); color: #2fbf9f; }"
        "QToolButton:checked { color: #fff; background: #2fbf9f; }"
        "QToolButton:hover:checked { background: #269d84; }";

    const QString panelBtnStyle =
        "QToolButton { font: 9px 'Microsoft YaHei UI','Segoe UI',sans-serif;"
        "  padding: 4px 6px; text-align: left; border: none; border-radius: 4px; }"
        "QToolButton:hover { background: rgba(255,255,255,0.06); }"
        "QToolButton:checked { background: rgba(47,191,159,0.15); color: #2fbf9f; }";

    const QString sectionFrameStyle =
        "QFrame#sectionGroup { background: rgba(255,255,255,0.03); border-radius: 6px; }";

    // Helper: create a section card with header + content inside
    auto addSection = [&](const QString& title, QLayout* contentLayout) {
        auto* frame = new QFrame(content);
        frame->setObjectName("sectionGroup");
        frame->setStyleSheet(sectionFrameStyle);
        auto* frameLayout = new QVBoxLayout(frame);
        frameLayout->setContentsMargins(6, 6, 6, 8);
        frameLayout->setSpacing(4);

        auto* headerRow = new QHBoxLayout();
        headerRow->setContentsMargins(0, 0, 0, 0);
        headerRow->setSpacing(6);
        auto* accent = new QFrame(frame);
        accent->setFixedSize(3, 12);
        accent->setStyleSheet("background: #2fbf9f; border-radius: 2px;");
        headerRow->addWidget(accent);
        auto* label = new QLabel(title, frame);
        label->setStyleSheet(sectionHeaderStyle);
        headerRow->addWidget(label);
        headerRow->addStretch();
        frameLayout->addLayout(headerRow);

        frameLayout->addLayout(contentLayout);
        layout->addWidget(frame);
    };

    // ════════════════════════════════════════════
    // Section 1: Annotation Tools (2-column grid)
    // ════════════════════════════════════════════
    struct ToolDef { std::function<QIcon()> iconFn; const char* name; const char* tooltip; AnnotationTool tool; };
    const ToolDef toolDefs[] = {
        {[]{ return IconProvider::icon(IconName::Rectangle); }, "Rectangle", "Rectangle (R)", AnnotationTool::Rectangle},
        {makeEllipseIcon, "Ellipse", "Ellipse (E)", AnnotationTool::Ellipse},
        {[]{ return IconProvider::icon(IconName::Arrow); }, "Arrow", "Arrow (A)", AnnotationTool::Arrow},
        {[]{ return IconProvider::icon(IconName::Line); }, "Line", "Line (L)", AnnotationTool::Line},
        {[]{ return IconProvider::icon(IconName::Pen); }, "Pen", "Pen (P)", AnnotationTool::Pen},
        {[]{ return IconProvider::icon(IconName::Text); }, "Text", "Text (T)", AnnotationTool::Text},
        {makeHighlightIcon, "Highlight", "Highlight (H)", AnnotationTool::Highlight},
        {makeNumberedIcon, "Numbered", "Numbered (N)", AnnotationTool::Numbered},
        {[]{ return IconProvider::icon(IconName::Mosaic); }, "Mosaic", "Mosaic (M)", AnnotationTool::Mosaic},
        {makeEraserIcon, "Eraser", "Eraser (X)", AnnotationTool::Eraser},
        {makeSelectIcon, "Select", "Select (V)", AnnotationTool::Select},
        {makeCropIcon, "Crop", "Crop (C)", AnnotationTool::Crop},
    };

    auto* toolsGrid = new QGridLayout();
    toolsGrid->setSpacing(2);
    toolsGrid->setContentsMargins(0, 0, 0, 0);

    QVector<QToolButton*> toolButtons;
    toolButtons.reserve(std::size(toolDefs));
    for (int i = 0; i < std::size(toolDefs); ++i) {
        auto* btn = new QToolButton(content);
        btn->setIcon(toolDefs[i].iconFn());
        btn->setText(toolDefs[i].name);
        btn->setToolTip(toolDefs[i].tooltip);
        btn->setCheckable(true);
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setIconSize(QSize(14, 14));
        btn->setStyleSheet(panelBtnStyle);
        btn->setFixedHeight(24);
        btn->setProperty("tool", static_cast<int>(toolDefs[i].tool));
        toolsGrid->addWidget(btn, i / 2, i % 2);
        toolButtons.push_back(btn);
    }
    addSection("ANNOTATION TOOLS", toolsGrid);

    // ════════════════════════════════════════════
    // Section 2: Properties
    // ════════════════════════════════════════════
    struct PropToggle { const char* text; const char* tip; bool defaultOn; void (AnnotationCanvas::*setter)(bool); };
    const PropToggle props[] = {
        {"Outline", "Toggle text outline", true, &AnnotationCanvas::setTextOutlineEnabled},
        {"Fill", "Toggle fill for shapes", false, &AnnotationCanvas::setFilled},
        {"Blur", "Toggle mosaic blur mode", false, &AnnotationCanvas::setMosaicBlurred},
    };

    auto* propsLayout = new QVBoxLayout();
    propsLayout->setContentsMargins(0, 0, 0, 0);
    propsLayout->setSpacing(2);
    for (const auto& p : props) {
        auto* btn = new QToolButton(content);
        btn->setText(p.text);
        btn->setToolTip(p.tip);
        btn->setFixedHeight(24);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setCheckable(true);
        btn->setChecked(p.defaultOn);
        btn->setStyleSheet(toggleBtnStyle);
        connect(btn, &QToolButton::clicked, this, [this, p](bool checked) {
            (canvas_->*p.setter)(checked);
        });
        propsLayout->addWidget(btn);
    }

    auto* fontSizeRow = new QHBoxLayout();
    fontSizeRow->setContentsMargins(0, 0, 0, 0);
    fontSizeRow->setSpacing(2);

    auto* fontSizeDec = new QToolButton(content);
    fontSizeDec->setText("−");
    fontSizeDec->setToolTip("Decrease font size ( [ )");
    fontSizeDec->setFixedSize(24, 24);
    fontSizeDec->setStyleSheet(toggleBtnStyle);

    auto* fontSizeLabel = new QLabel("14px", content);
    fontSizeLabel->setToolTip("Font size for Text / Numbered tools");
    fontSizeLabel->setAlignment(Qt::AlignCenter);
    fontSizeLabel->setStyleSheet("color: #bcbec6; font: 10px; padding: 0; background: transparent;");
    fontSizeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* fontSizeInc = new QToolButton(content);
    fontSizeInc->setText("+");
    fontSizeInc->setToolTip("Increase font size ( ] )");
    fontSizeInc->setFixedSize(24, 24);
    fontSizeInc->setStyleSheet(toggleBtnStyle);

    fontSizeRow->addWidget(fontSizeDec);
    fontSizeRow->addWidget(fontSizeLabel);
    fontSizeRow->addWidget(fontSizeInc);
    propsLayout->addLayout(fontSizeRow);

    connect(fontSizeDec, &QToolButton::clicked, this, [this] {
        canvas_->setFontSize(canvas_->fontSize() - 2);
    });
    connect(fontSizeInc, &QToolButton::clicked, this, [this] {
        canvas_->setFontSize(canvas_->fontSize() + 2);
    });
    canvas_->setOnFontSizeChanged([fontSizeLabel](int size) {
        fontSizeLabel->setText(QString("%1px").arg(size));
    });
    addSection("PROPERTIES", propsLayout);

    // ════════════════════════════════════════════
    // Section 3: Stroke
    // ════════════════════════════════════════════
    auto* strokeRow = new QHBoxLayout();
    strokeRow->setContentsMargins(0, 0, 0, 0);
    strokeRow->setSpacing(2);

    struct StrokePreset { QString label; int width; };
    const StrokePreset strokes[] = {{"S", 2}, {"M", 4}, {"L", 8}};
    auto* strokeGroup = new QButtonGroup(content);
    strokeGroup->setExclusive(true);
    for (const auto& s : strokes) {
        auto* btn = new QToolButton(content);
        btn->setText(s.label);
        btn->setToolTip(QString("Stroke: %1px").arg(s.width));
        btn->setFixedHeight(24);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setCheckable(true);
        btn->setStyleSheet(toggleBtnStyle);
        if (s.width == 4) btn->setChecked(true);
        strokeGroup->addButton(btn);
        connect(btn, &QToolButton::clicked, this, [this, s] {
            canvas_->setStrokeWidth(s.width);
        });
        strokeRow->addWidget(btn);
    }
    addSection("STROKE", strokeRow);

    // ════════════════════════════════════════════
    // Section 4: Color
    // ════════════════════════════════════════════
    colorBtn_ = new QToolButton(content);
    colorBtn_->setObjectName("colorWell");
    colorBtn_->setFixedHeight(32);
    colorBtn_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    colorBtn_->setPopupMode(QToolButton::InstantPopup);
    colorBtn_->setToolTip("Color");
    colorBtn_->setStyleSheet(
        "QToolButton#colorWell { border: 1px solid rgba(255,255,255,0.12);"
        "  border-radius: 4px; padding: 2px; }"
        "QToolButton#colorWell:hover { border-color: rgba(255,255,255,0.25); }");
    updateColorWell(QColor("#ff3b30"));

    auto* colorMenu = new QMenu(colorBtn_);
    eyeAction_ = new QAction(makeEyedropperIcon(), "Eyedropper", colorMenu);
    eyeAction_->setCheckable(true);
    connect(eyeAction_, &QAction::triggered, this, [this] {
        canvas_->setPickingColor(eyeAction_->isChecked());
    });
    canvas_->setOnPickingColorChanged([this](bool picking) {
        eyeAction_->setChecked(picking);
    });
    colorMenu->addAction(eyeAction_);
    connect(colorMenu, &QMenu::aboutToShow, this, &EditorWindow::rebuildColorMenu);
    colorBtn_->setMenu(colorMenu);

    auto* colorLayout = new QVBoxLayout();
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->addWidget(colorBtn_);
    addSection("COLOR", colorLayout);

    // ── Spacer: push actions to bottom ──
    layout->addStretch(1);

    // ════════════════════════════════════════════
    // Section 5: Actions (2-column grid)
    // ════════════════════════════════════════════
    struct ActionDef { QIcon icon; const char* text; const char* tip; };
    const ActionDef actionDefs[] = {
        {IconProvider::icon(IconName::Undo), "Undo", "Undo (Ctrl+Z)"},
        {IconProvider::icon(IconName::Redo), "Redo", "Redo (Ctrl+Y)"},
        {IconProvider::icon(IconName::Copy), "Copy", "Copy (Ctrl+Shift+C)"},
        {IconProvider::icon(IconName::Pin), "Pin", "Pin (F3)"},
        {IconProvider::icon(IconName::Save), "Save", "Save (Ctrl+S)"},
        {IconProvider::icon(IconName::Export), "Export...", "Export (Ctrl+Shift+S)"},
    };

    auto* actionsGrid = new QGridLayout();
    actionsGrid->setContentsMargins(0, 0, 0, 0);
    actionsGrid->setSpacing(2);

    QVector<QToolButton*> actionButtons;
    actionButtons.reserve(std::size(actionDefs));
    for (int i = 0; i < std::size(actionDefs); ++i) {
        auto* btn = new QToolButton(content);
        btn->setIcon(actionDefs[i].icon);
        btn->setText(actionDefs[i].text);
        btn->setToolTip(actionDefs[i].tip);
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setIconSize(QSize(14, 14));
        btn->setStyleSheet(panelBtnStyle);
        btn->setFixedHeight(24);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        actionsGrid->addWidget(btn, i / 2, i % 2);
        actionButtons.push_back(btn);
    }
    addSection("ACTIONS", actionsGrid);

    // ── updateToolActions callback ──
    updateToolActions_ = [toolButtons](AnnotationTool tool) {
        for (auto* btn : toolButtons)
            btn->setChecked(static_cast<AnnotationTool>(btn->property("tool").toInt()) == tool);
    };
    updateToolActions_(AnnotationTool::Rectangle);

    // ── Connections ──
    connect(actionButtons[0], &QToolButton::clicked, this, [this] { canvas_->undo(); });
    connect(actionButtons[1], &QToolButton::clicked, this, [this] { canvas_->redo(); });
    for (auto* btn : toolButtons) {
        connect(btn, &QToolButton::clicked, this, [this, tool = static_cast<AnnotationTool>(btn->property("tool").toInt())] {
            canvas_->setTool(tool);
        });
    }
    connect(actionButtons[2], &QToolButton::clicked, this, [this] {
        emit imageEdited(canvas_->renderedImage());
        emit copyRequested();
        statusBar()->showMessage("Copied to clipboard", 3000);
    });
    connect(actionButtons[3], &QToolButton::clicked, this, [this] {
        auto img = canvas_->renderedImage();
        emit imageEdited(img);
        emit pinRequested(img);
        statusBar()->showMessage("Image pinned", 3000);
    });
    connect(actionButtons[4], &QToolButton::clicked, this, [this] {
        canvas_->clearModified();
        emit imageEdited(canvas_->renderedImage());
        emit saveRequested();
        statusBar()->showMessage("Saved", 3000);
    });
    connect(actionButtons[5], &QToolButton::clicked, this, [this] {
        auto path = QFileDialog::getSaveFileName(this, "Save As", QString(),
            "PNG (*.png);;JPEG (*.jpg *.jpeg)");
        if (!path.isEmpty()) {
            if (canvas_->renderedImage().save(path)) {
                statusBar()->showMessage("Saved to " + path, 5000);
            } else {
                statusBar()->showMessage("Failed to save image", 5000);
            }
        }
    });

    scrollArea->setWidget(content);
    dock->setWidget(scrollArea);
}

} // namespace snappaste
