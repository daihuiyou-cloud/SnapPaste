#include "presentation/editor/EditorWindow.h"

#include <algorithm>
#include <cmath>

#include "presentation/editor/AnnotationCanvas.h"
#include "presentation/editor/EditorIconFactory.h"

#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QShortcut>
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
    background: rgba(128,128,128,0.25); min-height: 28px; border-radius: 3px;
}
QScrollBar::handle:vertical:hover { background: rgba(128,128,128,0.40); }
QScrollBar::handle:vertical:pressed { background: rgba(128,128,128,0.55); }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal {
    background: transparent; height: 7px; margin: 0;
}
QScrollBar::handle:horizontal {
    background: rgba(128,128,128,0.25); min-width: 28px; border-radius: 3px;
}
QScrollBar::handle:horizontal:hover { background: rgba(128,128,128,0.40); }
QScrollBar::handle:horizontal:pressed { background: rgba(128,128,128,0.55); }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }
QScrollBar::corner { background: transparent; }
)";

const char* toolLabel(AnnotationTool tool)
{
    switch (tool) {
    case AnnotationTool::Select:    return QT_TRANSLATE_NOOP("EditorWindow", "Select");
    case AnnotationTool::Rectangle: return QT_TRANSLATE_NOOP("EditorWindow", "Rect");
    case AnnotationTool::Arrow:     return QT_TRANSLATE_NOOP("EditorWindow", "Arrow");
    case AnnotationTool::Line:      return QT_TRANSLATE_NOOP("EditorWindow", "Line");
    case AnnotationTool::Pen:       return QT_TRANSLATE_NOOP("EditorWindow", "Pen");
    case AnnotationTool::Text:      return QT_TRANSLATE_NOOP("EditorWindow", "Text");
    case AnnotationTool::Mosaic:    return QT_TRANSLATE_NOOP("EditorWindow", "Mosaic");
    case AnnotationTool::Ellipse:   return QT_TRANSLATE_NOOP("EditorWindow", "Ellipse");
    case AnnotationTool::Highlight: return QT_TRANSLATE_NOOP("EditorWindow", "Hi");
    case AnnotationTool::Eraser:    return QT_TRANSLATE_NOOP("EditorWindow", "Eraser");
    case AnnotationTool::Numbered:  return QT_TRANSLATE_NOOP("EditorWindow", "Num");
    case AnnotationTool::Crop:      return QT_TRANSLATE_NOOP("EditorWindow", "Crop");
    }
    return "";
}

} // namespace

namespace {

const char* kToolStyle =
    "QToolButton { font: 9px;"
    "  font-family: 'Microsoft YaHei UI','Segoe UI',sans-serif;"
    "  padding: 4px 6px; text-align: left; border: none; border-radius: 4px; }"
    "QToolButton:hover { background: rgba(255,255,255,0.06); }"
    "QToolButton:pressed { background: rgba(47,191,159,0.12); }"
    "QToolButton:checked { background: rgba(47,191,159,0.15); color: #2fbf9f; }";

const char* kSelStyle =
    "QToolButton { font: bold 10px; color: #999; background: transparent;"
    "  border: none; border-radius: 4px; padding: 3px 5px; }"
    "QToolButton:hover { background: rgba(47,191,159,0.1); color: #2fbf9f; }"
    "QToolButton:pressed { background: rgba(47,191,159,0.2); }"
    "QToolButton:checked { color: #fff; background: #2fbf9f; }"
    "QToolButton:hover:checked { background: #269d84; }";

const char* kChipStyle =
    "QToolButton { font: 9px; color: #999; background: rgba(255,255,255,0.04);"
    "  border: 1px solid rgba(255,255,255,0.08); border-radius: 4px; padding: 2px 4px; }"
    "QToolButton:hover { border-color: #2fbf9f; color: #2fbf9f; }"
    "QToolButton:pressed { background: rgba(47,191,159,0.25); }"
    "QToolButton:checked { background: #2fbf9f; color: #fff; border-color: #2fbf9f; }";

const char* kSliderLabelStyle = "color: #8e8e93; font: 10px; padding: 0;";
const char* kSliderValStyle = "color: #bcbec6; font: 10px; padding: 0;";
const char* kSliderGroove = "QSlider::groove:horizontal { height: 3px; background: rgba(255,255,255,0.08); border-radius: 1px; margin: 0; }";
const char* kSliderHandle = "QSlider::handle:horizontal { width: 10px; height: 10px; margin: -4px 0; background: #2fbf9f; border-radius: 5px; }";
const char* kSliderSub = "QSlider::sub-page:horizontal { background: #2fbf9f; border-radius: 1px; }";
const QString kSliderStyle = QString(kSliderGroove) + kSliderHandle + kSliderSub;

const char* kZoomBtnStyle =
    "QToolButton { font: 8px; color: #999; background: transparent;"
    "  border: 1px solid rgba(255,255,255,0.08); border-radius: 4px; padding: 2px 4px; }"
    "QToolButton:hover { border-color: #2fbf9f; color: #2fbf9f; }"
    "QToolButton:pressed { background: rgba(255,255,255,0.1); }";

const char* kSmallBtnStyle =
    "QToolButton { font: 9px; color: #999; background: rgba(255,255,255,0.04);"
    "  border: 1px solid rgba(255,255,255,0.08); border-radius: 4px; padding: 2px 4px; }"
    "QToolButton:hover { border-color: #2fbf9f; color: #2fbf9f; }"
    "QToolButton:pressed { background: rgba(255,255,255,0.1); }";

const QColor kFixedColors[] = {
    QColor("#ff3b30"), QColor("#ff9500"), QColor("#ffcc00"),
    QColor("#34c759"), QColor("#007aff"), QColor("#af52de"),
    QColor("#ffffff"), QColor("#000000")
};

} // namespace


EditorWindow::EditorWindow(IIconProvider& iconProvider, QWidget* parent)
    : QMainWindow(parent)
    , iconProvider_(iconProvider)
    , canvas_(new AnnotationCanvas(this))
{
    setWindowTitle(tr("SnapPaste Editor"));
    resize(980, 680);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(canvas_);
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setStyleSheet(scrollBarStyle);
    setCentralWidget(scrollArea);

    createToolPanel();
    connect(canvas_, &AnnotationCanvas::toolChanged, this, &EditorWindow::onToolChanged);
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
        auto img = QApplication::clipboard()->image();
        if (!img.isNull()) {
            setImage(img);
        }
    });
    addAction(pasteAction);

    auto* saveAsAction = new QAction(tr("Export"), this);
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
    connect(saveAsAction, &QAction::triggered, this, [this] {
        QSettings settings;
        auto dir = settings.value("editor/lastSaveDir").toString();
        auto path = QFileDialog::getSaveFileName(this, tr("Export"), dir,
            tr("PNG (*.png);;JPEG (*.jpg *.jpeg)"));
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
            canvas_->clearModified();
        } else if (ret == QMessageBox::Discard) {
        } else {
            event->ignore();
            return;
        }
    }
    if (canvas_) {
        QSettings().setValue("editor/zoomFactor", canvas_->zoomFactor());
    }
    event->accept();
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
    if (canvas_->selectedIndex() < 0)
        syncPanelDefaults();
}

void EditorWindow::rebuildLayerList()
{
    if (!layerList_) return;
    rebuildingLayerList_ = true;
    layerList_->clear();

    int n = canvas_->annotationCount();
    for (int i = n - 1; i >= 0; --i) {
        const auto& a = canvas_->annotationAt(i);
        QString label = qApp->translate("EditorWindow", toolLabel(a.tool));
        if (!a.text.isEmpty()) {
            QString t = a.text.left(20);
            t.replace('\n', ' ');
            label += QStringLiteral(" - ") + t;
        } else if (a.tool == AnnotationTool::Numbered) {
            label += QStringLiteral(" #%1").arg(a.number);
        }
        auto* item = new QListWidgetItem(label, layerList_);
        item->setData(Qt::UserRole, i);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(a.visible ? Qt::Checked : Qt::Unchecked);
    }

    // Restore selection
    int sel = canvas_->selectedIndex();
    if (sel >= 0) {
        int row = n - 1 - sel;
        layerList_->setCurrentRow(row);
        layerList_->scrollToItem(layerList_->item(row));
    } else {
        layerList_->setCurrentRow(-1);
    }
    rebuildingLayerList_ = false;
}

void EditorWindow::syncPanelDefaults()
{
    if (!propsWidget_) return;
    updateColorWell(canvas_->color());
    updateFillColorWell(canvas_->fillColor());
    if (preview_)
        static_cast<StrokePreview*>(preview_)->showStroke(canvas_->color(), canvas_->strokeWidth());
    if (auto* sg = findChild<QButtonGroup*>("strokeGroup")) {
        for (auto* btn : sg->buttons())
            btn->setChecked(btn->property("width").toInt() == canvas_->strokeWidth());
    }
    for (auto* btn : propsWidget_->findChildren<QToolButton*>()) {
        auto key = btn->property("chipKey").toByteArray();
        if (key == "Fill")
            btn->setChecked(canvas_->filled());
        else if (key == "Outline")
            btn->setChecked(canvas_->textOutlineEnabled());
        else if (key == "Blur")
            btn->setChecked(canvas_->mosaicBlurred());
        else if (key == "Grid")
            btn->setChecked(canvas_->gridEnabled());
    }
    if (auto* rs = findChild<QSlider*>("radiusSlider")) {
        int v = canvas_->cornerRadius();
        rs->setValue(v);
        if (auto* rv = findChild<QLabel*>("radiusVal"))
            rv->setText(v > 0 ? tr("%1px").arg(v) : tr("Off"));
    }
    if (auto* ag = findChild<QButtonGroup*>("arrowGroup")) {
        if (auto* abtn = ag->button(static_cast<int>(canvas_->arrowStyle())))
            abtn->setChecked(true);
    }
    canvas_->syncTextPropertiesUI();
}

void EditorWindow::onToolChanged(AnnotationTool tool)
{
    if (updateToolActions_) {
        updateToolActions_(tool);
    }
    if (contextHint_) {
        QString hint;
        switch (tool) {
            case AnnotationTool::Select:    hint = tr("Click or drag to select"); break;
            case AnnotationTool::Rectangle: hint = tr("Drag to draw a rectangle"); break;
            case AnnotationTool::Arrow:     hint = tr("Drag to draw an arrow"); break;
            case AnnotationTool::Line:      hint = tr("Drag to draw a line"); break;
            case AnnotationTool::Pen:       hint = tr("Freehand drawing"); break;
            case AnnotationTool::Text:      hint = tr("Click to place text"); break;
            case AnnotationTool::Mosaic:    hint = tr("Drag to apply mosaic blur"); break;
            case AnnotationTool::Ellipse:   hint = tr("Drag to draw an ellipse"); break;
            case AnnotationTool::Highlight: hint = tr("Drag to highlight an area"); break;
            case AnnotationTool::Eraser:    hint = tr("Click or drag to erase"); break;
            case AnnotationTool::Numbered:  hint = tr("Click to place numbered circle"); break;
            case AnnotationTool::Crop:      hint = tr("Drag crop handles to trim"); break;
        }
        contextHint_->setText(hint);
    }
    if (propsWidget_) {
        bool anyVisible = false;
        for (auto* chip : propsWidget_->findChildren<QToolButton*>()) {
            auto key = chip->property("chipKey").toByteArray();
            bool visible = true;
            if (key == "Fill")
                visible = (tool == AnnotationTool::Rectangle || tool == AnnotationTool::Ellipse || tool == AnnotationTool::Arrow);
            else if (key == "Outline")
                visible = (tool == AnnotationTool::Text || tool == AnnotationTool::Numbered);
            else if (key == "Blur")
                visible = (tool == AnnotationTool::Mosaic);
            chip->setVisible(visible);
            if (visible) anyVisible = true;
        }
        propsWidget_->setVisible(anyVisible);
    }
    if (arrowWidget_) {
        arrowWidget_->setVisible(tool == AnnotationTool::Arrow);
    }
    if (radiusWidget_) {
        radiusWidget_->setVisible(tool == AnnotationTool::Rectangle);
    }
    if (fontWidget_) {
        bool show = tool == AnnotationTool::Text || tool == AnnotationTool::Numbered;
        fontWidget_->setVisible(show);
    }
    if (cropWidget_) {
        bool show = tool == AnnotationTool::Crop;
        cropWidget_->setVisible(show);
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

void EditorWindow::updateFillColorWell(const QColor& c)
{
    QPixmap px(18, 18);
    px.fill(c.isValid() ? c : QColor(0, 0, 0, 0));
    QPainter p(&px);
    p.setPen(QPen(QColor(255, 255, 255, 48), 1));
    if (c.isValid()) {
        p.fillRect(QRectF(0.5, 0.5, 17, 17), c);
        p.drawRect(QRectF(0.5, 0.5, 17, 17));
    } else {
        p.setPen(QPen(QColor(255, 255, 255, 32), 1));
        p.drawLine(1, 1, 16, 16);
        p.drawLine(16, 1, 1, 16);
        p.drawRect(QRectF(0.5, 0.5, 17, 17));
    }
    p.end();
    fillColorBtn_->setIcon(QIcon(px));
}

void EditorWindow::rebuildColorMenu()
{
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
    for (const auto& c : kFixedColors) {
        auto* a = new QAction(makeColorIcon(c), c.name(QColor::HexRgb).toUpper(), colorBtn_->menu());
        colorBtn_->menu()->insertAction(eyeAction_, a);
        connect(a, &QAction::triggered, this, [this, c] {
            canvas_->setColor(c);
            updateColorWell(c);
        });
    }
    colorBtn_->menu()->insertSeparator(eyeAction_);
    auto* customAction = new QAction(tr("Custom Color..."), colorBtn_->menu());
    colorBtn_->menu()->insertAction(eyeAction_, customAction);
    connect(customAction, &QAction::triggered, this, [this] {
        auto color = QColorDialog::getColor(Qt::white, this, tr("Choose Color"));
        if (color.isValid()) {
            canvas_->setColor(color);
            canvas_->addRecentColor(color);
            updateColorWell(color);
        }
    });
    colorBtn_->menu()->insertSeparator(eyeAction_);
}

void EditorWindow::buildToolSection(QVBoxLayout* layout, QWidget* content, const QString& toolStyle, QVector<QToolButton*>& toolButtons)
{
    struct ToolDef { std::function<QIcon()> iconFn; const char* name; const char* tip; AnnotationTool tool; };

    auto makeToolBtn = [this, content, &toolStyle](const ToolDef& d) -> QToolButton* {
        auto* btn = new QToolButton(content);
        btn->setIcon(d.iconFn());
        btn->setText(QCoreApplication::translate("EditorWindow", d.name));
        btn->setToolTip(QCoreApplication::translate("EditorWindow", d.tip));
        btn->setCheckable(true);
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setIconSize(QSize(14, 14));
        btn->setStyleSheet(toolStyle);
        btn->setFixedHeight(24);
        btn->setProperty("tool", static_cast<int>(d.tool));
        return btn;
    };

    const ToolDef allTools[] = {
        {[this]{ return iconForTool(AnnotationTool::Rectangle, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Rect (R)"), QT_TRANSLATE_NOOP("EditorWindow", "Rectangle"), AnnotationTool::Rectangle},
        {[this]{ return iconForTool(AnnotationTool::Ellipse, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Ellipse (E)"), QT_TRANSLATE_NOOP("EditorWindow", "Ellipse"), AnnotationTool::Ellipse},
        {[this]{ return iconForTool(AnnotationTool::Arrow, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Arrow (A)"), QT_TRANSLATE_NOOP("EditorWindow", "Arrow"), AnnotationTool::Arrow},
        {[this]{ return iconForTool(AnnotationTool::Line, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Line (L)"), QT_TRANSLATE_NOOP("EditorWindow", "Line"), AnnotationTool::Line},
        {[this]{ return iconForTool(AnnotationTool::Pen, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Pen (P)"), QT_TRANSLATE_NOOP("EditorWindow", "Pen"), AnnotationTool::Pen},
        {[this]{ return iconForTool(AnnotationTool::Text, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Text (T)"), QT_TRANSLATE_NOOP("EditorWindow", "Text"), AnnotationTool::Text},
        {[this]{ return iconForTool(AnnotationTool::Highlight, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Hi (H)"), QT_TRANSLATE_NOOP("EditorWindow", "Highlight"), AnnotationTool::Highlight},
        {[this]{ return iconForTool(AnnotationTool::Numbered, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Num (N)"), QT_TRANSLATE_NOOP("EditorWindow", "Numbered"), AnnotationTool::Numbered},
        {[this]{ return iconForTool(AnnotationTool::Mosaic, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Mosaic (M)"), QT_TRANSLATE_NOOP("EditorWindow", "Mosaic"), AnnotationTool::Mosaic},
        {[this]{ return iconForTool(AnnotationTool::Eraser, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Eraser (X)"), QT_TRANSLATE_NOOP("EditorWindow", "Eraser"), AnnotationTool::Eraser},
        {[this]{ return iconForTool(AnnotationTool::Select, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Select (V)"), QT_TRANSLATE_NOOP("EditorWindow", "Select"), AnnotationTool::Select},
        {[this]{ return iconForTool(AnnotationTool::Crop, iconProvider_); }, QT_TRANSLATE_NOOP("EditorWindow", "Crop (C)"), QT_TRANSLATE_NOOP("EditorWindow", "Crop"), AnnotationTool::Crop},
    };

    auto* toolGrid = new QGridLayout();
    toolGrid->setSpacing(2);
    toolGrid->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < std::size(allTools); ++i) {
        auto* btn = makeToolBtn(allTools[i]);
        toolGrid->addWidget(btn, i / 2, i % 2);
        toolButtons.push_back(btn);
    }
    layout->addLayout(toolGrid);

    for (auto* btn : toolButtons) {
        connect(btn, &QToolButton::clicked, this, [this, tool = static_cast<AnnotationTool>(btn->property("tool").toInt())] {
            canvas_->setTool(tool);
            canvas_->setFocus();
        });
    }
}

void EditorWindow::buildActionSection(QVBoxLayout* layout, QWidget* content, const QString& toolStyle, QVector<QToolButton*>& actionButtons)
{
    struct ActionDef { QIcon icon; const char* text; const char* tip; QToolButton** ptr; };
    undoBtn_ = redoBtn_ = copyBtn_ = pinBtn_ = saveBtn_ = exportBtn_ = nullptr;
    const ActionDef actionDefs[] = {
        {iconProvider_.icon(IconName::Undo), QT_TRANSLATE_NOOP("EditorWindow", "Undo"), QT_TRANSLATE_NOOP("EditorWindow", "Undo (Ctrl+Z)"), &undoBtn_},
        {iconProvider_.icon(IconName::Redo), QT_TRANSLATE_NOOP("EditorWindow", "Redo"), QT_TRANSLATE_NOOP("EditorWindow", "Redo (Ctrl+Y)"), &redoBtn_},
        {iconProvider_.icon(IconName::Copy), QT_TRANSLATE_NOOP("EditorWindow", "Copy"), QT_TRANSLATE_NOOP("EditorWindow", "Copy (Ctrl+Shift+C)"), &copyBtn_},
        {iconProvider_.icon(IconName::Pin), QT_TRANSLATE_NOOP("EditorWindow", "Pin"), QT_TRANSLATE_NOOP("EditorWindow", "Pin (F3)"), &pinBtn_},
        {iconProvider_.icon(IconName::Save), QT_TRANSLATE_NOOP("EditorWindow", "Save"), QT_TRANSLATE_NOOP("EditorWindow", "Save (Ctrl+S)"), &saveBtn_},
        {iconProvider_.icon(IconName::Export), QT_TRANSLATE_NOOP("EditorWindow", "Export..."), QT_TRANSLATE_NOOP("EditorWindow", "Export (Ctrl+Shift+S)"), &exportBtn_},
    };

    auto* actionsGrid = new QGridLayout();
    actionsGrid->setContentsMargins(0, 0, 0, 0);
    actionsGrid->setSpacing(2);

    actionButtons.reserve(std::size(actionDefs));
    for (int i = 0; i < std::size(actionDefs); ++i) {
        auto* btn = new QToolButton(content);
        btn->setIcon(actionDefs[i].icon);
        btn->setText(QCoreApplication::translate("EditorWindow", actionDefs[i].text));
        btn->setToolTip(QCoreApplication::translate("EditorWindow", actionDefs[i].tip));
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
    layout->setContentsMargins(10, 12, 10, 12);
    layout->setSpacing(0);

    QVector<QToolButton*> toolButtons;
    buildToolSection(layout, content, kToolStyle, toolButtons);

    layout->addSpacing(10);
    {
        auto* line = new QFrame(content);
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet("QFrame { background: rgba(255,255,255,0.08); max-height: 1px; }");
        layout->addWidget(line);
    }
    layout->addSpacing(10);

    buildColorSection(layout, content);
    buildStrokeSection(layout, content);
    buildArrowSection(layout, content);
    buildFontSection(layout, content);
    buildCropSection(layout, content);

    layout->addSpacing(10);

    buildZoomSection(layout, content);
    buildImageAdjustSection(layout, content);
    buildTransformSection(layout, content);
    buildLayerSection(layout, content);
    buildInfoSection(layout, content);

    QVector<QToolButton*> actionButtons;
    buildActionSection(layout, content, kToolStyle, actionButtons);

    // -- updateToolActions callback --
    updateToolActions_ = [toolButtons](AnnotationTool tool) {
        for (auto* btn : toolButtons)
            btn->setChecked(static_cast<AnnotationTool>(btn->property("tool").toInt()) == tool);
    };
    updateToolActions_(AnnotationTool::Rectangle);
    refreshPanelUi();

    wireToolPanelConnections();

    scrollArea->setWidget(content);
    dock->setWidget(scrollArea);
}

// ============================================================
// Tool panel sub-builders
// ============================================================

void EditorWindow::buildColorSection(QVBoxLayout* layout, QWidget* content)
{
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
    eyeAction_ = new QAction(iconProvider_.icon(IconName::Edit), tr("Eyedropper"), colorMenu);
    eyeAction_->setCheckable(true);
    connect(eyeAction_, &QAction::triggered, this, [this] {
        canvas_->setPickingColor(eyeAction_->isChecked());
    });
    connect(canvas_, &AnnotationCanvas::pickingColorChanged, this, [this](bool picking) {
        eyeAction_->setChecked(picking);
        if (!picking)
            updateColorWell(canvas_->color());
    });
    colorMenu->addAction(eyeAction_);
    connect(colorMenu, &QMenu::aboutToShow, this, &EditorWindow::rebuildColorMenu);
    colorBtn_->setMenu(colorMenu);
    layout->addWidget(colorBtn_);

    // Fill color
    fillColorBtn_ = new QToolButton(content);
    fillColorBtn_->setObjectName("fillColorWell");
    fillColorBtn_->setFixedHeight(26);
    fillColorBtn_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    fillColorBtn_->setPopupMode(QToolButton::InstantPopup);
    fillColorBtn_->setToolTip(tr("Fill Color"));
    fillColorBtn_->setStyleSheet(
        "QToolButton#fillColorWell { border: 1px solid rgba(255,255,255,0.1);"
        "  border-radius: 4px; padding: 2px; }"
        "QToolButton#fillColorWell:hover { border-color: rgba(255,255,255,0.25); }");
    updateFillColorWell(canvas_->fillColor());

    auto* fillMenu = new QMenu(fillColorBtn_);
    connect(fillMenu, &QMenu::aboutToShow, this, [this, fillMenu]() {
        auto actions = fillMenu->actions();
        for (auto* a : actions) {
            fillMenu->removeAction(a);
            delete a;
        }
        auto recent = canvas_->recentColors();
        if (!recent.isEmpty()) {
            for (const auto& c : recent) {
                auto* a = new QAction(makeColorIcon(c), c.name(QColor::HexRgb).toUpper(), fillMenu);
                fillMenu->addAction(a);
                connect(a, &QAction::triggered, this, [this, c] {
                    canvas_->setFillColor(c);
                    canvas_->setFilled(true);
                    updateFillColorWell(c);
                });
            }
            fillMenu->addSeparator();
        }
        for (const auto& c : kFixedColors) {
            auto* a = new QAction(makeColorIcon(c), c.name(QColor::HexRgb).toUpper(), fillMenu);
            fillMenu->addAction(a);
            connect(a, &QAction::triggered, this, [this, c] {
                canvas_->setFillColor(c);
                canvas_->setFilled(true);
                updateFillColorWell(c);
            });
        }
        fillMenu->addSeparator();
        auto* customAction = new QAction(tr("Custom Color..."), fillMenu);
        fillMenu->addAction(customAction);
        connect(customAction, &QAction::triggered, this, [this] {
            auto color = QColorDialog::getColor(canvas_->fillColor().isValid() ? canvas_->fillColor() : Qt::white, this, tr("Choose Fill Color"));
            if (color.isValid()) {
                canvas_->setFillColor(color);
                canvas_->setFilled(true);
                canvas_->addRecentColor(color);
                updateFillColorWell(color);
            }
        });
        fillMenu->addSeparator();
        auto* noFillAction = new QAction(tr("No Fill"), fillMenu);
        fillMenu->addAction(noFillAction);
        connect(noFillAction, &QAction::triggered, this, [this] {
            canvas_->setFillColor(QColor());
            canvas_->setFilled(false);
            updateFillColorWell(QColor());
        });
    });
    fillColorBtn_->setMenu(fillMenu);
    layout->addWidget(fillColorBtn_);
}

void EditorWindow::buildStrokeSection(QVBoxLayout* layout, QWidget* content)
{
    layout->addSpacing(6);

    preview_ = new StrokePreview(content);

    struct StrokePreset { QString label; int width; };
    const StrokePreset strokes[] = {{tr("S"), 2}, {tr("M"), 4}, {tr("L"), 8}};
    auto* strokeGroup = new QButtonGroup(content);
    strokeGroup->setObjectName("strokeGroup");
    strokeGroup->setExclusive(true);
    auto* widthRow = new QHBoxLayout();
    widthRow->setContentsMargins(0, 0, 0, 0);
    widthRow->setSpacing(4);
    for (const auto& s : strokes) {
        auto* btn = new QToolButton(content);
        btn->setText(s.label);
        btn->setToolTip(tr("Stroke: %1px").arg(s.width));
        btn->setFixedHeight(28);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setCheckable(true);
        btn->setStyleSheet(kSelStyle);
        btn->setProperty("width", s.width);
        if (s.width == 4) btn->setChecked(true);
        strokeGroup->addButton(btn);
        connect(btn, &QToolButton::clicked, this, [this, s] {
            canvas_->setStrokeWidth(s.width);
            static_cast<StrokePreview*>(preview_)->showStroke(canvas_->color(), canvas_->strokeWidth());
            canvas_->setFocus();
        });
        widthRow->addWidget(btn);
    }
    layout->addLayout(widthRow);

    layout->addSpacing(2);
    layout->addWidget(static_cast<QWidget*>(preview_));
    layout->addSpacing(6);

    struct PropToggle { const char* text; const char* tip; bool defaultOn; void (AnnotationCanvas::*setter)(bool); };
    const PropToggle props[] = {
        {QT_TRANSLATE_NOOP("EditorWindow", "Outline"), QT_TRANSLATE_NOOP("EditorWindow", "Toggle text outline"), true, &AnnotationCanvas::setTextOutlineEnabled},
        {QT_TRANSLATE_NOOP("EditorWindow", "Fill"), QT_TRANSLATE_NOOP("EditorWindow", "Toggle fill for shapes"), false, &AnnotationCanvas::setFilled},
        {QT_TRANSLATE_NOOP("EditorWindow", "Blur"), QT_TRANSLATE_NOOP("EditorWindow", "Toggle mosaic blur mode"), false, &AnnotationCanvas::setMosaicBlurred},
        {QT_TRANSLATE_NOOP("EditorWindow", "Grid"), QT_TRANSLATE_NOOP("EditorWindow", "Toggle alignment grid"), false, &AnnotationCanvas::setGridEnabled},
    };

    auto* chipRow = new QHBoxLayout();
    chipRow->setContentsMargins(0, 0, 0, 0);
    chipRow->setSpacing(4);
    for (const auto& p : props) {
        auto* btn = new QToolButton(content);
        btn->setText(QCoreApplication::translate("EditorWindow", p.text));
        btn->setToolTip(QCoreApplication::translate("EditorWindow", p.tip));
        btn->setFixedHeight(26);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setCheckable(true);
        btn->setChecked(p.defaultOn);
        btn->setStyleSheet(kChipStyle);
        btn->setProperty("chipKey", QByteArray(p.text));
        connect(btn, &QToolButton::clicked, this, [this, p](bool checked) {
            (canvas_->*p.setter)(checked);
        });
        chipRow->addWidget(btn);
    }
    propsWidget_ = new QWidget(content);
    propsWidget_->setLayout(chipRow);
    layout->addWidget(propsWidget_);

    layout->addSpacing(6);

    addSliderRow(layout, content, tr("Opacity"), 0, 255, 255,
        [this](int v) { canvas_->setStrokeAlpha(v); },
        [this](int v) { return tr("%1%").arg(v * 100 / 255); });
}

void EditorWindow::buildArrowSection(QVBoxLayout* layout, QWidget* content)
{
    arrowWidget_ = new QWidget(content);
    auto* arrowRow = new QHBoxLayout(arrowWidget_);
    arrowRow->setContentsMargins(0, 0, 0, 0);
    arrowRow->setSpacing(4);
    auto* arrowLbl = new QLabel(tr("Arrow"), content);
    arrowLbl->setFixedWidth(34);
    arrowLbl->setStyleSheet(kSliderLabelStyle);
    arrowRow->addWidget(arrowLbl);

    struct ArrowDef { const char* text; const char* tip; int value; };
    const ArrowDef arrowDefs[] = {{QT_TRANSLATE_NOOP("EditorWindow", "Tri"), QT_TRANSLATE_NOOP("EditorWindow", "Triangular arrowhead"), 0},
                                  {QT_TRANSLATE_NOOP("EditorWindow", "Circle"), QT_TRANSLATE_NOOP("EditorWindow", "Circular endpoint"), 1},
                                  {QT_TRANSLATE_NOOP("EditorWindow", "Square"), QT_TRANSLATE_NOOP("EditorWindow", "Square endpoint"), 2}};
    auto* arrowGroup = new QButtonGroup(content);
    arrowGroup->setObjectName("arrowGroup");
    arrowGroup->setExclusive(true);
    for (const auto& ad : arrowDefs) {
        auto* btn = new QToolButton(content);
        btn->setText(tr(ad.text));
        btn->setCheckable(true);
        btn->setFixedHeight(26);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setStyleSheet(kChipStyle);
        btn->setToolTip(tr(ad.tip));
        if (ad.value == 0) btn->setChecked(true);
        arrowGroup->addButton(btn, ad.value);
        connect(btn, &QToolButton::clicked, this, [this, val = ad.value] {
            canvas_->setArrowStyle(static_cast<ArrowStyle>(val));
        });
        arrowRow->addWidget(btn);
    }
    arrowWidget_->setVisible(false);
    layout->addWidget(arrowWidget_);

    radiusWidget_ = new QWidget(content);
    auto* radiusRow = new QHBoxLayout(radiusWidget_);
    radiusRow->setContentsMargins(0, 0, 0, 0);
    radiusRow->setSpacing(4);
    auto* radiusLbl = new QLabel(tr("Radius"), content);
    radiusLbl->setFixedWidth(34);
    radiusLbl->setStyleSheet(kSliderLabelStyle);
    auto* radiusSlider = new QSlider(Qt::Horizontal, content);
    radiusSlider->setObjectName("radiusSlider");
    radiusSlider->setRange(0, 40);
    radiusSlider->setValue(0);
    radiusSlider->setFixedHeight(22);
    radiusSlider->setStyleSheet(kSliderStyle);
    auto* radiusVal = new QLabel(tr("Off"), content);
    radiusVal->setObjectName("radiusVal");
    radiusVal->setFixedWidth(30);
    radiusVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    radiusVal->setStyleSheet(kSliderValStyle);
    connect(radiusSlider, &QSlider::valueChanged, this, [this, radiusVal](int v) {
        canvas_->setCornerRadius(v);
        radiusVal->setText(v > 0 ? tr("%1px").arg(v) : tr("Off"));
    });
    radiusRow->addWidget(radiusLbl);
    radiusRow->addWidget(radiusSlider);
    radiusRow->addWidget(radiusVal);
    radiusWidget_->setVisible(false);
    layout->addWidget(radiusWidget_);

    layout->addSpacing(6);
}

QSlider* EditorWindow::addSliderRow(QVBoxLayout* layout, QWidget* content,
                                    const QString& label, int min, int max, int def,
                                    std::function<void(int)> onChanged,
                                    std::function<QString(int)> fmt)
{
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);
    auto* lbl = new QLabel(label, content);
    lbl->setFixedWidth(34);
    lbl->setStyleSheet(kSliderLabelStyle);
    auto* slider = new QSlider(Qt::Horizontal, content);
    slider->setRange(min, max);
    slider->setValue(def);
    slider->setFixedHeight(22);
    slider->setStyleSheet(kSliderStyle);
    auto* val = new QLabel(fmt(def), content);
    val->setFixedWidth(32);
    val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    val->setStyleSheet(kSliderValStyle);
    connect(slider, &QSlider::valueChanged, this, [val, onChanged, fmt](int v) {
        onChanged(v);
        val->setText(fmt(v));
    });
    row->addWidget(lbl);
    row->addWidget(slider);
    row->addWidget(val);
    layout->addLayout(row);
    return slider;
}

void EditorWindow::buildFontSection(QVBoxLayout* layout, QWidget* content)
{
    fontWidget_ = new QWidget(content);
    auto* fontLayout = new QVBoxLayout(fontWidget_);
    fontLayout->setContentsMargins(0, 0, 0, 0);
    fontLayout->setSpacing(4);

    auto* fontCombo = new QFontComboBox(content);
    fontCombo->setCurrentFont(QFont(canvas_->fontFamily()));
    fontCombo->setToolTip(tr("Font family"));
    fontCombo->setStyleSheet(
        "QFontComboBox { font: 9px; color: #bcbec6; background: rgba(255,255,255,0.04);"
        "  border: 1px solid rgba(255,255,255,0.08); border-radius: 4px; padding: 2px 4px; }"
        "QFontComboBox:hover { border-color: #2fbf9f; }"
        "QFontComboBox::drop-down { border: none; width: 16px; }"
        "QFontComboBox QAbstractItemView { font: 9px; }");
    fontCombo->setFixedHeight(26);
    connect(fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont& f) {
        canvas_->setFontFamily(f.family());
    });
    fontLayout->addWidget(fontCombo);

    auto* fontStyleRow = new QHBoxLayout();
    fontStyleRow->setContentsMargins(0, 0, 0, 0);
    fontStyleRow->setSpacing(4);

    auto makeFontToggle = [content](const QString& text, const QString& tip, bool checked) -> QToolButton* {
        auto* btn = new QToolButton(content);
        btn->setText(text);
        btn->setToolTip(tip);
        btn->setCheckable(true);
        btn->setChecked(checked);
        btn->setFixedSize(30, 28);
        btn->setStyleSheet(kSelStyle);
        return btn;
    };

    auto* boldBtn = makeFontToggle(tr("B"), tr("Bold"), canvas_->bold());
    auto* italicBtn = makeFontToggle(tr("I"), tr("Italic"), canvas_->italic());
    auto* underlineBtn = makeFontToggle(tr("U"), tr("Underline"), canvas_->underline());
    connect(boldBtn, &QToolButton::clicked, this, [this](bool checked) { canvas_->setBold(checked); });
    connect(italicBtn, &QToolButton::clicked, this, [this](bool checked) { canvas_->setItalic(checked); });
    connect(underlineBtn, &QToolButton::clicked, this, [this](bool checked) { canvas_->setUnderline(checked); });

    fontStyleRow->addWidget(boldBtn);
    fontStyleRow->addWidget(italicBtn);
    fontStyleRow->addWidget(underlineBtn);
    fontStyleRow->addStretch();

    auto makeAlignToggle = [content](const QString& text, const QString& tip) -> QToolButton* {
        auto* btn = new QToolButton(content);
        btn->setText(text);
        btn->setToolTip(tip);
        btn->setCheckable(true);
        btn->setFixedSize(30, 28);
        btn->setStyleSheet(kSelStyle);
        return btn;
    };

    auto* alignLeft = makeAlignToggle(QStringLiteral("\u251C"), tr("Align left"));
    auto* alignCenter = makeAlignToggle(QStringLiteral("\u253C"), tr("Align center"));
    auto* alignRight = makeAlignToggle(QStringLiteral("\u2524"), tr("Align right"));
    auto* alignGroup = new QButtonGroup(content);
    alignGroup->setExclusive(true);
    alignGroup->addButton(alignLeft, 0);
    alignGroup->addButton(alignCenter, 1);
    alignGroup->addButton(alignRight, 2);
    int curAlign = canvas_->textAlignment();
    int alignId = 0;
    if (curAlign > 0) {
        if (curAlign & Qt::AlignHCenter) alignId = 1;
        else if (curAlign & Qt::AlignRight) alignId = 2;
    }
    if (auto* btn = alignGroup->button(alignId))
        btn->setChecked(true);
    connect(alignGroup, qOverload<int>(&QButtonGroup::buttonClicked), this, [this](int id) {
        int align = -1;
        switch (id) {
        case 0: align = Qt::AlignLeft | Qt::AlignTop; break;
        case 1: align = Qt::AlignHCenter | Qt::AlignTop; break;
        case 2: align = Qt::AlignRight | Qt::AlignTop; break;
        }
        canvas_->setTextAlignment(align);
    });

    fontStyleRow->addWidget(alignLeft);
    fontStyleRow->addWidget(alignCenter);
    fontStyleRow->addWidget(alignRight);
    fontLayout->addLayout(fontStyleRow);

    auto* fontSizeRow = new QHBoxLayout();
    fontSizeRow->setContentsMargins(0, 0, 0, 0);
    fontSizeRow->setSpacing(4);

    auto* fontLabel = new QLabel(tr("Font"), content);
    fontLabel->setFixedWidth(34);
    fontLabel->setStyleSheet("color: #8e8e93; font: 10px; padding: 0;");

    auto* fontSizeDec = new QToolButton(content);
    fontSizeDec->setText(tr("-"));
    fontSizeDec->setToolTip(tr("Decrease font size ( [ )"));
    fontSizeDec->setFixedSize(28, 28);
    fontSizeDec->setStyleSheet(kSelStyle);

    auto* fontSizeSlider = new QSlider(Qt::Horizontal, content);
    fontSizeSlider->setRange(8, 72);
    fontSizeSlider->setValue(canvas_->fontSize());
    fontSizeSlider->setFixedHeight(22);
    fontSizeSlider->setStyleSheet(kSliderStyle);
    fontSizeSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* fontSizeVal = new QLabel(tr("%1px").arg(canvas_->fontSize()), content);
    fontSizeVal->setFixedWidth(32);
    fontSizeVal->setToolTip(tr("Font size for Text / Numbered tools"));
    fontSizeVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    fontSizeVal->setStyleSheet("color: #bcbec6; font: 10px; padding: 0;");

    auto* fontSizeInc = new QToolButton(content);
    fontSizeInc->setText(tr("+"));
    fontSizeInc->setToolTip(tr("Increase font size ( ] )"));
    fontSizeInc->setFixedSize(28, 28);
    fontSizeInc->setStyleSheet(kSelStyle);

    fontSizeRow->addWidget(fontLabel);
    fontSizeRow->addWidget(fontSizeDec);
    fontSizeRow->addWidget(fontSizeSlider);
    fontSizeRow->addWidget(fontSizeVal);
    fontSizeRow->addWidget(fontSizeInc);
    fontLayout->addLayout(fontSizeRow);

    auto updateFontSizeUI = [fontSizeVal, fontSizeSlider](int size) {
        fontSizeVal->setText(tr("%1px").arg(size));
        fontSizeSlider->blockSignals(true);
        fontSizeSlider->setValue(size);
        fontSizeSlider->blockSignals(false);
    };

    connect(fontSizeDec, &QToolButton::clicked, this, [this, updateFontSizeUI] {
        int newSize = qMax(8, canvas_->fontSize() - 2);
        canvas_->setFontSize(newSize);
        updateFontSizeUI(newSize);
    });
    connect(fontSizeInc, &QToolButton::clicked, this, [this, updateFontSizeUI] {
        int newSize = qMin(72, canvas_->fontSize() + 2);
        canvas_->setFontSize(newSize);
        updateFontSizeUI(newSize);
    });
    connect(fontSizeSlider, &QSlider::valueChanged, this, [this, updateFontSizeUI](int size) {
        canvas_->setFontSize(size, false);
        updateFontSizeUI(size);
    });
    connect(fontSizeSlider, &QSlider::sliderReleased, this, [this] {
        QSettings().setValue("editor/fontSize", canvas_->fontSize());
    });
    connect(canvas_, &AnnotationCanvas::fontSizeChanged, this, [updateFontSizeUI](int size) {
        updateFontSizeUI(size);
    });

    // Text background
    auto* bgRow = new QHBoxLayout();
    bgRow->setContentsMargins(0, 0, 0, 0);
    bgRow->setSpacing(4);

    auto* bgLabel = new QLabel(tr("BG"), content);
    bgLabel->setFixedWidth(34);
    bgLabel->setStyleSheet("color: #8e8e93; font: 10px; padding: 0;");

    auto* bgToggle = new QToolButton(content);
    bgToggle->setText(tr("On"));
    bgToggle->setToolTip(tr("Toggle text background"));
    bgToggle->setCheckable(true);
    bgToggle->setChecked(canvas_->textBackgroundEnabled());
    bgToggle->setFixedHeight(22);
    bgToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    bgToggle->setStyleSheet(kChipStyle);
    bgToggle->setText(canvas_->textBackgroundEnabled() ? tr("On") : tr("Off"));
    connect(bgToggle, &QToolButton::clicked, this, [this, bgToggle](bool checked) {
        canvas_->setTextBackgroundEnabled(checked);
        bgToggle->setText(checked ? tr("On") : tr("Off"));
    });

    auto* bgColorBtn = new QToolButton(content);
    bgColorBtn->setFixedSize(22, 22);
    bgColorBtn->setToolTip(tr("Text background color"));
    auto updateBgColorIcon = [bgColorBtn, this](QColor color = QColor()) {
        if (!color.isValid()) color = canvas_->textBackgroundColor();
        QPixmap px(14, 14);
        px.fill(color);
        QPainter p(&px);
        p.setPen(QPen(QColor(255, 255, 255, 64), 1));
        p.drawRect(QRectF(0.5, 0.5, 13, 13));
        p.end();
        bgColorBtn->setIcon(QIcon(px));
    };
    updateBgColorIcon();
    bgColorBtn->setStyleSheet(
        "QToolButton { border: 1px solid rgba(255,255,255,0.15); border-radius: 3px; padding: 2px; }"
        "QToolButton:hover { border-color: #2fbf9f; }");
    connect(bgColorBtn, &QToolButton::clicked, this, [this, updateBgColorIcon] {
        auto color = QColorDialog::getColor(canvas_->textBackgroundColor(), this, tr("Choose Text Background Color"));
        if (color.isValid()) {
            canvas_->setTextBackgroundColor(color);
            updateBgColorIcon();
        }
    });

    bgRow->addWidget(bgLabel);
    bgRow->addWidget(bgToggle);
    bgRow->addWidget(bgColorBtn);
    fontLayout->addLayout(bgRow);

    connect(canvas_, &AnnotationCanvas::textPropertiesChanged, this, [this, fontCombo, boldBtn, italicBtn, underlineBtn, alignGroup, bgToggle, updateBgColorIcon]() {
        auto idx = canvas_->selectedIndex();
        bool sel = (idx >= 0);
        const Annotation* a = sel ? &canvas_->annotationAt(idx) : nullptr;
        bool textSel = (a && (a->tool == AnnotationTool::Text || a->tool == AnnotationTool::Numbered));

        fontCombo->setCurrentFont(QFont(textSel && !a->fontFamily.isEmpty() ? a->fontFamily : canvas_->fontFamily()));
        boldBtn->setChecked(textSel ? a->bold : canvas_->bold());
        italicBtn->setChecked(textSel ? a->italic : canvas_->italic());
        underlineBtn->setChecked(textSel ? a->underline : canvas_->underline());
        int align = textSel ? a->textAlignment : canvas_->textAlignment();
        int id = 0;
        if (align == (Qt::AlignHCenter | Qt::AlignTop)) id = 1;
        else if (align == (Qt::AlignRight | Qt::AlignTop)) id = 2;
        auto* btn = alignGroup->button(id);
        if (btn) btn->setChecked(true);
        bool bgOn = textSel ? a->textBackground : canvas_->textBackgroundEnabled();
        bgToggle->setChecked(bgOn);
        bgToggle->setText(bgOn ? tr("On") : tr("Off"));
        updateBgColorIcon(textSel ? a->textBackgroundColor : canvas_->textBackgroundColor());
    });

    fontWidget_->setVisible(false);
    layout->addWidget(fontWidget_);

    layout->addSpacing(8);
}

void EditorWindow::buildCropSection(QVBoxLayout* layout, QWidget* content)
{
    cropWidget_ = new QWidget(content);
    auto* cropLayout = new QVBoxLayout(cropWidget_);
    cropLayout->setContentsMargins(0, 0, 0, 0);
    cropLayout->setSpacing(4);

    auto* cropLabel = new QLabel(tr("Aspect Ratio"), content);
    cropLabel->setStyleSheet("color: #8e8e93; font: 10px; padding: 0;");
    cropLayout->addWidget(cropLabel);

    struct RatioDef { const char* text; const char* tip; double value; };
    const RatioDef ratios[] = {
        {QT_TRANSLATE_NOOP("EditorWindow", "Free"), QT_TRANSLATE_NOOP("EditorWindow", "Unconstrained crop"), 0.0},
        {QT_TRANSLATE_NOOP("EditorWindow", "1:1"), QT_TRANSLATE_NOOP("EditorWindow", "Square crop (1:1)"), 1.0},
        {QT_TRANSLATE_NOOP("EditorWindow", "16:9"), QT_TRANSLATE_NOOP("EditorWindow", "Widescreen crop (16:9)"), 16.0 / 9.0},
        {QT_TRANSLATE_NOOP("EditorWindow", "4:3"), QT_TRANSLATE_NOOP("EditorWindow", "Standard crop (4:3)"), 4.0 / 3.0},
        {QT_TRANSLATE_NOOP("EditorWindow", "3:2"), QT_TRANSLATE_NOOP("EditorWindow", "Classic photo crop (3:2)"), 3.0 / 2.0},
    };
    auto* ratioGroup = new QButtonGroup(content);
    ratioGroup->setExclusive(true);
    auto* ratioRow = new QHBoxLayout();
    ratioRow->setContentsMargins(0, 0, 0, 0);
    ratioRow->setSpacing(4);
    for (int i = 0; i < std::size(ratios); ++i) {
        auto* btn = new QToolButton(content);
        auto label = QCoreApplication::translate("EditorWindow", ratios[i].text);
        btn->setText(label);
        btn->setToolTip(QCoreApplication::translate("EditorWindow", ratios[i].tip));
        btn->setCheckable(true);
        btn->setFixedHeight(26);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setStyleSheet(kChipStyle);
        if (i == 0) btn->setChecked(true);
        ratioGroup->addButton(btn, i);
        connect(btn, &QToolButton::clicked, this, [this, val = ratios[i].value, label] {
            canvas_->setCropAspectRatio(val);
            if (val > 0.0)
                statusBar()->showMessage(tr("Crop ratio locked: %1").arg(label), 3000);
            else
                statusBar()->showMessage(tr("Crop ratio: Free"), 3000);
        });
        ratioRow->addWidget(btn);
    }
    cropLayout->addLayout(ratioRow);

    cropWidget_->setVisible(false);
    layout->addWidget(cropWidget_);
}

void EditorWindow::buildZoomSection(QVBoxLayout* layout, QWidget* content)
{
    auto* zoomRow = new QHBoxLayout();
    zoomRow->setContentsMargins(0, 0, 0, 0);
    zoomRow->setSpacing(4);

    auto* zoomLabel = new QLabel(tr("Zoom"), content);
    zoomLabel->setFixedWidth(34);
    zoomLabel->setStyleSheet("color: #8e8e93; font: 10px; padding: 0;");

    auto* zoomOut = new QToolButton(content);
    zoomOut->setText(tr("-"));
    zoomOut->setToolTip(tr("Zoom out"));
    zoomOut->setFixedSize(26, 28);
    zoomOut->setStyleSheet(kSelStyle);

    auto* zoomIn = new QToolButton(content);
    zoomIn->setText(tr("+"));
    zoomIn->setToolTip(tr("Zoom in"));
    zoomIn->setFixedSize(26, 28);
    zoomIn->setStyleSheet(kSelStyle);

    auto* zoomReset = new QToolButton(content);
    zoomReset->setText(tr("1:1"));
    zoomReset->setToolTip(tr("Reset zoom to 100% (Ctrl+0)"));
    zoomReset->setFixedSize(34, 28);
    zoomReset->setStyleSheet(kZoomBtnStyle);

    auto* zoomFit = new QToolButton(content);
    zoomFit->setText(tr("Fit"));
    zoomFit->setToolTip(tr("Fit to window (Ctrl+9)"));
    zoomFit->setFixedSize(34, 28);
    zoomFit->setStyleSheet(kZoomBtnStyle);

    zoomSlider_ = new QSlider(Qt::Horizontal, content);
    zoomSlider_->setRange(10, 500);
    zoomSlider_->setValue(100);
    zoomSlider_->setFixedHeight(22);
    zoomSlider_->setStyleSheet(kSliderStyle);
    zoomSlider_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    zoomVal_ = new QLabel(tr("100%"), content);
    zoomVal_->setFixedWidth(36);
    zoomVal_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    zoomVal_->setStyleSheet("color: #bcbec6; font: 9px; padding: 0;");

    auto updateZoomUI = [this](double factor) {
        int pct = static_cast<int>(factor * 100);
        zoomSlider_->blockSignals(true);
        zoomSlider_->setValue(qBound(10, pct, 500));
        zoomSlider_->blockSignals(false);
        zoomVal_->setText(tr("%1%").arg(pct));
    };

    connect(zoomSlider_, &QSlider::valueChanged, this, [this, updateZoomUI](int pct) {
        double factor = pct / 100.0;
        canvas_->zoomAt(factor, QPoint(canvas_->width() / 2, canvas_->height() / 2));
        updateZoomUI(canvas_->zoomFactor());
    });
    connect(zoomOut, &QToolButton::clicked, this, [this, updateZoomUI] {
        auto factor = canvas_->zoomFactor();
        canvas_->zoomAt(factor / 1.2, QPoint(canvas_->width() / 2, canvas_->height() / 2));
        updateZoomUI(canvas_->zoomFactor());
    });
    connect(zoomIn, &QToolButton::clicked, this, [this, updateZoomUI] {
        auto factor = canvas_->zoomFactor();
        canvas_->zoomAt(factor * 1.2, QPoint(canvas_->width() / 2, canvas_->height() / 2));
        updateZoomUI(canvas_->zoomFactor());
    });
    connect(zoomReset, &QToolButton::clicked, this, [this, updateZoomUI] {
        canvas_->zoomAt(1.0, QPoint(canvas_->width() / 2, canvas_->height() / 2));
        updateZoomUI(1.0);
    });
    connect(zoomFit, &QToolButton::clicked, this, [this, updateZoomUI] {
        canvas_->zoomFit();
        updateZoomUI(canvas_->zoomFactor());
    });
    connect(canvas_, &AnnotationCanvas::zoomChanged, this, [updateZoomUI](double factor) {
        updateZoomUI(factor);
    });

    zoomRow->addWidget(zoomLabel);
    zoomRow->addWidget(zoomOut);
    zoomRow->addWidget(zoomSlider_);
    zoomRow->addWidget(zoomVal_);
    zoomRow->addWidget(zoomIn);
    zoomRow->addWidget(zoomReset);
    zoomRow->addWidget(zoomFit);
    layout->addLayout(zoomRow);

    layout->addSpacing(10);
}

void EditorWindow::buildImageAdjustSection(QVBoxLayout* layout, QWidget* content)
{
    auto* adjustLabel = new QLabel(tr("Adjust"), content);
    adjustLabel->setStyleSheet("color: #8e8e93; font: 10px; padding: 0 0 0 6px;"
        "border-left: 2px solid rgba(47,191,159,0.3);");
    layout->addWidget(adjustLabel);

    brightSlider_ = addSliderRow(layout, content, tr("Bright"), -100, 100, 0,
        [this](int) {},
        [](int v) { return v > 0 ? tr("+%1").arg(v) : QString::number(v); });

    contrastSlider_ = addSliderRow(layout, content, tr("Contrast"), -100, 100, 0,
        [this](int) {},
        [](int v) { return v > 0 ? tr("+%1").arg(v) : QString::number(v); });

    auto doAdjust = [this] {
        if (!imageAdjustStarted_) {
            imageAdjustStarted_ = true;
            canvas_->beginImageAdjust();
        }
        canvas_->previewAdjustImage(brightSlider_->value(), contrastSlider_->value());
    };
    connect(brightSlider_, &QSlider::valueChanged, this, doAdjust);
    connect(contrastSlider_, &QSlider::valueChanged, this, doAdjust);
    connect(brightSlider_, &QSlider::sliderReleased, this, [this] {
        imageAdjustStarted_ = false;
    });
    connect(contrastSlider_, &QSlider::sliderReleased, this, [this] {
        imageAdjustStarted_ = false;
    });

    layout->addSpacing(10);
}

void EditorWindow::buildTransformSection(QVBoxLayout* layout, QWidget* content)
{
    auto* transformLabel = new QLabel(tr("Transform"), content);
    transformLabel->setStyleSheet("color: #8e8e93; font: 10px; padding: 0 0 0 6px;"
        "border-left: 2px solid rgba(47,191,159,0.3);");
    layout->addWidget(transformLabel);

    auto addTransformBtn = [this, content](const QIcon& icon, const QString& tip) -> QToolButton* {
        auto* btn = new QToolButton(content);
        btn->setIcon(icon);
        btn->setToolTip(tip);
        btn->setFixedHeight(26);
        btn->setIconSize(QSize(16, 16));
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setStyleSheet(kSmallBtnStyle);
        return btn;
    };

    auto* transformRow = new QHBoxLayout();
    transformRow->setContentsMargins(0, 0, 0, 0);
    transformRow->setSpacing(4);

    auto* rotCw = addTransformBtn(iconProvider_.icon(IconName::RotateRight), tr("Rotate 90\u00B0 clockwise\tR"));
    auto* rotCcw = addTransformBtn(iconProvider_.icon(IconName::RotateLeft), tr("Rotate 90\u00B0 counter-clockwise\tShift+R"));
    auto* rot180 = addTransformBtn(iconProvider_.icon(IconName::Rotate180), tr("Rotate 180\u00B0\tCtrl+R"));
    auto* flipH = addTransformBtn(iconProvider_.icon(IconName::FlipHorizontal), tr("Flip horizontal\tH"));
    auto* flipV = addTransformBtn(iconProvider_.icon(IconName::FlipVertical), tr("Flip vertical\tV"));

    connect(rotCw, &QToolButton::clicked, this, [this] {
        canvas_->rotateImage(90);
        statusBar()->showMessage(tr("Rotated 90\u00B0 clockwise"), 3000);
    });
    connect(rotCcw, &QToolButton::clicked, this, [this] {
        canvas_->rotateImage(270);
        statusBar()->showMessage(tr("Rotated 90\u00B0 counter-clockwise"), 3000);
    });
    connect(rot180, &QToolButton::clicked, this, [this] {
        canvas_->rotateImage(180);
        statusBar()->showMessage(tr("Rotated 180\u00B0"), 3000);
    });
    connect(flipH, &QToolButton::clicked, this, [this] {
        canvas_->flipImage(true, false);
        statusBar()->showMessage(tr("Flipped horizontally"), 3000);
    });
    connect(flipV, &QToolButton::clicked, this, [this] {
        canvas_->flipImage(false, true);
        statusBar()->showMessage(tr("Flipped vertically"), 3000);
    });

    transformRow->addWidget(rotCw);
    transformRow->addWidget(rotCcw);
    transformRow->addWidget(rot180);
    transformRow->addWidget(flipH);
    transformRow->addWidget(flipV);
    layout->addLayout(transformRow);

    // Keyboard shortcuts for transform operations
    auto* rotCwShortcut = new QShortcut(QKeySequence(Qt::Key_R), this);
    connect(rotCwShortcut, &QShortcut::activated, rotCw, &QToolButton::click);
    auto* rotCcwShortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_R), this);
    connect(rotCcwShortcut, &QShortcut::activated, rotCcw, &QToolButton::click);
    auto* rot180Shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), this);
    connect(rot180Shortcut, &QShortcut::activated, rot180, &QToolButton::click);
    auto* flipHShortcut = new QShortcut(QKeySequence(Qt::Key_H), this);
    connect(flipHShortcut, &QShortcut::activated, flipH, &QToolButton::click);
    auto* flipVShortcut = new QShortcut(QKeySequence(Qt::Key_V), this);
    connect(flipVShortcut, &QShortcut::activated, flipV, &QToolButton::click);

    layout->addSpacing(10);
}

void EditorWindow::buildLayerSection(QVBoxLayout* layout, QWidget* content)
{
    auto* layerLabel = new QLabel(tr("Layers"), content);
    layerLabel->setStyleSheet("color: #8e8e93; font: 10px; padding: 0 0 0 6px;"
        "border-left: 2px solid rgba(47,191,159,0.3);");
    layout->addWidget(layerLabel);

    layout->addSpacing(2);

    const QString listStyle =
        "QListWidget { background: transparent; border: 1px solid rgba(255,255,255,0.06);"
        "  border-radius: 4px; padding: 2px; font: 9px; color: #bcbec6; outline: none; }"
        "QListWidget::item { padding: 4px 6px; border-radius: 3px; }"
        "QListWidget::item:hover { background: rgba(255,255,255,0.06); }"
        "QListWidget::item:selected { background: rgba(47,191,159,0.15); color: #2fbf9f; }";

    layerList_ = new QListWidget(content);
    layerList_->setStyleSheet(listStyle);
    layerList_->setMinimumHeight(60);
    layerList_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layerList_->setContextMenuPolicy(Qt::CustomContextMenu);
    layerList_->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(layerList_, &QListWidget::currentRowChanged, this, [this](int row) {
        if (rebuildingLayerList_) return;
        if (row < 0) return;
        int annCount = canvas_->annotationCount();
        if (annCount == 0) return;
        int idx = annCount - 1 - row;
        canvas_->selectAnnotation(idx);
    });

    connect(layerList_, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        if (rebuildingLayerList_) return;
        int row = layerList_->row(item);
        int annCount = canvas_->annotationCount();
        if (annCount == 0) return;
        int idx = annCount - 1 - row;
        bool visible = (item->checkState() == Qt::Checked);
        canvas_->setAnnotationVisible(idx, visible);
    });

    connect(layerList_, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        auto* item = layerList_->itemAt(pos);
        if (!item) return;
        int row = layerList_->row(item);
        int annCount = canvas_->annotationCount();
        if (annCount == 0) return;
        int idx = annCount - 1 - row;

        QMenu menu(layerList_);
        auto* delAct = menu.addAction(tr("Delete"));
        auto* dupAct = menu.addAction(tr("Duplicate"));
        menu.addSeparator();
        auto* upAct = menu.addAction(tr("Move Up"));
        auto* downAct = menu.addAction(tr("Move Down"));

        auto* act = menu.exec(layerList_->mapToGlobal(pos));
        if (act == delAct) {
            auto ret = QMessageBox::question(this, tr("Delete Annotation"),
                tr("Are you sure you want to delete this annotation? This action cannot be undone."),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret != QMessageBox::Yes) return;
            canvas_->deleteAnnotation(idx);
            rebuildLayerList();
        } else if (act == dupAct) {
            canvas_->duplicateAnnotation(idx);
            rebuildLayerList();
        } else if (act == upAct) {
            if (idx < annCount - 1) {
                canvas_->swapAnnotations(idx, idx + 1);
                rebuildLayerList();
            }
        } else if (act == downAct) {
            if (idx > 0) {
                canvas_->swapAnnotations(idx, idx - 1);
                rebuildLayerList();
            }
        }
    });

    layout->addWidget(layerList_);

    layout->addSpacing(6);
}

void EditorWindow::buildInfoSection(QVBoxLayout* layout, QWidget* content)
{
    contextHint_ = new QLabel(content);
    contextHint_->setStyleSheet("color: #8e8e93; font: 9px; padding: 0 2px;");
    contextHint_->setWordWrap(true);
    layout->addWidget(contextHint_);

    imageInfoLabel_ = new QLabel(content);
    imageInfoLabel_->setStyleSheet("color: #5e5e63; font: 8px; padding: 0 2px;");
    layout->addWidget(imageInfoLabel_);

    layout->addSpacing(4);
}

void EditorWindow::wireToolPanelConnections()
{
    connect(undoBtn_, &QToolButton::clicked, this, [this] { canvas_->undo(); refreshPanelUi(); rebuildLayerList(); canvas_->setFocus(); });
    connect(redoBtn_, &QToolButton::clicked, this, [this] { canvas_->redo(); refreshPanelUi(); rebuildLayerList(); canvas_->setFocus(); });

    connect(canvas_, &AnnotationCanvas::modified, this, [this] { refreshPanelUi(); rebuildLayerList(); });

    connect(copyBtn_, &QToolButton::clicked, this, [this] {
        emit imageEdited(canvas_->renderedImage());
        emit copyRequested();
        statusBar()->showMessage(tr("Copied to clipboard"), 3000);
        canvas_->setFocus();
    });
    connect(pinBtn_, &QToolButton::clicked, this, [this] {
        auto img = canvas_->renderedImage();
        emit imageEdited(img);
        emit pinRequested(img);
        statusBar()->showMessage(tr("Image pinned"), 3000);
        canvas_->setFocus();
    });
    connect(saveBtn_, &QToolButton::clicked, this, [this] {
        canvas_->clearModified();
        emit imageEdited(canvas_->renderedImage());
        emit saveRequested();
        statusBar()->showMessage(tr("Saved"), 3000);
        canvas_->setFocus();
    });
    connect(exportBtn_, &QToolButton::clicked, this, [this] {
        auto path = QFileDialog::getSaveFileName(this, tr("Save As"), QString(),
            tr("PNG (*.png);;JPEG (*.jpg *.jpeg)"));
        if (!path.isEmpty()) {
            if (canvas_->renderedImage().save(path)) {
                statusBar()->showMessage(tr("Saved to %1").arg(path), 5000);
            } else {
                statusBar()->showMessage(tr("Failed to save image"), 5000);
            }
        }
        canvas_->setFocus();
    });

    connect(canvas_, &AnnotationCanvas::selectionChanged, this, [this]() {
        rebuildLayerList();
        auto idx = canvas_->selectedIndex();
        if (idx >= 0) {
            const auto& a = canvas_->annotationAt(idx);
            updateColorWell(a.color);
            updateFillColorWell(a.fillColor);
            if (preview_)
                static_cast<StrokePreview*>(preview_)->showStroke(a.color, a.strokeWidth);
            if (auto* sg = findChild<QButtonGroup*>("strokeGroup")) {
                for (auto* btn : sg->buttons())
                    btn->setChecked(btn->property("width").toInt() == a.strokeWidth);
            }
            for (auto* btn : propsWidget_->findChildren<QToolButton*>()) {
                auto key = btn->property("chipKey").toByteArray();
                if (key == "Fill")
                    btn->setChecked(a.filled);
                else if (key == "Outline")
                    btn->setChecked(a.textOutline);
                else if (key == "Blur")
                    btn->setChecked(a.blurRadius > 0);
                else if (key == "Grid")
                    btn->setChecked(canvas_->gridEnabled());
            }
            if (a.tool == AnnotationTool::Arrow) {
                if (auto* ag = findChild<QButtonGroup*>("arrowGroup")) {
                    auto* abtn = ag->button(static_cast<int>(a.arrowStyle));
                    if (abtn) abtn->setChecked(true);
                }
            }
            if (auto* rs = findChild<QSlider*>("radiusSlider")) {
                rs->setValue(a.cornerRadius);
                if (auto* rv = findChild<QLabel*>("radiusVal"))
                    rv->setText(a.cornerRadius > 0 ? tr("%1px").arg(a.cornerRadius) : tr("Off"));
            }
            canvas_->syncTextPropertiesUI();
        } else {
            syncPanelDefaults();
        }
    });
}

} // namespace snappaste