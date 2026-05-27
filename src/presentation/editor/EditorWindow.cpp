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
#include <QScrollArea>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

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

    createToolbar();

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
            canvas_->renderedImage().save(path);
            QSettings().setValue("editor/lastSaveDir", QFileInfo(path).absolutePath());
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

void EditorWindow::createToolbar()
{
    auto* toolbar = new QToolBar("Editor", this);
    addToolBar(Qt::RightToolBarArea, toolbar);
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setStyleSheet(
        "QToolBar { spacing: 1px; padding: 6px 4px; }"
        "QToolButton { font: 9px 'Microsoft YaHei UI','Segoe UI',sans-serif;"
        "  padding: 3px 4px; text-align: left; }");

    const QString toggleBtnStyle =
        "QToolButton { font: bold 10px; color: #999; background: transparent;"
        "  border: none; border-radius: 4px; padding: 3px 5px; }"
        "QToolButton:hover { background: rgba(47,191,159,0.1); color: #2fbf9f; }"
        "QToolButton:checked { color: #fff; background: #2fbf9f; }"
        "QToolButton:hover:checked { background: #269d84; }";

    // ── Tool buttons ──
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
    QVector<QAction*> toolActions;
    toolActions.reserve(std::size(toolDefs));
    for (const auto& td : toolDefs) {
        auto* a = toolbar->addAction(td.iconFn(), td.name);
        a->setCheckable(true);
        a->setToolTip(td.tooltip);
        a->setData(static_cast<int>(td.tool));
        toolActions.push_back(a);
    }

    toolbar->addSeparator();

    // ─────────────────────────────────────────────
    // Group 2: Properties — Outline, Fill, Blur,
    //           Stroke(S/M/L), Font, Color
    // ─────────────────────────────────────────────
    struct PropToggle { const char* text; const char* tip; bool defaultOn; void (AnnotationCanvas::*setter)(bool); };
    const PropToggle props[] = {
        {"Outline", "Toggle text outline", true, &AnnotationCanvas::setTextOutlineEnabled},
        {"Fill", "Toggle fill for shapes", false, &AnnotationCanvas::setFilled},
        {"Blur", "Toggle mosaic blur mode", false, &AnnotationCanvas::setMosaicBlurred},
    };
    for (const auto& p : props) {
        auto* btn = new QToolButton(toolbar);
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
        toolbar->addWidget(btn);
    }

    // Stroke presets
    struct StrokePreset { QString label; int width; };
    const StrokePreset strokes[] = {{"S", 2}, {"M", 4}, {"L", 8}};
    auto* strokeGroup = new QButtonGroup(toolbar);
    strokeGroup->setExclusive(true);
    for (const auto& s : strokes) {
        auto* btn = new QToolButton(toolbar);
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
        toolbar->addWidget(btn);
    }

    // Font size label
    auto* fontSizeLabel = new QLabel(toolbar);
    fontSizeLabel->setText("14px");
    fontSizeLabel->setToolTip("Font size");
    fontSizeLabel->setStyleSheet("color: #bcbec6; font: 10px; padding: 0 4px; background: transparent;");
    toolbar->addWidget(fontSizeLabel);
    canvas_->setOnFontSizeChanged([fontSizeLabel](int size) {
        fontSizeLabel->setText(QString("%1px").arg(size));
    });

    // Color picker
    colorBtn_ = new QToolButton(toolbar);
    colorBtn_->setObjectName("colorWell");
    colorBtn_->setFixedHeight(26);
    colorBtn_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    colorBtn_->setPopupMode(QToolButton::InstantPopup);
    colorBtn_->setToolTip("Color");
    updateColorWell(QColor("#ff3b30"));

    auto* colorMenu = new QMenu(colorBtn_);
    eyeAction_ = new QAction(makeEyedropperIcon(), "Eyedropper", nullptr);
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
    toolbar->addWidget(colorBtn_);

    // ── Spacer: push actions to bottom ──
    auto* spacer = new QWidget(toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer->setStyleSheet("background: transparent;");
    toolbar->addWidget(spacer);

    // ─────────────────────────────────────────────
    // Group 3: Actions (bottom)
    // ─────────────────────────────────────────────
    toolbar->addSeparator();
    auto* undo = toolbar->addAction(IconProvider::icon(IconName::Undo), "Undo");
    auto* redo = toolbar->addAction(IconProvider::icon(IconName::Redo), "Redo");
    auto* copy = toolbar->addAction(IconProvider::icon(IconName::Copy), "Copy");
    auto* pinBtn = toolbar->addAction(IconProvider::icon(IconName::Pin), "Pin");
    auto* save = toolbar->addAction(IconProvider::icon(IconName::Save), "Save");
    auto* saveAs = toolbar->addAction(IconProvider::icon(IconName::Export), "Export...");

    // ── ToolTips ──
    undo->setToolTip("Undo (Ctrl+Z)");
    redo->setToolTip("Redo (Ctrl+Y)");
    copy->setToolTip("Copy Image (Ctrl+Shift+C)");
    pinBtn->setToolTip("Pin (F3)");
    save->setToolTip("Save to SnapPaste (Ctrl+S)");
    saveAs->setToolTip("Export to file (Ctrl+Shift+S)");

    // ── updateToolActions callback ──
    updateToolActions_ = [toolActions](AnnotationTool tool) {
        for (auto* a : toolActions)
            a->setChecked(static_cast<AnnotationTool>(a->data().toInt()) == tool);
    };
    updateToolActions_(AnnotationTool::Rectangle);

    // ── Connections ──
    connect(undo, &QAction::triggered, this, [this] { canvas_->undo(); });
    connect(redo, &QAction::triggered, this, [this] { canvas_->redo(); });
    for (auto* a : toolActions) {
        connect(a, &QAction::triggered, this, [this, tool = static_cast<AnnotationTool>(a->data().toInt())] {
            canvas_->setTool(tool);
        });
    }
    connect(copy, &QAction::triggered, this, [this] {
        emit imageEdited(canvas_->renderedImage());
        emit copyRequested();
        statusBar()->showMessage("Copied to clipboard", 3000);
    });
    connect(pinBtn, &QAction::triggered, this, [this] {
        auto img = canvas_->renderedImage();
        emit imageEdited(img);
        emit pinRequested(img);
        statusBar()->showMessage("Image pinned", 3000);
    });
    connect(save, &QAction::triggered, this, [this] {
        canvas_->clearModified();
        emit imageEdited(canvas_->renderedImage());
        emit saveRequested();
        statusBar()->showMessage("Saved", 3000);
    });
    connect(saveAs, &QAction::triggered, this, [this] {
        auto path = QFileDialog::getSaveFileName(this, "Save As", QString(),
            "PNG (*.png);;JPEG (*.jpg *.jpeg)");
        if (!path.isEmpty()) {
            canvas_->renderedImage().save(path);
            statusBar()->showMessage("Saved to " + path, 5000);
        }
    });
}

} // namespace snappaste
