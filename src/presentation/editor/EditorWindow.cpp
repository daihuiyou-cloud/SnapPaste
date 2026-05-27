#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>

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
    copyImageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C));
    connect(copyImageAction, &QAction::triggered, this, [this] {
        QApplication::clipboard()->setImage(canvas_->renderedImage());
        statusBar()->showMessage("Image copied to clipboard", 3000);
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

    auto* saveAsAction = new QAction("Save As", this);
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
    connect(saveAsAction, &QAction::triggered, this, [this] {
        QSettings settings;
        auto dir = settings.value("editor/lastSaveDir").toString();
        auto path = QFileDialog::getSaveFileName(this, "Save As", dir,
            "PNG (*.png);;JPEG (*.jpg *.jpeg)");
        if (!path.isEmpty()) {
            canvas_->renderedImage().save(path);
            QSettings().setValue("editor/lastSaveDir", QFileInfo(path).absolutePath());
        }
    });
    addAction(saveAsAction);
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

    // ─────────────────────────────────────────────
    // Group 1: Drawing Tools
    // ─────────────────────────────────────────────
    auto* rectangle = toolbar->addAction(IconProvider::icon(IconName::Rectangle), "Rectangle");
    rectangle->setCheckable(true);
    auto* ellipse = toolbar->addAction(makeEllipseIcon(), "Ellipse");
    ellipse->setCheckable(true);
    auto* arrow = toolbar->addAction(IconProvider::icon(IconName::Arrow), "Arrow");
    arrow->setCheckable(true);
    auto* lineTool = toolbar->addAction(IconProvider::icon(IconName::Line), "Line");
    lineTool->setCheckable(true);
    auto* pen = toolbar->addAction(IconProvider::icon(IconName::Pen), "Pen");
    pen->setCheckable(true);
    auto* textAction = toolbar->addAction(IconProvider::icon(IconName::Text), "Text");
    textAction->setCheckable(true);
    auto* highlight = toolbar->addAction(makeHighlightIcon(), "Highlight");
    highlight->setCheckable(true);
    auto* numbered = toolbar->addAction(makeNumberedIcon(), "Numbered");
    numbered->setCheckable(true);
    auto* mosaic = toolbar->addAction(IconProvider::icon(IconName::Mosaic), "Mosaic");
    mosaic->setCheckable(true);
    auto* eraser = toolbar->addAction(makeEraserIcon(), "Eraser");
    eraser->setCheckable(true);
    auto* select = toolbar->addAction(makeSelectIcon(), "Select");
    select->setCheckable(true);
    auto* crop = toolbar->addAction(makeCropIcon(), "Crop");
    crop->setCheckable(true);

    toolbar->addSeparator();

    // ─────────────────────────────────────────────
    // Group 2: Properties — Outline, Fill, Blur,
    //           Stroke(S/M/L), Font, Color
    // ─────────────────────────────────────────────
    auto* outlineBtn = new QToolButton(toolbar);
    outlineBtn->setText("Outline");
    outlineBtn->setToolTip("Toggle text outline");
    outlineBtn->setFixedHeight(24);
    outlineBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    outlineBtn->setCheckable(true);
    outlineBtn->setChecked(true);
    outlineBtn->setStyleSheet(toggleBtnStyle);
    connect(outlineBtn, &QToolButton::clicked, this, [this](bool checked) {
        canvas_->setTextOutlineEnabled(checked);
    });
    toolbar->addWidget(outlineBtn);

    auto* fillBtn = new QToolButton(toolbar);
    fillBtn->setText("Fill");
    fillBtn->setToolTip("Toggle fill for shapes");
    fillBtn->setFixedHeight(24);
    fillBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    fillBtn->setCheckable(true);
    fillBtn->setStyleSheet(toggleBtnStyle);
    connect(fillBtn, &QToolButton::clicked, this, [this](bool checked) {
        canvas_->setFilled(checked);
    });
    toolbar->addWidget(fillBtn);

    auto* mosaicBlurBtn = new QToolButton(toolbar);
    mosaicBlurBtn->setText("Blur");
    mosaicBlurBtn->setToolTip("Toggle mosaic blur mode");
    mosaicBlurBtn->setFixedHeight(24);
    mosaicBlurBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mosaicBlurBtn->setCheckable(true);
    mosaicBlurBtn->setStyleSheet(toggleBtnStyle);
    connect(mosaicBlurBtn, &QToolButton::clicked, this, [this](bool checked) {
        canvas_->setMosaicBlurred(checked);
    });
    toolbar->addWidget(mosaicBlurBtn);

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
    auto* colorBtn = new QToolButton(toolbar);
    colorBtn->setObjectName("colorWell");
    colorBtn->setFixedHeight(26);
    colorBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    colorBtn->setPopupMode(QToolButton::InstantPopup);
    colorBtn->setToolTip("Color");

    auto updateColorIcon = [colorBtn](const QColor& c) {
        QPixmap px(18, 18);
        px.fill(c);
        QPainter p(&px);
        p.setPen(QPen(QColor(255, 255, 255, 48), 1));
        p.drawRect(QRectF(0.5, 0.5, 17, 17));
        p.end();
        colorBtn->setIcon(QIcon(px));
    };
    updateColorIcon(QColor("#ff3b30"));

    const QColor fixedColors[] = {
        QColor("#ff3b30"), QColor("#ff9500"), QColor("#ffcc00"),
        QColor("#34c759"), QColor("#007aff"), QColor("#af52de"),
        QColor("#ffffff"), QColor("#000000")
    };

    auto* colorMenu = new QMenu(colorBtn);
    auto* eyeAction = new QAction(makeEyedropperIcon(), "Eyedropper", nullptr);
    eyeAction->setCheckable(true);
    connect(eyeAction, &QAction::triggered, this, [this, eyeAction] {
        canvas_->setPickingColor(eyeAction->isChecked());
    });
    canvas_->setOnPickingColorChanged([eyeAction](bool picking) {
        eyeAction->setChecked(picking);
    });
    colorMenu->addAction(eyeAction);

    connect(colorMenu, &QMenu::aboutToShow, this, [this, colorMenu, fixedColors, updateColorIcon, eyeAction]() {
        auto actions = colorMenu->actions();
        for (auto* action : actions) {
            if (action != eyeAction) {
                colorMenu->removeAction(action);
                delete action;
            }
        }
        auto recent = canvas_->recentColors();
        if (!recent.isEmpty()) {
            for (const auto& c : recent) {
                auto* a = new QAction(makeColorIcon(c), c.name(QColor::HexRgb).toUpper(), colorMenu);
                colorMenu->insertAction(eyeAction, a);
                connect(a, &QAction::triggered, this, [this, c, updateColorIcon] {
                    canvas_->setColor(c); updateColorIcon(c);
                });
            }
            colorMenu->insertSeparator(eyeAction);
        }
        for (const auto& c : fixedColors) {
            auto* a = new QAction(makeColorIcon(c), c.name(QColor::HexRgb).toUpper(), colorMenu);
            colorMenu->insertAction(eyeAction, a);
            connect(a, &QAction::triggered, this, [this, c, updateColorIcon] {
                canvas_->setColor(c); updateColorIcon(c);
            });
        }
        colorMenu->insertSeparator(eyeAction);
        auto* customAction = new QAction("Custom Color...", colorMenu);
        colorMenu->insertAction(eyeAction, customAction);
        connect(customAction, &QAction::triggered, this, [this, updateColorIcon] {
            auto color = QColorDialog::getColor(Qt::white, this, "Choose Color");
            if (color.isValid()) {
                canvas_->setColor(color);
                canvas_->addRecentColor(color);
                updateColorIcon(color);
            }
        });
        colorMenu->insertSeparator(eyeAction);
    });

    colorBtn->setMenu(colorMenu);
    toolbar->addWidget(colorBtn);

    // ── Spacer: push actions to bottom ──
    auto* spacer = new QWidget(toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer->setStyleSheet("background: transparent;");
    toolbar->addWidget(spacer);

    // ─────────────────────────────────────────────
    // Group 3: Actions (bottom)
    // ─────────────────────────────────────────────
    toolbar->addSeparator();
    auto* undo = toolbar->addAction(QIcon::fromTheme("edit-undo"), "Undo");
    auto* redo = toolbar->addAction(QIcon::fromTheme("edit-redo"), "Redo");
    auto* copy = toolbar->addAction(IconProvider::icon(IconName::Copy), "Copy");
    auto* pinBtn = toolbar->addAction(IconProvider::icon(IconName::Pin), "Pin");
    auto* save = toolbar->addAction(IconProvider::icon(IconName::Save), "Save");
    auto* saveAs = toolbar->addAction("Save As...");

    // ── ToolTips ──
    rectangle->setToolTip("Rectangle (R)");
    ellipse->setToolTip("Ellipse (E)");
    arrow->setToolTip("Arrow (A)");
    lineTool->setToolTip("Line (L)");
    pen->setToolTip("Pen (P)");
    textAction->setToolTip("Text (T)");
    highlight->setToolTip("Highlight (H)");
    numbered->setToolTip("Numbered (N)");
    mosaic->setToolTip("Mosaic (M)");
    eraser->setToolTip("Eraser (X)");
    select->setToolTip("Select (V)");
    crop->setToolTip("Crop (C)");
    undo->setToolTip("Undo (Ctrl+Z)");
    redo->setToolTip("Redo (Ctrl+Y)");
    copy->setToolTip("Copy");
    pinBtn->setToolTip("Pin (F3)");
    save->setToolTip("Save (Ctrl+S)");
    saveAs->setToolTip("Save As... (Ctrl+Shift+S)");

    // ── updateToolActions callback ──
    updateToolActions_ = [rectangle, ellipse, arrow, lineTool, pen, textAction, highlight, numbered, mosaic, select, eraser, crop](AnnotationTool tool) {
        QAction* lookup[] = {rectangle, ellipse, arrow, lineTool, pen, textAction, highlight, numbered, mosaic, select, eraser, crop};
        const AnnotationTool tools[] = {AnnotationTool::Rectangle, AnnotationTool::Ellipse, AnnotationTool::Arrow,
            AnnotationTool::Line, AnnotationTool::Pen, AnnotationTool::Text, AnnotationTool::Highlight, AnnotationTool::Numbered,
            AnnotationTool::Mosaic, AnnotationTool::Select, AnnotationTool::Eraser, AnnotationTool::Crop};
        for (auto* action : lookup) action->setChecked(false);
        for (int i = 0; i < 12; ++i) {
            if (tools[i] == tool) { lookup[i]->setChecked(true); break; }
        }
    };
    updateToolActions_(AnnotationTool::Rectangle);

    // ── Connections ──
    connect(undo, &QAction::triggered, this, [this] { canvas_->undo(); });
    connect(redo, &QAction::triggered, this, [this] { canvas_->redo(); });
    connect(rectangle, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Rectangle); });
    connect(ellipse, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Ellipse); });
    connect(arrow, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Arrow); });
    connect(lineTool, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Line); });
    connect(pen, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Pen); });
    connect(textAction, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Text); });
    connect(highlight, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Highlight); });
    connect(numbered, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Numbered); });
    connect(mosaic, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Mosaic); });
    connect(select, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Select); });
    connect(crop, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Crop); });
    connect(eraser, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Eraser); });
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
