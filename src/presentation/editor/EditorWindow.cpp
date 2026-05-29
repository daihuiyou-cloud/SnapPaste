#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>

#include "presentation/editor/AnnotationCanvas.h"
#include "presentation/editor/EditorIconFactory.h"
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

class StrokePreview : public QWidget {
public:
    explicit StrokePreview(QWidget* parent) : QWidget(parent) { setFixedHeight(16); }
    void showStroke(QColor color, int width) {
        if (color_ != color || width_ != width) {
            color_ = color;
            width_ = width;
            update();
        }
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(color_, qMax(1, width_), Qt::SolidLine, Qt::RoundCap));
        int y = height() / 2;
        p.drawLine(QPointF(6, y), QPointF(width() - 6, y));
    }
private:
    QColor color_{"#ff3b30"};
    int width_ = 4;
};

const char* scrollBarStyle = R"(
QScrollBar:vertical {
    background: transparent; width: 7px; margin: 0;
}
QScrollBar::handle:vertical {
    background: rgba(255,255,255,0.1); min-height: 28px; border-radius: 3px;
}
QScrollBar::handle:vertical:hover { background: rgba(255,255,255,0.18); }
QScrollBar::handle:vertical:pressed { background: rgba(255,255,255,0.25); }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal {
    background: transparent; height: 7px; margin: 0;
}
QScrollBar::handle:horizontal {
    background: rgba(255,255,255,0.1); min-width: 28px; border-radius: 3px;
}
QScrollBar::handle:horizontal:hover { background: rgba(255,255,255,0.18); }
QScrollBar::handle:horizontal:pressed { background: rgba(255,255,255,0.25); }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }
QScrollBar::corner { background: transparent; }
)";

} // namespace


EditorWindow::EditorWindow(QWidget* parent)
    : QMainWindow(parent)
    , canvas_(new AnnotationCanvas(this))
{
    connect(canvas_, &AnnotationCanvas::toolChanged, this, &EditorWindow::onToolChanged);

    setWindowTitle(tr("SnapPaste Editor"));
    resize(980, 680);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(canvas_);
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setStyleSheet(scrollBarStyle);
    setCentralWidget(scrollArea);

    createToolPanel();
    setupActions();
}

void EditorWindow::setupActions()
{
    auto* undoAction = new QAction(tr("Undo"), this);
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, [this] { canvas_->undo(); });
    addAction(undoAction);

    auto* redoAction = new QAction(tr("Redo"), this);
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, [this] { canvas_->redo(); });
    addAction(redoAction);

    auto* copyImageAction = new QAction(tr("Copy Image"), this);
    copyImageAction->setShortcuts({QKeySequence::Copy, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C)});
    connect(copyImageAction, &QAction::triggered, this, [this] {
        emit imageEdited(canvas_->renderedImage());
        emit copyRequested();
        statusBar()->showMessage(tr("Copied to clipboard"), 3000);
    });
    addAction(copyImageAction);

    auto* pasteAction = new QAction(tr("Paste Image"), this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, [this] {
        auto pix = QApplication::clipboard()->pixmap();
        if (!pix.isNull()) {
            canvas_->setImage(pix.toImage());
            statusBar()->showMessage(tr("Image pasted from clipboard"), 3000);
        }
    });
    addAction(pasteAction);

    auto* saveAsAction = new QAction(tr("Export"), this);
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
    connect(saveAsAction, &QAction::triggered, this, [this] {
        QSettings settings;
        auto dir = settings.value("editor/lastSaveDir").toString();
        auto path = QFileDialog::getSaveFileName(this, "Export", dir,
            "PNG (*.png);;JPEG (*.jpg *.jpeg)");
        if (!path.isEmpty()) {
            if (canvas_->renderedImage().save(path)) {
                QSettings().setValue("editor/lastSaveDir", QFileInfo(path).absolutePath());
                statusBar()->showMessage(tr("Saved to %1").arg(path), 3000);
            } else {
                statusBar()->showMessage(tr("Failed to save image"), 3000);
            }
        }
    });
    addAction(saveAsAction);

    auto* closeAction = new QAction(tr("Close"), this);
    closeAction->setShortcut(QKeySequence(Qt::Key_Escape));
    connect(closeAction, &QAction::triggered, this, &QWidget::close);
    addAction(closeAction);

    auto* saveAction = new QAction(tr("Save"), this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, [this] {
        canvas_->clearModified();
        emit imageEdited(canvas_->renderedImage());
        emit saveRequested();
        statusBar()->showMessage(tr("Saved"), 3000);
    });
    addAction(saveAction);

    auto* pinAction = new QAction(tr("Pin"), this);
    pinAction->setShortcut(QKeySequence(Qt::Key_F3));
    connect(pinAction, &QAction::triggered, this, [this] {
        auto img = canvas_->renderedImage();
        emit imageEdited(img);
        emit pinRequested(img);
        statusBar()->showMessage(tr("Image pinned"), 3000);
    });
    addAction(pinAction);
}

void EditorWindow::closeEvent(QCloseEvent* event)
{
    if (canvas_ && canvas_->isModified()) {
        auto ret = QMessageBox::question(this, tr("Unsaved Changes"),
            tr("You have unsaved annotations. Save before closing?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            emit imageEdited(canvas_->renderedImage());
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
    if (imageInfoLabel_) {
        imageInfoLabel_->setText(tr("Image: %1 x %2 px").arg(image.width()).arg(image.height()));
    }
    refreshPanelUi();
    show();
    raise();
    activateWindow();
}

void EditorWindow::refreshPanelUi()
{
    // Recent tools
    auto recent = canvas_->recentTools();
    for (int i = 0; i < recentToolBtns_.size(); ++i) {
        if (i < recent.size()) {
            recentToolBtns_[i]->setIcon(iconForTool(recent[i]));
            recentToolBtns_[i]->setToolTip([&]{
                switch (recent[i]) {
                    case AnnotationTool::Rectangle: return "Rect"; case AnnotationTool::Ellipse: return "Ellipse";
                    case AnnotationTool::Arrow: return "Arrow"; case AnnotationTool::Line: return "Line";
                    case AnnotationTool::Pen: return "Pen"; case AnnotationTool::Text: return "Text";
                    case AnnotationTool::Highlight: return "Highlight"; case AnnotationTool::Numbered: return "Numbered";
                    case AnnotationTool::Mosaic: return "Mosaic"; case AnnotationTool::Eraser: return "Eraser";
                    case AnnotationTool::Select: return "Select"; case AnnotationTool::Crop: return "Crop";
                    default: return "";
                }
            }());
            recentToolBtns_[i]->setVisible(true);
        } else {
            recentToolBtns_[i]->setVisible(false);
        }
    }
    // Undo/Redo counts
    if (undoBtn_) {
        QString text = undoBtn_->text();
        int uc = canvas_->undoCount();
        QString newText = uc > 0 ? tr("Undo (%1)").arg(uc) : tr("Undo");
        if (text != newText) undoBtn_->setText(newText);
    }
    if (redoBtn_) {
        QString text = redoBtn_->text();
        int rc = canvas_->redoCount();
        QString newText = rc > 0 ? tr("Redo (%1)").arg(rc) : tr("Redo");
        if (text != newText) redoBtn_->setText(newText);
    }
}

void EditorWindow::onToolChanged(AnnotationTool tool)
{
    if (updateToolActions_) {
        updateToolActions_(tool);
    }
    if (contextHint_) {
        static const char* hints[] = {
            QT_TRANSLATE_NOOP("EditorWindow", "Click or drag to select"),
            QT_TRANSLATE_NOOP("EditorWindow", "Drag to draw a rectangle"),
            QT_TRANSLATE_NOOP("EditorWindow", "Drag to draw an arrow"),
            QT_TRANSLATE_NOOP("EditorWindow", "Drag to draw a line"),
            QT_TRANSLATE_NOOP("EditorWindow", "Freehand drawing"),
            QT_TRANSLATE_NOOP("EditorWindow", "Click to place text"),
            QT_TRANSLATE_NOOP("EditorWindow", "Drag to apply mosaic blur"),
            QT_TRANSLATE_NOOP("EditorWindow", "Drag to draw an ellipse"),
            QT_TRANSLATE_NOOP("EditorWindow", "Drag to highlight an area"),
            QT_TRANSLATE_NOOP("EditorWindow", "Click or drag to erase"),
            QT_TRANSLATE_NOOP("EditorWindow", "Click to place numbered circle"),
            QT_TRANSLATE_NOOP("EditorWindow", "Drag crop handles to trim"),
        };
        auto idx = static_cast<int>(tool);
        if (idx >= 0 && idx < static_cast<int>(sizeof(hints)/sizeof(hints[0]))) {
            contextHint_->setText(tr(hints[idx]));
        }
    }
    refreshPanelUi();
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
    auto* dock = new QDockWidget(tr("Tools"), this);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setAllowedAreas(Qt::RightDockWidgetArea);
    dock->setTitleBarWidget(new QWidget());
    addDockWidget(Qt::RightDockWidgetArea, dock);

    auto* scrollArea = new QScrollArea(dock);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(scrollBarStyle);

    auto* content = new QWidget(scrollArea);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(0);

    // -- Styles --

    const QString toolStyle =
        "QToolButton { font: 9px 'Microsoft YaHei UI','Segoe UI',sans-serif;"
        "  padding: 4px 6px; text-align: left; border: none; border-radius: 4px; }"
        "QToolButton:hover { background: rgba(255,255,255,0.06); }"
        "QToolButton:checked { background: rgba(47,191,159,0.15); color: #2fbf9f; }";

    const QString selStyle =
        "QToolButton { font: bold 10px; color: #999; background: transparent;"
        "  border: none; border-radius: 4px; padding: 3px 5px; }"
        "QToolButton:hover { background: rgba(47,191,159,0.1); color: #2fbf9f; }"
        "QToolButton:checked { color: #fff; background: #2fbf9f; }"
        "QToolButton:hover:checked { background: #269d84; }";

    const QString chipStyle =
        "QToolButton { font: 9px; color: #999; background: rgba(255,255,255,0.04);"
        "  border: 1px solid rgba(255,255,255,0.08); border-radius: 4px; padding: 2px 4px; }"
        "QToolButton:hover { border-color: #2fbf9f; color: #2fbf9f; }"
        "QToolButton:checked { background: #2fbf9f; color: #fff; border-color: #2fbf9f; }";

    auto addHr = [&]() {
        auto* line = new QFrame(content);
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet("QFrame { color: rgba(255,255,255,0.06); max-height: 1px; }");
        layout->addWidget(line);
    };

    struct ToolDef { std::function<QIcon()> iconFn; const char* name; const char* tip; AnnotationTool tool; };

    auto toolTipText = [this](const char* key) { return tr(key); };
    QVector<QToolButton*> toolButtons;

    auto makeToolBtn = [&](const ToolDef& d) -> QToolButton* {
        auto* btn = new QToolButton(content);
        btn->setIcon(d.iconFn());
        btn->setText(tr(d.name));
        btn->setToolTip(tr(d.tip));
        btn->setCheckable(true);
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setIconSize(QSize(14, 14));
        btn->setStyleSheet(toolStyle);
        btn->setFixedHeight(24);
        btn->setProperty("tool", static_cast<int>(d.tool));
        return btn;
    };

    // ==========================================
    // Tools
    // ==========================================
    auto toolName = [this](const char* key) { return tr(key); };

    const ToolDef allTools[] = {
        {[]{ return iconForTool(AnnotationTool::Rectangle); }, QT_TRANSLATE_NOOP("EditorWindow", "Rect (R)"), QT_TRANSLATE_NOOP("EditorWindow", "Rectangle"), AnnotationTool::Rectangle},
        {[]{ return iconForTool(AnnotationTool::Ellipse); }, QT_TRANSLATE_NOOP("EditorWindow", "Ellipse (E)"), QT_TRANSLATE_NOOP("EditorWindow", "Ellipse"), AnnotationTool::Ellipse},
        {[]{ return iconForTool(AnnotationTool::Arrow); }, QT_TRANSLATE_NOOP("EditorWindow", "Arrow (A)"), QT_TRANSLATE_NOOP("EditorWindow", "Arrow"), AnnotationTool::Arrow},
        {[]{ return iconForTool(AnnotationTool::Line); }, QT_TRANSLATE_NOOP("EditorWindow", "Line (L)"), QT_TRANSLATE_NOOP("EditorWindow", "Line"), AnnotationTool::Line},
        {[]{ return iconForTool(AnnotationTool::Pen); }, QT_TRANSLATE_NOOP("EditorWindow", "Pen (P)"), QT_TRANSLATE_NOOP("EditorWindow", "Pen"), AnnotationTool::Pen},
        {[]{ return iconForTool(AnnotationTool::Text); }, QT_TRANSLATE_NOOP("EditorWindow", "Text (T)"), QT_TRANSLATE_NOOP("EditorWindow", "Text"), AnnotationTool::Text},
        {[]{ return iconForTool(AnnotationTool::Highlight); }, QT_TRANSLATE_NOOP("EditorWindow", "Hi (H)"), QT_TRANSLATE_NOOP("EditorWindow", "Highlight"), AnnotationTool::Highlight},
        {[]{ return iconForTool(AnnotationTool::Numbered); }, QT_TRANSLATE_NOOP("EditorWindow", "Num (N)"), QT_TRANSLATE_NOOP("EditorWindow", "Numbered"), AnnotationTool::Numbered},
        {[]{ return iconForTool(AnnotationTool::Mosaic); }, QT_TRANSLATE_NOOP("EditorWindow", "Mosaic (M)"), QT_TRANSLATE_NOOP("EditorWindow", "Mosaic"), AnnotationTool::Mosaic},
        {[]{ return iconForTool(AnnotationTool::Eraser); }, QT_TRANSLATE_NOOP("EditorWindow", "Eraser (X)"), QT_TRANSLATE_NOOP("EditorWindow", "Eraser"), AnnotationTool::Eraser},
        {[]{ return iconForTool(AnnotationTool::Select); }, QT_TRANSLATE_NOOP("EditorWindow", "Select (V)"), QT_TRANSLATE_NOOP("EditorWindow", "Select"), AnnotationTool::Select},
        {[]{ return iconForTool(AnnotationTool::Crop); }, QT_TRANSLATE_NOOP("EditorWindow", "Crop (C)"), QT_TRANSLATE_NOOP("EditorWindow", "Crop"), AnnotationTool::Crop},
    };

    auto* toolGrid = new QGridLayout();
    toolGrid->setSpacing(2);
    toolGrid->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < 12; ++i) {
        auto* btn = makeToolBtn(allTools[i]);
        toolGrid->addWidget(btn, i / 2, i % 2);
        toolButtons.push_back(btn);
    }

    // -- Recent tools --
    auto* recentRow = new QHBoxLayout();
    recentRow->setContentsMargins(0, 0, 0, 0);
    recentRow->setSpacing(2);
    QVector<QToolButton*> recentBtns;
    for (int i = 0; i < 4; ++i) {
        auto* btn = new QToolButton(content);
        btn->setFixedSize(22, 22);
        btn->setVisible(false);
        btn->setIconSize(QSize(14, 14));
        btn->setStyleSheet(
            "QToolButton { border: 1px solid rgba(255,255,255,0.08); border-radius: 3px; }"
            "QToolButton:hover { border-color: #2fbf9f; background: rgba(47,191,159,0.1); }");
        connect(btn, &QToolButton::clicked, this, [this, i] {
            auto recent = canvas_->recentTools();
            if (i < recent.size()) canvas_->setTool(recent[i]);
        });
        recentBtns.push_back(btn);
        recentRow->addWidget(btn);
    }
    recentToolBtns_ = recentBtns;
    layout->addLayout(recentRow);

    layout->addLayout(toolGrid);

    layout->addSpacing(8);
    addHr();
    layout->addSpacing(6);

    colorBtn_ = new QToolButton(content);
    colorBtn_->setObjectName("colorWell");
    colorBtn_->setFixedHeight(32);
    colorBtn_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    colorBtn_->setPopupMode(QToolButton::InstantPopup);
    colorBtn_->setToolTip(tr("Color"));
    colorBtn_->setStyleSheet(
        "QToolButton#colorWell { border: 1px solid rgba(255,255,255,0.1);"
        "  border-radius: 4px; padding: 2px; }"
        "QToolButton#colorWell:hover { border-color: rgba(255,255,255,0.25); }");
    updateColorWell(QColor("#ff3b30"));

    auto* colorMenu = new QMenu(colorBtn_);
    eyeAction_ = new QAction(IconProvider::icon(IconName::Edit), tr("Eyedropper"), colorMenu);
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
    layout->addWidget(colorBtn_);

    layout->addSpacing(4);

    // -- Color swatches + Stroke preview --
    auto* swatchRow = new QHBoxLayout();
    swatchRow->setContentsMargins(0, 0, 0, 0);
    swatchRow->setSpacing(2);
    const QColor fixedColors[] = {
        QColor("#ff3b30"), QColor("#ff9500"), QColor("#ffcc00"),
        QColor("#34c759"), QColor("#007aff"), QColor("#000000")
    };
    auto* preview = new StrokePreview(content);
    auto refreshPreview = [this, preview]() {
        preview->showStroke(canvas_->color(), canvas_->strokeWidth());
    };
    for (const auto& c : fixedColors) {
        auto* swatch = new QToolButton(content);
        QPixmap px(14, 14);
        px.fill(c);
        swatch->setIcon(QIcon(px));
        swatch->setIconSize(QSize(14, 14));
        swatch->setFixedSize(18, 18);
        swatch->setToolTip(c.name(QColor::HexRgb).toUpper());
        swatch->setStyleSheet(
            "QToolButton { border: 1px solid rgba(255,255,255,0.1); border-radius: 2px; padding: 0; }"
            "QToolButton:hover { border-color: #2fbf9f; }");
        connect(swatch, &QToolButton::clicked, this, [this, c, refreshPreview] {
            canvas_->setColor(c);
            updateColorWell(c);
            refreshPreview();
        });
        swatchRow->addWidget(swatch);
    }
    layout->addLayout(swatchRow);

    layout->addSpacing(4);

    struct StrokePreset { QString label; int width; };
    const StrokePreset strokes[] = {{"S", 2}, {"M", 4}, {"L", 8}};
    auto* strokeGroup = new QButtonGroup(content);
    strokeGroup->setExclusive(true);
    auto* widthRow = new QHBoxLayout();
    widthRow->setContentsMargins(0, 0, 0, 0);
    widthRow->setSpacing(4);
    for (const auto& s : strokes) {
        auto* btn = new QToolButton(content);
        btn->setText(s.label);
        btn->setToolTip(tr("Stroke: %1px").arg(s.width));
        btn->setFixedHeight(24);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setCheckable(true);
        btn->setStyleSheet(selStyle);
        if (s.width == 4) btn->setChecked(true);
        strokeGroup->addButton(btn);
        connect(btn, &QToolButton::clicked, this, [this, s, refreshPreview] {
            canvas_->setStrokeWidth(s.width);
            refreshPreview();
        });
        widthRow->addWidget(btn);
    }
    layout->addLayout(widthRow);

    layout->addSpacing(2);
    layout->addWidget(preview);
    layout->addSpacing(6);

    struct PropToggle { const char* text; const char* tip; bool defaultOn; void (AnnotationCanvas::*setter)(bool); };
    const PropToggle props[] = {
        {QT_TRANSLATE_NOOP("EditorWindow", "Outline"), QT_TRANSLATE_NOOP("EditorWindow", "Toggle text outline"), true, &AnnotationCanvas::setTextOutlineEnabled},
        {QT_TRANSLATE_NOOP("EditorWindow", "Fill"), QT_TRANSLATE_NOOP("EditorWindow", "Toggle fill for shapes"), false, &AnnotationCanvas::setFilled},
        {QT_TRANSLATE_NOOP("EditorWindow", "Blur"), QT_TRANSLATE_NOOP("EditorWindow", "Toggle mosaic blur mode"), false, &AnnotationCanvas::setMosaicBlurred},
        {QT_TRANSLATE_NOOP("EditorWindow", "Grid"), QT_TRANSLATE_NOOP("EditorWindow", "Toggle alignment grid"), false, &AnnotationCanvas::setGridEnabled},
    };

    auto propText = [this](const char* key) { return tr(key); };
    auto propTip = [this](const char* key) { return tr(key); };

    auto* chipRow = new QHBoxLayout();
    chipRow->setContentsMargins(0, 0, 0, 0);
    chipRow->setSpacing(4);
    for (const auto& p : props) {
        auto* btn = new QToolButton(content);
        btn->setText(tr(p.text));
        btn->setToolTip(tr(p.tip));
        btn->setFixedHeight(22);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setCheckable(true);
        btn->setChecked(p.defaultOn);
        btn->setStyleSheet(chipStyle);
        connect(btn, &QToolButton::clicked, this, [this, p](bool checked) {
            (canvas_->*p.setter)(checked);
        });
        chipRow->addWidget(btn);
    }
    layout->addLayout(chipRow);

    layout->addSpacing(4);

    const QString sliderLabelStyle = "color: #8e8e93; font: 9px; padding: 0;";
    const QString sliderValStyle = "color: #bcbec6; font: 9px; padding: 0;";
    const QString sliderGroove = "QSlider::groove:horizontal { height: 3px; background: rgba(255,255,255,0.08); border-radius: 1px; margin: 0; }";
    const QString sliderHandle = "QSlider::handle:horizontal { width: 10px; height: 10px; margin: -4px 0; background: #2fbf9f; border-radius: 5px; }";
    const QString sliderSub = "QSlider::sub-page:horizontal { background: #2fbf9f; border-radius: 1px; }";
    const QString sliderStyle = sliderGroove + sliderHandle + sliderSub;

    auto addSliderRow = [&](const char* label, int min, int max, int def,
                            std::function<void(int)> onChanged,
                            std::function<QString(int)> fmt) -> QSlider*
    {
        auto* row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(4);
        auto* lbl = new QLabel(label, content);
        lbl->setFixedWidth(34);
        lbl->setStyleSheet(sliderLabelStyle);
        auto* slider = new QSlider(Qt::Horizontal, content);
        slider->setRange(min, max);
        slider->setValue(def);
        slider->setFixedHeight(18);
        slider->setStyleSheet(sliderStyle);
        auto* val = new QLabel(fmt(def), content);
        val->setFixedWidth(30);
        val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        val->setStyleSheet(sliderValStyle);
        connect(slider, &QSlider::valueChanged, this, [val, onChanged, fmt](int v) {
            onChanged(v);
            val->setText(fmt(v));
        });
        row->addWidget(lbl);
        row->addWidget(slider);
        row->addWidget(val);
        layout->addLayout(row);
        return slider;
    };

    addSliderRow("Opacity", 0, 255, 255,
        [this](int v) { canvas_->setStrokeAlpha(v); },
        [](int v) { return QString("%1%").arg(v * 100 / 255); });

    // Arrow style
    auto* arrowRow = new QHBoxLayout();
    arrowRow->setContentsMargins(0, 0, 0, 0);
    arrowRow->setSpacing(4);
    auto* arrowLbl = new QLabel(tr("Arrow"), content);
    arrowLbl->setFixedWidth(34);
    arrowLbl->setStyleSheet(sliderLabelStyle);
    arrowRow->addWidget(arrowLbl);
    struct ArrowDef { const char* text; int value; };
    const ArrowDef arrowDefs[] = {{"Tri", 0}, {"Circle", 1}, {"Square", 2}};
    auto* arrowGroup = new QButtonGroup(content);
    arrowGroup->setExclusive(true);
    for (const auto& ad : arrowDefs) {
        auto* btn = new QToolButton(content);
        btn->setText(ad.text);
        btn->setCheckable(true);
        btn->setFixedHeight(22);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setStyleSheet(chipStyle);
        if (ad.value == 0) btn->setChecked(true);
        arrowGroup->addButton(btn, ad.value);
        connect(btn, &QToolButton::clicked, this, [this, val = ad.value] {
            canvas_->setArrowStyle(static_cast<ArrowStyle>(val));
        });
        arrowRow->addWidget(btn);
    }
    layout->addLayout(arrowRow);

    addSliderRow("Radius", 0, 40, 0,
        [this](int v) { canvas_->setCornerRadius(v); },
        [](int v) { return v > 0 ? QString("%1px").arg(v) : "Off"; });

    layout->addSpacing(4);

    auto* fontRow = new QHBoxLayout();
    fontRow->setContentsMargins(0, 0, 0, 0);
    fontRow->setSpacing(4);

    auto* fontLabel = new QLabel(tr("Font"), content);
    fontLabel->setFixedWidth(34);
    fontLabel->setStyleSheet("color: #8e8e93; font: 9px; padding: 0;");

    auto* fontSizeDec = new QToolButton(content);
    fontSizeDec->setText("-");
    fontSizeDec->setToolTip(tr("Decrease font size ( [ )"));
    fontSizeDec->setFixedSize(24, 24);
    fontSizeDec->setStyleSheet(selStyle);

    auto* fontSizeVal = new QLabel("14px", content);
    fontSizeVal->setToolTip(tr("Font size for Text / Numbered tools"));
    fontSizeVal->setAlignment(Qt::AlignCenter);
    fontSizeVal->setStyleSheet("color: #bcbec6; font: 10px; padding: 0; background: transparent;");
    fontSizeVal->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* fontSizeInc = new QToolButton(content);
    fontSizeInc->setText("+");
    fontSizeInc->setToolTip(tr("Increase font size ( ] )"));
    fontSizeInc->setFixedSize(24, 24);
    fontSizeInc->setStyleSheet(selStyle);

    fontRow->addWidget(fontLabel);
    fontRow->addWidget(fontSizeDec);
    fontRow->addWidget(fontSizeVal);
    fontRow->addWidget(fontSizeInc);
    layout->addLayout(fontRow);

    connect(fontSizeDec, &QToolButton::clicked, this, [this] {
        canvas_->setFontSize(canvas_->fontSize() - 2);
    });
    connect(fontSizeInc, &QToolButton::clicked, this, [this] {
        canvas_->setFontSize(canvas_->fontSize() + 2);
    });
    canvas_->setOnFontSizeChanged([fontSizeVal](int size) {
        fontSizeVal->setText(QString("%1px").arg(size));
    });

    layout->addSpacing(6);

    // ==========================================
    // Zoom
    // ==========================================
    auto* zoomRow = new QHBoxLayout();
    zoomRow->setContentsMargins(0, 0, 0, 0);
    zoomRow->setSpacing(4);

    auto* zoomLabel = new QLabel(tr("Zoom"), content);
    zoomLabel->setFixedWidth(34);
    zoomLabel->setStyleSheet("color: #8e8e93; font: 9px; padding: 0;");

    auto* zoomOut = new QToolButton(content);
    zoomOut->setText("-");
    zoomOut->setToolTip(tr("Zoom out"));
    zoomOut->setFixedSize(24, 24);
    zoomOut->setStyleSheet(selStyle);

    auto* zoomVal = new QLabel("100%", content);
    zoomVal->setAlignment(Qt::AlignCenter);
    zoomVal->setStyleSheet("color: #bcbec6; font: 10px; padding: 0; background: transparent;");
    zoomVal->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* zoomIn = new QToolButton(content);
    zoomIn->setText("+");
    zoomIn->setToolTip(tr("Zoom in"));
    zoomIn->setFixedSize(24, 24);
    zoomIn->setStyleSheet(selStyle);

    auto* zoomReset = new QToolButton(content);
    zoomReset->setText("1:1");
    zoomReset->setToolTip(tr("Reset zoom to 100%"));
    zoomReset->setFixedSize(32, 24);
    zoomReset->setStyleSheet(
        "QToolButton { font: 8px; color: #999; background: transparent;"
        "  border: 1px solid rgba(255,255,255,0.08); border-radius: 4px; padding: 2px 4px; }"
        "QToolButton:hover { border-color: #2fbf9f; color: #2fbf9f; }");

    connect(zoomOut, &QToolButton::clicked, this, [this, zoomVal] {
        auto factor = canvas_->zoomFactor();
        canvas_->zoomAt(factor / 1.2, QPoint(canvas_->width() / 2, canvas_->height() / 2));
        zoomVal->setText(QString("%1%").arg(static_cast<int>(canvas_->zoomFactor() * 100)));
    });
    connect(zoomIn, &QToolButton::clicked, this, [this, zoomVal] {
        auto factor = canvas_->zoomFactor();
        canvas_->zoomAt(factor * 1.2, QPoint(canvas_->width() / 2, canvas_->height() / 2));
        zoomVal->setText(QString("%1%").arg(static_cast<int>(canvas_->zoomFactor() * 100)));
    });
    connect(zoomReset, &QToolButton::clicked, this, [this, zoomVal] {
        canvas_->zoomAt(1.0, QPoint(canvas_->width() / 2, canvas_->height() / 2));
        zoomVal->setText("100%");
    });
    canvas_->setOnZoomChanged([zoomVal](double factor) {
        zoomVal->setText(QString("%1%").arg(static_cast<int>(factor * 100)));
    });

    zoomRow->addWidget(zoomLabel);
    zoomRow->addWidget(zoomOut);
    zoomRow->addWidget(zoomVal);
    zoomRow->addWidget(zoomIn);
    zoomRow->addWidget(zoomReset);
    layout->addLayout(zoomRow);

    layout->addSpacing(6);

    // -- Info --
    contextHint_ = new QLabel(content);
    contextHint_->setStyleSheet("color: #8e8e93; font: 9px; padding: 0 2px;");
    contextHint_->setWordWrap(true);
    layout->addWidget(contextHint_);

    imageInfoLabel_ = new QLabel(content);
    imageInfoLabel_->setStyleSheet("color: #5e5e63; font: 8px; padding: 0 2px;");
    layout->addWidget(imageInfoLabel_);

    pixelInfoLabel_ = new QLabel(content);
    pixelInfoLabel_->setStyleSheet("color: #5e5e63; font: 8px; padding: 0 2px;");
    layout->addWidget(pixelInfoLabel_);

    layout->addSpacing(4);

    // ==========================================
    // Actions (2-column grid)
    // ==========================================
    struct ActionDef { QIcon icon; const char* text; const char* tip; QToolButton** ptr; };
    undoBtn_ = nullptr; redoBtn_ = nullptr;
    const ActionDef actionDefs[] = {
        {IconProvider::icon(IconName::Undo), QT_TRANSLATE_NOOP("EditorWindow", "Undo"), QT_TRANSLATE_NOOP("EditorWindow", "Undo (Ctrl+Z)"), &undoBtn_},
        {IconProvider::icon(IconName::Redo), QT_TRANSLATE_NOOP("EditorWindow", "Redo"), QT_TRANSLATE_NOOP("EditorWindow", "Redo (Ctrl+Y)"), &redoBtn_},
        {IconProvider::icon(IconName::Copy), QT_TRANSLATE_NOOP("EditorWindow", "Copy"), QT_TRANSLATE_NOOP("EditorWindow", "Copy (Ctrl+Shift+C)"), nullptr},
        {IconProvider::icon(IconName::Pin), QT_TRANSLATE_NOOP("EditorWindow", "Pin"), QT_TRANSLATE_NOOP("EditorWindow", "Pin (F3)"), nullptr},
        {IconProvider::icon(IconName::Save), QT_TRANSLATE_NOOP("EditorWindow", "Save"), QT_TRANSLATE_NOOP("EditorWindow", "Save (Ctrl+S)"), nullptr},
        {IconProvider::icon(IconName::Export), QT_TRANSLATE_NOOP("EditorWindow", "Export..."), QT_TRANSLATE_NOOP("EditorWindow", "Export (Ctrl+Shift+S)"), nullptr},
    };

    auto* actionsGrid = new QGridLayout();
    actionsGrid->setContentsMargins(0, 0, 0, 0);
    actionsGrid->setSpacing(2);

    QVector<QToolButton*> actionButtons;
    actionButtons.reserve(std::size(actionDefs));
    for (int i = 0; i < std::size(actionDefs); ++i) {
        auto* btn = new QToolButton(content);
        btn->setIcon(actionDefs[i].icon);
        btn->setText(tr(actionDefs[i].text));
        btn->setToolTip(tr(actionDefs[i].tip));
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setIconSize(QSize(14, 14));
        btn->setStyleSheet(toolStyle);
        btn->setFixedHeight(24);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        actionsGrid->addWidget(btn, i / 2, i % 2);
        actionButtons.push_back(btn);
        if (actionDefs[i].ptr) *actionDefs[i].ptr = btn;
    }
    layout->addLayout(actionsGrid);

    // -- updateToolActions callback --
    updateToolActions_ = [toolButtons](AnnotationTool tool) {
        for (auto* btn : toolButtons)
            btn->setChecked(static_cast<AnnotationTool>(btn->property("tool").toInt()) == tool);
    };
    updateToolActions_(AnnotationTool::Rectangle);
    refreshPanelUi();

    // -- Connections --
    connect(actionButtons[0], &QToolButton::clicked, this, [this] { canvas_->undo(); refreshPanelUi(); });
    connect(actionButtons[1], &QToolButton::clicked, this, [this] { canvas_->redo(); refreshPanelUi(); });
    for (auto* btn : toolButtons) {
        connect(btn, &QToolButton::clicked, this, [this, tool = static_cast<AnnotationTool>(btn->property("tool").toInt())] {
            canvas_->setTool(tool);
        });
    }

    // -- Pixel info --
    canvas_->setOnMouseInfoChanged([this](QPointF pos, QColor c) {
        if (pixelInfoLabel_)
            pixelInfoLabel_->setText(QString("(%1, %2) %3")
                .arg(static_cast<int>(pos.x())).arg(static_cast<int>(pos.y()))
                .arg(c.name(QColor::HexRgb).toUpper()));
    });

    // -- State change (undo/redo counts etc.) --
    canvas_->setOnModified([this] { refreshPanelUi(); });

    connect(actionButtons[2], &QToolButton::clicked, this, [this] {
        emit imageEdited(canvas_->renderedImage());
        emit copyRequested();
        statusBar()->showMessage(tr("Copied to clipboard"), 3000);
    });
    connect(actionButtons[3], &QToolButton::clicked, this, [this] {
        auto img = canvas_->renderedImage();
        emit imageEdited(img);
        emit pinRequested(img);
        statusBar()->showMessage(tr("Image pinned"), 3000);
    });
    connect(actionButtons[4], &QToolButton::clicked, this, [this] {
        canvas_->clearModified();
        emit imageEdited(canvas_->renderedImage());
        emit saveRequested();
        statusBar()->showMessage(tr("Saved"), 3000);
    });
    connect(actionButtons[5], &QToolButton::clicked, this, [this] {
        auto path = QFileDialog::getSaveFileName(this, tr("Save As"), QString(),
            "PNG (*.png);;JPEG (*.jpg *.jpeg)");
        if (!path.isEmpty()) {
            if (canvas_->renderedImage().save(path)) {
                statusBar()->showMessage(tr("Saved to %1").arg(path), 5000);
            } else {
                statusBar()->showMessage(tr("Failed to save image"), 5000);
            }
        }
    });

    scrollArea->setWidget(content);
    dock->setWidget(scrollArea);
}

} // namespace snappaste
