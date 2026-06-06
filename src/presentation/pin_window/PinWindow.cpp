#include "presentation/pin_window/PinWindow.h"
#include "presentation/pin_window/EditToolbar.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QFileDialog>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QDrag>
#include <QMimeData>
#include <QContextMenuEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QEasingCurve>
#include <QPointer>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QToolTip>
#include <QTransform>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace snappaste {

namespace {

constexpr int kResizeMargin = 6;
constexpr int kMinPinSize = 40;
constexpr int kThumbnailMaxSize = 200;
constexpr int kSnapThreshold = 12;
constexpr int kSnapMargin = 6;

const QColor kPinAccent("#2fbf9f");
const QColor kZoomTextColor(255, 255, 255, 160);
const QColor kClickThroughFill(20, 26, 33, 18);
const QColor kControlOverlay(20, 26, 33, 28);
const QColor kControlBorder(255, 255, 255, 110);
const QColor kOverflowBg(20, 26, 33, 200);
const QColor kOverflowText("#bcbec6");

const QColor kOcrBlockFill(47, 191, 159, 30);
const QColor kOcrBlockBorder(47, 191, 159, 120);
const QColor kOcrBlockHoverFill(47, 191, 159, 80);
const QColor kOcrBlockHoverBorder(47, 191, 159, 220);
const QColor kOcrBlockSelectedFill(47, 191, 159, 60);
const QColor kOcrBlockSelectedBorder(47, 191, 159, 200);

const QFont& kThumbnailFont()
{
    static const QFont f = [] {
        QFont font = QApplication::font();
        font.setPointSize(9);
        font.setBold(true);
        return font;
    }();
    return f;
}

const QFont& kZoomFont()
{
    static const QFont f = [] {
        QFont font = QApplication::font();
        font.setPointSize(10);
        return font;
    }();
    return f;
}

const QFont kOverflowDotsFont("Segoe UI", 10, QFont::Bold);

const QFont& kOcrInfoFont()
{
    static const QFont f = [] {
        QFont font = QApplication::font();
        font.setPointSize(10);
        font.setBold(true);
        return font;
    }();
    return f;
}

} // namespace

PinWindow::PinWindow(PinnedItem item, IIconProvider& iconProvider, QWidget* parent)
    : QWidget(parent)
    , item_(std::move(item))
    , iconProvider_(iconProvider)
{
    item_.image.setDevicePixelRatio(item_.state.devicePixelRatio);
    applyWindowFlags();
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumSize(kMinPinSize, kMinPinSize);
    showAnimation_ = new QPropertyAnimation(this, "geometry", this);
    showAnimation_->setDuration(100);
    showAnimation_->setEasingCurve(QEasingCurve::OutCubic);
    applyState();
}

qint64 PinWindow::id() const noexcept
{
    return item_.id;
}

const PinnedImageState& PinWindow::state() const noexcept
{
    return item_.state;
}

void PinWindow::setPinnedVisible(bool visible)
{
    item_.state.options.visible = visible;
    if (!visible) {
        if (!visibleSaved_) {
            visibleSaved_ = true;
            savedClickThrough_ = item_.state.options.clickThrough;
        }
        item_.state.options.clickThrough = false;
        windowInteraction_.setClickThrough(this, false);
    } else {
        visibleSaved_ = false;
        windowInteraction_.setClickThrough(this, savedClickThrough_);
    }
    if (visible) {
        show();
        raise();
        activateWindow();
    } else {
        hide();
    }
    update();
    emitStateChanged();
}

void PinWindow::restoreInteraction()
{
    controlsVisible_ = true;
    setPinnedVisible(true);
}

PinWindow::ResizeEdge PinWindow::resizeEdgeAt(const QPoint& pos) const
{
    const auto r = rect();
    const bool left = pos.x() <= r.left() + kResizeMargin;
    const bool right = pos.x() >= r.right() - kResizeMargin;
    const bool top = pos.y() <= r.top() + kResizeMargin;
    const bool bottom = pos.y() >= r.bottom() - kResizeMargin;

    if (top && left) return EdgeTopLeft;
    if (top && right) return EdgeTopRight;
    if (bottom && left) return EdgeBottomLeft;
    if (bottom && right) return EdgeBottomRight;
    if (left) return EdgeLeft;
    if (right) return EdgeRight;
    if (top) return EdgeTop;
    if (bottom) return EdgeBottom;
    return EdgeNone;
}

void PinWindow::applyResizeToScale()
{
    const auto imgSize = item_.image.size() / item_.image.devicePixelRatio();
    if (imgSize.isEmpty()) return;
    const auto winSize = frameGeometry().size();
    item_.state.transform.scale = std::max(0.1, std::min(8.0,
        static_cast<double>(winSize.width()) / imgSize.width()));
    item_.state = normalizedState(item_.state);
}

QRect PinWindow::constrainedResizeGeometry(const QPoint& globalPos) const
{
    const auto imgSize = item_.image.size() / item_.image.devicePixelRatio();
    if (imgSize.isEmpty()) {
        return resizeStartGeometry_;
    }

    const auto delta = globalPos - resizeStartGlobal_;
    auto targetWidth = resizeStartGeometry_.width();
    auto targetHeight = resizeStartGeometry_.height();

    switch (resizeEdge_) {
    case EdgeLeft:
        targetWidth -= delta.x();
        break;
    case EdgeRight:
        targetWidth += delta.x();
        break;
    case EdgeTop:
        targetHeight -= delta.y();
        break;
    case EdgeBottom:
        targetHeight += delta.y();
        break;
    case EdgeTopLeft:
        targetWidth -= delta.x();
        targetHeight -= delta.y();
        break;
    case EdgeTopRight:
        targetWidth += delta.x();
        targetHeight -= delta.y();
        break;
    case EdgeBottomLeft:
        targetWidth -= delta.x();
        targetHeight += delta.y();
        break;
    case EdgeBottomRight:
        targetWidth += delta.x();
        targetHeight += delta.y();
        break;
    case EdgeNone:
        break;
    }

    const auto scaleFromWidth = targetWidth / static_cast<double>(imgSize.width());
    const auto scaleFromHeight = targetHeight / static_cast<double>(imgSize.height());
    const auto usesHorizontal = resizeEdge_ == EdgeLeft || resizeEdge_ == EdgeRight;
    const auto usesVertical = resizeEdge_ == EdgeTop || resizeEdge_ == EdgeBottom;
    auto scale = usesHorizontal ? scaleFromWidth : usesVertical ? scaleFromHeight : std::max(scaleFromWidth, scaleFromHeight);
    const auto minScale = std::max(kMinPinSize / static_cast<double>(imgSize.width()),
                                   kMinPinSize / static_cast<double>(imgSize.height()));
    scale = std::max(minScale, std::min(8.0, scale));

    QSize size(static_cast<int>(std::round(imgSize.width() * scale)),
               static_cast<int>(std::round(imgSize.height() * scale)));
    QRect geometry(QPoint(0, 0), size);
    const auto start = resizeStartGeometry_;

    switch (resizeEdge_) {
    case EdgeLeft:
        geometry.moveRight(start.right());
        geometry.moveCenter(QPoint(geometry.center().x(), start.center().y()));
        break;
    case EdgeRight:
        geometry.moveLeft(start.left());
        geometry.moveCenter(QPoint(geometry.center().x(), start.center().y()));
        break;
    case EdgeTop:
        geometry.moveBottom(start.bottom());
        geometry.moveCenter(QPoint(start.center().x(), geometry.center().y()));
        break;
    case EdgeBottom:
        geometry.moveTop(start.top());
        geometry.moveCenter(QPoint(start.center().x(), geometry.center().y()));
        break;
    case EdgeTopLeft:
        geometry.moveBottomRight(start.bottomRight());
        break;
    case EdgeTopRight:
        geometry.moveBottomLeft(start.bottomLeft());
        break;
    case EdgeBottomLeft:
        geometry.moveTopRight(start.topRight());
        break;
    case EdgeBottomRight:
    case EdgeNone:
        geometry.moveTopLeft(start.topLeft());
        break;
    }

    return geometry;
}

void PinWindow::contextMenuEvent(QContextMenuEvent* event)
{
    if (editing_) {
        QMenu menu(this);
        if (editToolManager_.selectedIndex() >= 0) {
            auto* delAction = menu.addAction(tr("Delete\tDel"));
            auto* duplicateAction = menu.addAction(tr("Duplicate"));
            auto* bringForward = menu.addAction(tr("Bring Forward"));
            auto* sendBackward = menu.addAction(tr("Send Backward"));
            menu.addSeparator();
            auto* doneAction = menu.addAction(tr("Done\tEsc"));
            const auto* action = menu.exec(event->globalPos());
            if (action == delAction) {
                editToolManager_.pushUndo();
                editToolManager_.deleteAnnotation(editToolManager_.selectedIndex());
            } else if (action == duplicateAction) {
                editToolManager_.pushUndo();
                editToolManager_.duplicateAnnotation(editToolManager_.selectedIndex());
            } else if (action == bringForward) {
                int sel = editToolManager_.selectedIndex();
                int swap = sel + 1;
                if (swap < editToolManager_.annotationCount()) {
                    editToolManager_.pushUndo();
                    qSwap(editToolManager_.annotationsMut()[sel], editToolManager_.annotationsMut()[swap]);
                    editToolManager_.setSelectedIndex(swap);
                }
            } else if (action == sendBackward) {
                int sel = editToolManager_.selectedIndex();
                int swap = sel - 1;
                if (swap >= 0) {
                    editToolManager_.pushUndo();
                    qSwap(editToolManager_.annotationsMut()[sel], editToolManager_.annotationsMut()[swap]);
                    editToolManager_.setSelectedIndex(swap);
                }
            } else if (action == doneAction) {
                applyEditAndExit();
            }
        } else {
            auto* doneAction = menu.addAction(tr("Done\tEsc"));
            if (menu.exec(event->globalPos()) == doneAction) {
                applyEditAndExit();
            }
        }
        update();
        return;
    }

    if (ocrActive_) {
        QMenu menu(this);
        auto* copySel = menu.addAction(tr("Copy Selected"));
        auto* copyAll = menu.addAction(tr("Copy All Text\tCtrl+C"));
        menu.addSeparator();
        auto* exitOcr = menu.addAction(tr("Exit OCR Recognition\tEsc"));

        const auto* action = menu.exec(event->globalPos());
        if (action == copySel) {
            ocrCopySelected();
        } else if (action == copyAll) {
            ocrCopyAll();
        } else if (action == exitOcr) {
            clearOcrOverlay();
        }
        return;
    }

    QMenu menu(this);
    auto* copyAction = menu.addAction(iconProvider_.icon(IconName::Copy), tr("Copy\tCtrl+C"));
    auto* saveAction = menu.addAction(iconProvider_.icon(IconName::Save), tr("Save\tCtrl+S"));
    auto* saveAsAction = menu.addAction(tr("Save As...\tCtrl+Shift+S"));
    auto* copyColorAction = menu.addAction(tr("Copy Color"));
    menu.addSeparator();
    auto* rotateLeftAction = menu.addAction(iconProvider_.icon(IconName::RotateLeft), tr("Rotate Left"));
    auto* rotateRightAction = menu.addAction(iconProvider_.icon(IconName::RotateRight), tr("Rotate Right"));
    auto* flipHAction = menu.addAction(iconProvider_.icon(IconName::FlipHorizontal), tr("Flip Horizontal"));
    auto* flipVAction = menu.addAction(iconProvider_.icon(IconName::FlipVertical), tr("Flip Vertical"));
    auto* alwaysOnTopAction = menu.addAction(tr("Always on Top\tA"));
    alwaysOnTopAction->setCheckable(true);
    alwaysOnTopAction->setChecked(item_.state.options.alwaysOnTop);
    auto* clickThroughAction = menu.addAction(iconProvider_.icon(IconName::ClickThrough), tr("Click Through"));
    clickThroughAction->setCheckable(true);
    clickThroughAction->setChecked(item_.state.options.clickThrough);
    menu.addSeparator();
    auto* actualSizeAction = menu.addAction(tr("Actual Size (1:1)\tCtrl+0"));
    auto* fitScreenAction = menu.addAction(tr("Fit to Screen\tCtrl+9"));
    menu.addSeparator();
    auto* closeAction = menu.addAction(iconProvider_.icon(IconName::Close), tr("Close"));

    const auto* action = menu.exec(event->globalPos());
    if (action == copyAction) {
        emit copyRequested(renderedImage());
        QToolTip::showText(QCursor::pos(), tr("Copied to clipboard"), this);
    } else if (action == saveAction) {
        emit saveRequested(renderedImage());
    } else if (action == saveAsAction) {
        auto path = QFileDialog::getSaveFileName(this, tr("Save As"), QString(),
            tr("PNG (*.png);;JPEG (*.jpg *.jpeg)"));
        if (!path.isEmpty() && !renderedImage().save(path)) {
            QMessageBox::warning(this, tr("Save Error"), tr("Failed to save image."));
        }
    } else if (action == copyColorAction) {
        auto img = renderedImage();
        if (!img.isNull() && !img.size().isEmpty()) {
            int ix = qBound(0, event->pos().x() * img.width() / width(), img.width() - 1);
            int iy = qBound(0, event->pos().y() * img.height() / height(), img.height() - 1);
            QColor pixel = QColor::fromRgba(img.pixel(ix, iy));
            { const QSignalBlocker blocker(QApplication::clipboard()); QApplication::clipboard()->setText(pixel.name().toUpper()); }
            QToolTip::showText(QCursor::pos(), tr("Copied %1").arg(pixel.name().toUpper()), this);
        }
    } else if (action == rotateLeftAction) {
        rotateBy(-90);
    } else if (action == rotateRightAction) {
        rotateBy(90);
    } else if (action == flipHAction) {
        flipH();
    } else if (action == flipVAction) {
        flipV();
    } else if (action == alwaysOnTopAction) {
        toggleAlwaysOnTop();
    } else if (action == clickThroughAction) {
        toggleClickThrough();
    } else if (action == actualSizeAction) {
        setScale(1.0);
        QRect screenGeo;
        auto* screen = QGuiApplication::screenAt(QCursor::pos());
        if (screen) screenGeo = screen->geometry();
        if (!screenGeo.isNull()) {
            auto pos = screenGeo.center() - rect().center();
            item_.state.position = pos;
            move(pos);
            emitStateChanged();
        }
    } else if (action == fitScreenAction) {
        auto imgSize = item_.state.size;
        if (!imgSize.isValid() || imgSize.isNull()) {
            imgSize = logicalImageSize();
        }
        QRect screenGeo;
        auto* screen = QGuiApplication::screenAt(QCursor::pos());
        if (screen) screenGeo = screen->geometry();
        if (screenGeo.isNull()) return;
        auto availGeo = screen->availableGeometry();
        double sx = static_cast<double>(availGeo.width() - 40) / imgSize.width();
        double sy = static_cast<double>(availGeo.height() - 40) / imgSize.height();
        setScale(std::min(sx, sy));
        auto pos = screenGeo.center() - rect().center();
        item_.state.position = pos;
        move(pos);
        emitStateChanged();
    } else if (action == closeAction) {
        requestClose();
    }
}

void PinWindow::closeEvent(QCloseEvent* event)
{
    if (!closeRequested_) {
        closeRequested_ = true;
        QPointer<PinWindow> self(this);
        emit closeRequested(item_.id);
        if (self.isNull()) {
            return;
        }
    }
    QWidget::closeEvent(event);
}

void PinWindow::enterEvent(QEvent* event)
{
    Q_UNUSED(event)
    hovered_ = true;
    update();
}

void PinWindow::focusInEvent(QFocusEvent* event)
{
    Q_UNUSED(event)
    update();
}

void PinWindow::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event)
    update();
}

void PinWindow::keyPressEvent(QKeyEvent* event)
{
    if (editing_) {
        switch (event->key()) {
        case Qt::Key_Escape:
            toggleEditMode();
            event->accept();
            return;
        case Qt::Key_Z:
            if (event->modifiers().testFlag(Qt::ControlModifier)) {
                if (event->modifiers().testFlag(Qt::ShiftModifier)) {
                    editToolManager_.redo();
                } else {
                    editToolManager_.undo();
                }
                update();
                event->accept();
                return;
            }
            break;
        case Qt::Key_Delete:
            if (editToolManager_.selectedIndex() >= 0) {
                editToolManager_.pushUndo();
                editToolManager_.deleteAnnotation(editToolManager_.selectedIndex());
                update();
                event->accept();
                return;
            }
            break;
        default:
            break;
        }
        QWidget::keyPressEvent(event);
        return;
    }

    if (ocrActive_) {
        switch (event->key()) {
        case Qt::Key_Escape:
            clearOcrOverlay();
            event->accept();
            return;
        case Qt::Key_C:
            if (event->modifiers().testFlag(Qt::ControlModifier)) {
                ocrCopySelected();
                event->accept();
                return;
            }
            break;
        case Qt::Key_A:
            if (event->modifiers().testFlag(Qt::ControlModifier)) {
                ocrSelectedBlocks_.clear();
                for (int i = 0; i < ocrBlocks_.size(); ++i)
                    ocrSelectedBlocks_.insert(i);
                update();
                event->accept();
                return;
            }
            break;
        default:
            break;
        }
    }

    switch (event->key()) {
    case Qt::Key_Escape:
        requestClose();
        return;
    case Qt::Key_C:
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            emit copyRequested(renderedImage());
            QToolTip::showText(QCursor::pos(), tr("Copied to clipboard"), this);
            return;
        }
        break;
    case Qt::Key_S:
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            if (event->modifiers().testFlag(Qt::ShiftModifier)) {
                auto path = QFileDialog::getSaveFileName(this, tr("Save As"), QString(),
                    tr("PNG (*.png);;JPEG (*.jpg *.jpeg)"));
                if (!path.isEmpty() && !renderedImage().save(path)) {
                    QMessageBox::warning(this, tr("Save Error"), tr("Failed to save image."));
                }
            } else {
                emit saveRequested(renderedImage());
            }
            return;
        }
        break;
    case Qt::Key_R:
        rotateBy(event->modifiers().testFlag(Qt::ShiftModifier) ? -90 : 90);
        return;
    case Qt::Key_H:
        flipH();
        return;
    case Qt::Key_V:
        flipV();
        return;
    case Qt::Key_Z:
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            undoTransform();
            QToolTip::showText(QCursor::pos(), tr("Undo transform"), this);
            return;
        }
        break;
    case Qt::Key_T:
        toggleClickThrough();
        return;
    case Qt::Key_A:
        toggleAlwaysOnTop();
        return;
    case Qt::Key_0:
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            setScale(1.0);
            return;
        }
        break;
    case Qt::Key_F1:
    case Qt::Key_Slash:
        QMessageBox::information(static_cast<QWidget*>(window()), tr("Keyboard Shortcuts"),
            tr("<b>Pin Window Controls</b><br><br>"
            "<b>Transform</b><br>"
            "R - Rotate 90&deg;<br>Shift+R - Rotate -90&deg;<br>"
            "H - Flip horizontally<br>V - Flip vertically<br>"
            "Ctrl+Z - Undo transform<br><br>"
            "<b>View</b><br>"
            "Ctrl+0 - Actual size (1:1)<br>Ctrl+9 - Fit to screen<br>"
            "Ctrl+Scroll - Change opacity<br>Scroll - Zoom<br>"
            "Shift+Double-click - Toggle thumbnail mode<br><br>"
            "<b>Actions</b><br>"
            "Ctrl+C - Copy image<br>Ctrl+S - Save<br>"
            "Ctrl+Shift+S - Save As...<br>"
            "Ctrl+Drag - Drag image out<br>"
            "A - Toggle always on top<br>T - Toggle click through<br>"
            "Escape - Close pin window"));
        event->accept();
        return;
    case Qt::Key_9:
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            auto imgSize = item_.state.size;
            if (!imgSize.isValid() || imgSize.isNull()) {
                imgSize = logicalImageSize();
            }
            QRect screenGeo;
            auto* screen = QGuiApplication::screenAt(QCursor::pos());
            if (screen) screenGeo = screen->geometry();
            if (screenGeo.isNull()) break;
            auto availGeo = screen->availableGeometry();
            double sx = static_cast<double>(availGeo.width() - 40) / imgSize.width();
            double sy = static_cast<double>(availGeo.height() - 40) / imgSize.height();
            setScale(std::min(sx, sy));
            auto pos = screenGeo.center() - rect().center();
            item_.state.position = pos;
            move(pos);
            emitStateChanged();
            return;
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void PinWindow::leaveEvent(QEvent* event)
{
    Q_UNUSED(event)
    hovered_ = false;
    hoveredButton_ = -1;
    editHoveredButton_ = -1;
    if (ocrActive_) {
        ocrHoveredBlock_ = -1;
    }
    update();
}

void PinWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->modifiers().testFlag(Qt::ShiftModifier)) {
        thumbnailMode_ = !thumbnailMode_;
        if (thumbnailMode_) {
            fullScale_ = item_.state.transform.scale;
            fullPosition_ = item_.state.position;
            const auto imgSize = logicalImageSize();
            const auto maxDim = std::max(imgSize.width(), imgSize.height());
            if (maxDim > 0) {
                setScale(static_cast<double>(kThumbnailMaxSize) / maxDim);
            }
        } else {
            pushUndoState();
            item_.state.position = fullPosition_;
            item_.state.transform.scale = fullScale_;
            item_.state = normalizedState(item_.state);
            resize(logicalImageSize() * item_.state.transform.scale);
            move(item_.state.position);
            update();
            emitStateChanged();
        }
        event->accept();
        return;
    } else {
        if (std::abs(item_.state.transform.scale - 1.0) < 0.01) {
            auto* screen = QGuiApplication::screenAt(QCursor::pos());
            if (screen) {
                auto screenGeo = screen->geometry();
                auto logicalSize = logicalImageSize();
                double fit = qMin(static_cast<double>(screenGeo.width()) / logicalSize.width(),
                                  static_cast<double>(screenGeo.height()) / logicalSize.height());
                if (fit > 0) {
                    pushUndoState();
                    setScale(fit);
                }
            }
        } else {
            pushUndoState();
            setScale(1.0);
            QRect screenGeo;
            auto* screen = QGuiApplication::screenAt(QCursor::pos());
            if (screen) screenGeo = screen->geometry();
            if (!screenGeo.isNull()) {
                auto pos = screenGeo.center() - rect().center();
                item_.state.position = pos;
                move(pos);
            }
            emitStateChanged();
        }
    }
    update();
    event->accept();
}

void PinWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (editing_ && editToolManager_.drawing()) {
        editToolManager_.updateDrawingStroke(toEditImage(event->pos()));
        update();
    }

    if (editing_ && editToolManager_.moving()) {
        editToolManager_.updateMove(toEditImage(event->pos()));
        update();
    }

    if (editing_ && editToolManager_.resizing()) {
        editToolManager_.updateResize(toEditImage(event->pos()), event->modifiers().testFlag(Qt::ShiftModifier));
        update();
    }

    if (!dragging_ && !resizing_) {
        if (editing_) {
            editHoveredButton_ = EditToolbar::fits(width(), height())
                ? EditToolbar::buttonAt(event->pos(), width()) : -1;
            update();
        }

        if (ocrActive_) {
            int prev = ocrHoveredBlock_;
            ocrHoveredBlock_ = ocrBlockAt(event->pos());
            if (ocrHoveredBlock_ != prev) {
                if (ocrHoveredBlock_ >= 0) {
                    QToolTip::showText(event->globalPos(), ocrBlocks_[ocrHoveredBlock_].text, this);
                } else {
                    QToolTip::hideText();
                }
                update();
            }
        }

        hoveredButton_ = (hovered_ || controlsVisible_) && PinToolbar::fits(width(), height())
            ? PinToolbar::buttonAt(event->pos(), width()) : -1;
        const int btn = hoveredButton_;
        static const char* kTooltipLabels[] = {
            QT_TRANSLATE_NOOP("snappaste::PinWindow", "Close"),
            QT_TRANSLATE_NOOP("snappaste::PinWindow", "Rotate Left"),
            QT_TRANSLATE_NOOP("snappaste::PinWindow", "Rotate Right"),
            QT_TRANSLATE_NOOP("snappaste::PinWindow", "Flip Horizontal"),
            QT_TRANSLATE_NOOP("snappaste::PinWindow", "Flip Vertical"),
            QT_TRANSLATE_NOOP("snappaste::PinWindow", "Copy"),
            QT_TRANSLATE_NOOP("snappaste::PinWindow", "Click Through"),
            QT_TRANSLATE_NOOP("snappaste::PinWindow", "Always on Top"),
            QT_TRANSLATE_NOOP("snappaste::PinWindow", "OCR"),
            QT_TRANSLATE_NOOP("snappaste::PinWindow", "Edit")
        };
        if (btn >= 0 && !ocrActive_ && !editing_) {
            QToolTip::showText(event->globalPos(), tr(kTooltipLabels[btn]), this);
        } else if (btn < 0 && ocrHoveredBlock_ < 0 && !editing_) {
            QToolTip::hideText();
        }
        switch (resizeEdgeAt(event->pos())) {
        case EdgeLeft:
        case EdgeRight:     setCursor(Qt::SizeHorCursor); break;
        case EdgeTop:
        case EdgeBottom:    setCursor(Qt::SizeVerCursor); break;
        case EdgeTopLeft:
        case EdgeBottomRight: setCursor(Qt::SizeFDiagCursor); break;
        case EdgeTopRight:
        case EdgeBottomLeft:  setCursor(Qt::SizeBDiagCursor); break;
        case EdgeNone:      setCursor(ocrActive_ && ocrHoveredBlock_ >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor); break;
        }
    }

    if (dragDropping_) {
        const auto dist = (event->pos() - dragOffset_).manhattanLength();
        if (dist > 5) {
            dragDropping_ = false;
            auto img = renderedImage();
            QString tempPath = QDir::toNativeSeparators(
                QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                + "/snappaste_drag_" + QString::number(QCoreApplication::applicationPid()) + ".png");
            img.save(tempPath, "PNG");
            auto dragPixmap = QPixmap::fromImage(
                img.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            QDrag drag(this);
            auto* mimeData = new QMimeData();
            mimeData->setImageData(QVariant(std::move(img)));
            mimeData->setUrls({QUrl::fromLocalFile(tempPath)});
            drag.setMimeData(mimeData);
            drag.setPixmap(std::move(dragPixmap));
            drag.exec(Qt::CopyAction);
            QFile::remove(tempPath);
        }
        event->accept();
        return;
    }

    if (resizing_) {
        setGeometry(constrainedResizeGeometry(event->globalPos()));
        applyResizeToScale();
        update();
        emitStateChangedThrottled();
        event->accept();
        return;
    }

    if (!dragging_) {
        return;
    }

    {
        auto newPos = event->globalPos() - dragOffset_;
        const auto frame = frameGeometry();
        QScreen* screen = cachedDragScreen_;
        if (!screen || !screen->geometry().contains(frame.center())) {
            screen = QGuiApplication::screenAt(frame.center());
            cachedDragScreen_ = screen;
        }
        if (screen != nullptr) {
            const auto geo = screen->availableGeometry();
            const auto rightEdge = geo.right() - frame.width() + 1;
            const auto bottomEdge = geo.bottom() - frame.height() + 1;
            if (std::abs(newPos.x() - geo.left()) < kSnapThreshold) {
                newPos.setX(geo.left() + kSnapMargin);
            } else if (std::abs(newPos.x() - rightEdge) < kSnapThreshold) {
                newPos.setX(rightEdge - kSnapMargin);
            }
            if (std::abs(newPos.y() - geo.top()) < kSnapThreshold) {
                newPos.setY(geo.top() + kSnapMargin);
            } else if (std::abs(newPos.y() - bottomEdge) < kSnapThreshold) {
                newPos.setY(bottomEdge - kSnapMargin);
            }
        }
        move(newPos);
        item_.state.position = frameGeometry().topLeft();
    }
    emitStateChangedThrottled();
    event->accept();
}

void PinWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    const auto pos = event->pos();

    if (editing_) {
        if (EditToolbar::fits(width(), height())) {
            const int btn = EditToolbar::buttonAt(pos, width());
            if (btn >= 0) {
                if (btn < 9) {
                    editToolManager_.setTool(EditToolbar::toolAt(btn));
                    setCursor(Qt::CrossCursor);
                } else if (btn == 9) {
                    editToolManager_.undo();
                } else if (btn == 10) {
                    editToolManager_.redo();
                } else if (btn == 11) {
                    applyEditAndExit();
                }
                update();
                event->accept();
                return;
            }
        }

        QPoint imgPt = toEditImage(pos);
        if (editToolManager_.currentTool() == AnnotationTool::Select) {
            for (int i = editToolManager_.annotationCount() - 1; i >= 0; --i) {
                if (AnnotationRenderer::hitTestAnnotation(editToolManager_.annotationAt(i), imgPt)) {
                    editToolManager_.selectAnnotation(i);
                    update();
                    event->accept();
                    return;
                }
            }
            editToolManager_.setSelectedIndex(-1);
            update();
        } else {
            editToolManager_.pushUndo();
            editToolManager_.startDrawing(imgPt);
        }
        event->accept();
        return;
    }

    if (ocrActive_) {
        int idx = ocrBlockAt(pos);
        if (idx >= 0) {
            if (ocrSelectedBlocks_.contains(idx)) {
                ocrSelectedBlocks_.remove(idx);
            } else {
                ocrSelectedBlocks_.insert(idx);
            }
            update();
            event->accept();
            return;
        }
        clearOcrOverlay();
    }

    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        dragDropping_ = true;
        dragOffset_ = pos;
        event->accept();
        return;
    }

    if (hovered_ || controlsVisible_) {
        if (PinToolbar::fits(width(), height())) {
            const int btn = PinToolbar::buttonAt(pos, width());
            if (btn >= 0) {
                switch (btn) {
                case 0: requestClose(); break;
                case 1: rotateBy(-90); break;
                case 2: rotateBy(90); break;
                case 3: flipH(); break;
                case 4: flipV(); break;
                case 5: emit copyRequested(renderedImage()); break;
                case 6: toggleClickThrough(); break;
                case 7: toggleAlwaysOnTop(); break;
                case 8: triggerOcr(); break;
                case 9: toggleEditMode(); break;
                }
                event->accept();
                return;
            }
        } else if (PinToolbar::overflowRect(width(), height()).contains(pos)) {
            QMenu menu(this);
            auto* copyAction = menu.addAction(iconProvider_.icon(IconName::Copy), tr("Copy\tCtrl+C"));
            auto* saveAction = menu.addAction(iconProvider_.icon(IconName::Save), tr("Save\tCtrl+S"));
            menu.addSeparator();
            auto* rotateLeftAction = menu.addAction(iconProvider_.icon(IconName::RotateLeft), tr("Rotate Left"));
            auto* rotateRightAction = menu.addAction(iconProvider_.icon(IconName::RotateRight), tr("Rotate Right"));
            auto* flipHAction = menu.addAction(iconProvider_.icon(IconName::FlipHorizontal), tr("Flip Horizontal"));
            auto* flipVAction = menu.addAction(iconProvider_.icon(IconName::FlipVertical), tr("Flip Vertical"));
            menu.addSeparator();
            auto* alwaysOnTopAction = menu.addAction(tr("Always on Top\tA"));
            alwaysOnTopAction->setCheckable(true);
            alwaysOnTopAction->setChecked(item_.state.options.alwaysOnTop);
            auto* clickThroughAction = menu.addAction(iconProvider_.icon(IconName::ClickThrough), tr("Click Through"));
            clickThroughAction->setCheckable(true);
            clickThroughAction->setChecked(item_.state.options.clickThrough);
            menu.addSeparator();
            auto* closeAction = menu.addAction(iconProvider_.icon(IconName::Close), tr("Close\tEsc"));

            const auto* action = menu.exec(event->globalPos());
            if (action == copyAction) {
                emit copyRequested(renderedImage());
            } else if (action == saveAction) {
                emit saveRequested(renderedImage());
            } else if (action == rotateLeftAction) {
                rotateBy(-90);
            } else if (action == rotateRightAction) {
                rotateBy(90);
            } else if (action == flipHAction) {
                flipH();
            } else if (action == flipVAction) {
                flipV();
            } else if (action == alwaysOnTopAction) {
                toggleAlwaysOnTop();
            } else if (action == clickThroughAction) {
                toggleClickThrough();
            } else if (action == closeAction) {
                requestClose();
            }
            event->accept();
            return;
        }
    }

    const auto edge = resizeEdgeAt(pos);
    if (edge != EdgeNone) {
        pushUndoState();
        resizing_ = true;
        resizeEdge_ = edge;
        resizeStartGeometry_ = frameGeometry();
        resizeStartGlobal_ = event->globalPos();
        event->accept();
        return;
    }

    dragging_ = true;
    dragOffset_ = event->globalPos() - frameGeometry().topLeft();
    cachedDragScreen_ = QGuiApplication::screenAt(frameGeometry().center());
    event->accept();
}

void PinWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (editing_) {
        if (editToolManager_.drawing()) {
            editToolManager_.finishDrawing();
        }
        editToolManager_.setMoving(false);
        editToolManager_.setResizing(false);
        update();
        event->accept();
        return;
    }

    dragDropping_ = false;
    dragging_ = false;
    resizing_ = false;
    setCursor(Qt::ArrowCursor);
    event->accept();
}

void PinWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);

    auto rotation = item_.state.transform.rotationDegrees;
    bool flippedH = item_.state.transform.flippedHorizontally;
    bool flippedV = item_.state.transform.flippedVertically;
    if (rotation != 0 || flippedH || flippedV) {
        painter.save();
        auto r = rect();
        auto center = r.center();
        painter.translate(center);
        painter.rotate(rotation);
        painter.scale(flippedH ? -1.0 : 1.0, flippedV ? -1.0 : 1.0);
        painter.translate(-center);
        painter.drawImage(r, item_.image);
        painter.restore();
    } else {
        painter.drawImage(rect(), item_.image);
    }

    if (thumbnailMode_) {
        painter.setPen(kPinAccent);
        painter.setFont(kThumbnailFont());
        painter.drawText(rect().adjusted(8, 8, -8, -8), Qt::AlignTop | Qt::AlignLeft, tr("T"));
    }

    const int zoomPct = static_cast<int>(std::round(item_.state.transform.scale * 100));
    if (zoomPct != 100 || thumbnailMode_) {
        painter.setPen(kZoomTextColor);
        painter.setFont(kZoomFont());
        painter.drawText(rect().adjusted(8, 8, -8, -8),
            Qt::AlignTop | Qt::AlignRight,
            tr("%1%").arg(zoomPct));
    }

    if (editing_) {
        if (!editToolManager_.image().isNull()) {
            painter.save();
            double zf = editToolManager_.zoomFactor();
            painter.scale(zf, zf);
            editRenderer_.drawAnnotations(painter, editToolManager_.image(),
                                          editToolManager_.annotations(),
                                          editToolManager_.fontSize());
            if (editToolManager_.drawing()) {
                editRenderer_.drawDraft(painter, editToolManager_.image(),
                                        editToolManager_.draft(),
                                        editToolManager_.fontSize());
            }
            painter.restore();
        }

        bool showEditTools = hovered_ || hasFocus();
        if (showEditTools && EditToolbar::fits(width(), height())) {
            EditToolbar::draw(painter, width(), height(), iconProvider_,
                              editHoveredButton_,
                              editToolManager_.currentTool());
        }
    }

    if (!editing_) {
        if (ocrActive_ && !ocrBlocks_.isEmpty()) {
            for (int i = 0; i < ocrBlockWidgetRects_.size(); ++i) {
                const auto& r = ocrBlockWidgetRects_[i];
                bool hover = (i == ocrHoveredBlock_);
                bool sel = ocrSelectedBlocks_.contains(i);

                if (hover) {
                    painter.fillRect(r, kOcrBlockHoverFill);
                    painter.setPen(QPen(kOcrBlockHoverBorder, 2));
                } else if (sel) {
                    painter.fillRect(r, kOcrBlockSelectedFill);
                    painter.setPen(QPen(kOcrBlockSelectedBorder, 2));
                } else {
                    painter.fillRect(r, kOcrBlockFill);
                    painter.setPen(QPen(kOcrBlockBorder, 1));
                }
                painter.drawRect(r.adjusted(0, 0, -1, -1));
            }

            painter.setPen(kZoomTextColor);
            painter.setFont(kOcrInfoFont());
            int selCount = ocrSelectedBlocks_.size();
            if (selCount > 0) {
                painter.drawText(rect().adjusted(8, 8, -8, -8),
                    Qt::AlignBottom | Qt::AlignLeft,
                    tr("OCR - %1/%2 selected").arg(selCount).arg(ocrBlocks_.size()));
            } else {
                painter.drawText(rect().adjusted(8, 8, -8, -8),
                    Qt::AlignBottom | Qt::AlignLeft,
                    tr("OCR - %1 blocks  (Esc to exit)").arg(ocrBlocks_.size()));
            }
        }

        if (item_.state.options.clickThrough) {
            QPen dashPen(kPinAccent, 2, Qt::DashLine);
            dashPen.setDashPattern({6, 4});
            painter.setPen(dashPen);
            painter.setBrush(kClickThroughFill);
            painter.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 5, 5);
        }

        const auto showControls = hovered_ || hasFocus() || controlsVisible_;
        if (showControls && !ocrActive_) {
            painter.fillRect(rect(), kControlOverlay);
            painter.setPen(QPen(kPinAccent, 2));
            painter.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 5, 5);
            painter.setPen(QPen(kControlBorder, 1));
            painter.drawRoundedRect(rect().adjusted(4, 4, -5, -5), 3, 3);

            if (PinToolbar::fits(width(), height())) {
                PinToolbar::draw(painter, width(), height(), iconProvider_,
                                 showControls ? hoveredButton_ : -1,
                                 item_.state.options.clickThrough,
                                 item_.state.options.alwaysOnTop);
            } else {
                const auto ob = PinToolbar::overflowRect(width(), height());
                painter.setBrush(kOverflowBg);
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(ob, 3, 3);
                painter.setPen(kOverflowText);
                painter.setFont(kOverflowDotsFont);
                painter.drawText(ob, Qt::AlignCenter, tr("..."));
            }
        }
    }
}

void PinWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (ocrActive_) {
        rebuildOcrBlockRects();
    }
}

void PinWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!firstShow_) {
        return;
    }

    firstShow_ = false;
    const auto target = geometry();
    auto start = target;
    start.setSize(QSize(std::max(1, static_cast<int>(target.width() * 0.96)),
                        std::max(1, static_cast<int>(target.height() * 0.96))));
    start.moveCenter(target.center());
    showAnimation_->stop();
    showAnimation_->setStartValue(start);
    showAnimation_->setEndValue(target);
    showAnimation_->start();
}

void PinWindow::wheelEvent(QWheelEvent* event)
{
    const auto delta = event->angleDelta().y();
    if (delta == 0) {
        event->ignore();
        return;
    }

    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        setOpacityValue(item_.state.opacity + (delta > 0 ? 0.05 : -0.05));
    } else {
        const auto oldScale = item_.state.transform.scale;
        pushUndoState();
        item_.state.transform.scale *= delta > 0 ? 1.08 : 0.92;
        item_.state = normalizedState(item_.state);
        const auto newSize = item_.state.size * item_.state.transform.scale;
        const auto factor = item_.state.transform.scale / oldScale;
        const auto cursorOffset = event->globalPos() - frameGeometry().topLeft();
        const auto newPos = event->globalPos() - QPoint(static_cast<int>(cursorOffset.x() * factor),
                                                         static_cast<int>(cursorOffset.y() * factor));
        setGeometry(QRect(newPos, newSize));
        item_.state.position = frameGeometry().topLeft();
        update();
        emitStateChangedThrottled();
    }
    event->accept();
}

void PinWindow::applyState()
{
    item_.state = normalizedState(item_.state);
    applyWindowFlags();
    resize(logicalImageSize() * item_.state.transform.scale);
    move(item_.state.position);
    setWindowOpacity(item_.state.opacity);
    windowInteraction_.setClickThrough(this, item_.state.options.clickThrough);
}

void PinWindow::applyWindowFlags()
{
    const auto wasVisible = isVisible();
    Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::Tool;
    if (item_.state.options.alwaysOnTop) {
        flags |= Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);
    if (wasVisible) {
        show();
        raise();
    }
}

void PinWindow::emitStateChanged()
{
    stateEmitLimiter_.invalidate();
    item_.state.position = frameGeometry().topLeft();
    item_.state.size = logicalImageSize();
    item_.state.devicePixelRatio = item_.image.devicePixelRatio();
    item_.state = normalizedState(item_.state);
    setWindowOpacity(item_.state.opacity);
    emit stateChanged(item_.id, item_.state);
}

void PinWindow::emitStateChangedThrottled()
{
    if (stateEmitLimiter_.isValid() && stateEmitLimiter_.elapsed() < kStateEmitIntervalMs) {
        return;
    }
    stateEmitLimiter_.restart();
    item_.state.position = frameGeometry().topLeft();
    item_.state.size = logicalImageSize();
    item_.state.devicePixelRatio = item_.image.devicePixelRatio();
    item_.state = normalizedState(item_.state);
    emit stateChanged(item_.id, item_.state);
}

void PinWindow::requestClose()
{
    if (closeRequested_) {
        close();
        return;
    }

    closeRequested_ = true;
    QPointer<PinWindow> self(this);
    emit closeRequested(item_.id);
    if (!self.isNull()) {
        close();
    }
}

void PinWindow::pushUndoState()
{
    undoStack_.push_back(item_.state);
    if (undoStack_.size() > kMaxPinUndo) {
        undoStack_.removeFirst();
    }
}

void PinWindow::undoTransform()
{
    if (undoStack_.isEmpty()) return;
    item_.state = undoStack_.takeLast();
    item_.state = normalizedState(item_.state);
    invalidateRenderedCache();
    resize(logicalImageSize() * item_.state.transform.scale);
    applyWindowFlags();
    update();
    emitStateChanged();
}

void PinWindow::rotateBy(int degrees)
{
    pushUndoState();
    item_.state.transform.rotationDegrees += degrees;
    item_.state = normalizedState(item_.state);
    invalidateRenderedCache();
    resize(logicalImageSize() * item_.state.transform.scale);
    update();
    emitStateChanged();
    QToolTip::showText(QCursor::pos(), tr("Rotated %1\xC2\xB0").arg(degrees), this);
}

void PinWindow::setScale(double scale)
{
    pushUndoState();
    item_.state.transform.scale = scale;
    item_.state = normalizedState(item_.state);
    resize(item_.state.size * item_.state.transform.scale);
    update();
    emitStateChanged();
}

void PinWindow::setOpacityValue(double opacity)
{
    pushUndoState();
    item_.state.opacity = opacity;
    item_.state = normalizedState(item_.state);
    setWindowOpacity(item_.state.opacity);
    emitStateChanged();
}

void PinWindow::flipH()
{
    pushUndoState();
    item_.state.transform.flippedHorizontally = !item_.state.transform.flippedHorizontally;
    invalidateRenderedCache();
    update();
    emitStateChanged();
    QToolTip::showText(QCursor::pos(), tr("Flipped horizontally"), this);
}

void PinWindow::flipV()
{
    pushUndoState();
    item_.state.transform.flippedVertically = !item_.state.transform.flippedVertically;
    invalidateRenderedCache();
    update();
    emitStateChanged();
    QToolTip::showText(QCursor::pos(), tr("Flipped vertically"), this);
}

void PinWindow::toggleAlwaysOnTop()
{
    item_.state.options.alwaysOnTop = !item_.state.options.alwaysOnTop;
    applyWindowFlags();
    emitStateChanged();
}

void PinWindow::toggleClickThrough()
{
    item_.state.options.clickThrough = !item_.state.options.clickThrough;
    windowInteraction_.setClickThrough(this, item_.state.options.clickThrough);
    emitStateChanged();
}

void PinWindow::triggerOcr()
{
    if (ocrActive_) {
        clearOcrOverlay();
        return;
    }
    emit ocrRequested(item_.id, item_.image);
}

void PinWindow::setOcrResult(OcrResult result)
{
    if (!result.ok || result.blocks.isEmpty()) {
        return;
    }

    ocrActive_ = true;
    ocrBlocks_ = std::move(result.blocks);
    ocrFullText_ = std::move(result.text);
    ocrHoveredBlock_ = -1;
    ocrSelectedBlocks_.clear();
    rebuildOcrBlockRects();
    controlsVisible_ = false;
    hovered_ = false;
    update();
}

void PinWindow::clearOcrOverlay()
{
    ocrActive_ = false;
    ocrBlocks_.clear();
    ocrBlockWidgetRects_.clear();
    ocrFullText_.clear();
    ocrHoveredBlock_ = -1;
    ocrSelectedBlocks_.clear();
    update();
}

void PinWindow::rebuildOcrBlockRects()
{
    ocrBlockWidgetRects_.resize(ocrBlocks_.size());

    QTransform transform;
    transform.rotate(item_.state.transform.rotationDegrees);
    transform.scale(item_.state.transform.flippedHorizontally ? -1.0 : 1.0,
                    item_.state.transform.flippedVertically ? -1.0 : 1.0);

    const auto rdr = renderedImage();
    if (rdr.isNull()) return;
    double sx = static_cast<double>(width()) / rdr.width();
    double sy = static_cast<double>(height()) / rdr.height();

    for (int i = 0; i < ocrBlocks_.size(); ++i) {
        auto mappedRect = transform.mapRect(ocrBlocks_[i].rect);
        ocrBlockWidgetRects_[i] = QRect(
            static_cast<int>(mappedRect.x() * sx),
            static_cast<int>(mappedRect.y() * sy),
            static_cast<int>(mappedRect.width() * sx),
            static_cast<int>(mappedRect.height() * sy)
        ).intersected(rect());
    }
}

int PinWindow::ocrBlockAt(const QPoint& pos) const
{
    for (int i = ocrBlockWidgetRects_.size() - 1; i >= 0; --i) {
        if (ocrBlockWidgetRects_[i].contains(pos)) {
            return i;
        }
    }
    return -1;
}

void PinWindow::ocrCopySelected()
{
    if (ocrSelectedBlocks_.isEmpty()) {
        for (int i = 0; i < ocrBlocks_.size(); ++i)
            ocrSelectedBlocks_.insert(i);
    }

    QStringList parts;
    QVector<int> sorted;
    sorted.reserve(ocrSelectedBlocks_.size());
    for (int idx : ocrSelectedBlocks_)
        sorted.append(idx);
    std::sort(sorted.begin(), sorted.end());
    for (int i : sorted) {
        parts.push_back(ocrBlocks_[i].text);
    }
    QString text = parts.join("\n");
    if (text.isEmpty()) return;

    { const QSignalBlocker blocker(QApplication::clipboard()); QApplication::clipboard()->setText(text); }
    QToolTip::showText(QCursor::pos(), tr("Copied %1 characters").arg(text.length()), this);
}

void PinWindow::ocrCopyAll()
{
    if (ocrFullText_.isEmpty()) return;
    { const QSignalBlocker blocker(QApplication::clipboard()); QApplication::clipboard()->setText(ocrFullText_); }
    QToolTip::showText(QCursor::pos(), tr("Copied all %1 characters").arg(ocrFullText_.length()), this);
}

QImage PinWindow::renderedImage() const
{
    if (cachedRenderedVersion_ != renderedVersion_) {
        QTransform transform;
        transform.rotate(item_.state.transform.rotationDegrees);
        transform.scale(item_.state.transform.flippedHorizontally ? -1.0 : 1.0,
                        item_.state.transform.flippedVertically ? -1.0 : 1.0);
        renderedCache_ = item_.image.transformed(transform, Qt::SmoothTransformation);
        renderedCache_.setDevicePixelRatio(item_.image.devicePixelRatio());
        cachedRenderedVersion_ = renderedVersion_;
    }
    return renderedCache_;
}

QSize PinWindow::logicalImageSize() const
{
    auto img = item_.image;
    return img.size() / img.devicePixelRatio();
}

void PinWindow::invalidateRenderedCache()
{
    ++renderedVersion_;
}

void PinWindow::toggleEditMode()
{
    if (editing_) {
        // Cancel edit — discard annotations, restore original state
        editing_ = false;
        editToolManager_.clearAnnotations();
        item_.state = savedEditState_;
        invalidateRenderedCache();
        applyState();
        update();
    } else {
        clearOcrOverlay();
        savedEditState_ = item_.state;
        item_.state.transform.rotationDegrees = 0;
        item_.state.transform.flippedHorizontally = false;
        item_.state.transform.flippedVertically = false;
        invalidateRenderedCache();
        resize(logicalImageSize() * item_.state.transform.scale);
        editToolManager_.setImage(item_.image, item_.state.transform.scale);
        editing_ = true;
        editHoveredButton_ = -1;
        update();
    }
}

void PinWindow::applyEditAndExit()
{
    if (!editing_) return;
    if (editToolManager_.annotationCount() > 0) {
        item_.image = editRenderer_.renderToImage(
            item_.image, editToolManager_.annotations(), editToolManager_.fontSize());
    }
    item_.state.transform.rotationDegrees = savedEditState_.transform.rotationDegrees;
    item_.state.transform.flippedHorizontally = savedEditState_.transform.flippedHorizontally;
    item_.state.transform.flippedVertically = savedEditState_.transform.flippedVertically;
    editing_ = false;
    editToolManager_.clearAnnotations();
    savedEditState_ = PinnedImageState();
    invalidateRenderedCache();
    applyState();
    emitStateChanged();
    update();
}

QPoint PinWindow::toEditImage(QPoint widgetPt) const
{
    double zf = editToolManager_.zoomFactor();
    return QPoint(static_cast<int>(widgetPt.x() / zf),
                  static_cast<int>(widgetPt.y() / zf));
}

} // namespace snappaste
