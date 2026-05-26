#include "presentation/pin_window/PinWindow.h"

#include "presentation/icons/IconProvider.h"

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
#include <QMouseEvent>
#include <QPainter>
#include <QEasingCurve>
#include <QPointer>
#include <QScreen>
#include <QShowEvent>
#include <QToolTip>
#include <QTransform>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <utility>

namespace snappaste {

namespace {

constexpr int kResizeMargin = 6;
constexpr int kToolbarHeight = 28;
constexpr int kToolbarBtnSize = 20;
constexpr int kToolbarIconSize = 14;
constexpr int kToolbarBtnPad = 4;
constexpr int kToolbarButtonCount = 7;
constexpr int kMinPinSize = 40;
constexpr int kThumbnailMaxSize = 200;

} // namespace

PinWindow::PinWindow(PinnedItem item, QWidget* parent)
    : QWidget(parent)
    , item_(std::move(item))
{
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
    item_.state.options.clickThrough = false;
    windowInteraction_.setClickThrough(this, false);
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
    const auto imgSize = item_.image.size();
    if (imgSize.isEmpty()) return;
    const auto winSize = frameGeometry().size();
    item_.state.transform.scale = std::max(0.1, std::min(8.0,
        static_cast<double>(winSize.width()) / imgSize.width()));
    item_.state = normalizedState(item_.state);
}

QRect PinWindow::constrainedResizeGeometry(const QPoint& globalPos) const
{
    const auto imgSize = item_.image.size();
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

QRect PinWindow::toolbarRect() const
{
    const auto w = width();
    const auto tbWidth = kToolbarButtonCount * (kToolbarBtnSize + kToolbarBtnPad) + kToolbarBtnPad;
    return QRect((w - tbWidth) / 2, kToolbarBtnPad, tbWidth, kToolbarHeight);
}

QVector<QRect> PinWindow::toolbarButtonRects() const
{
    const auto tb = toolbarRect();
    QVector<QRect> rects;
    rects.reserve(kToolbarButtonCount);
    int x = tb.left() + kToolbarBtnPad;
    for (int i = 0; i < kToolbarButtonCount; ++i) {
        rects.append(QRect(x, tb.top() + (tb.height() - kToolbarBtnSize) / 2,
                           kToolbarBtnSize, kToolbarBtnSize));
        x += kToolbarBtnSize + kToolbarBtnPad;
    }
    return rects;
}

bool PinWindow::toolbarFits() const
{
    return width() >= toolbarRect().width() && height() >= toolbarRect().bottom() + kToolbarBtnPad;
}

void PinWindow::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    auto* copyAction = menu.addAction(IconProvider::icon(IconName::Copy), "Copy\tCtrl+C");
    auto* saveAction = menu.addAction(IconProvider::icon(IconName::Save), "Save\tCtrl+S");
    auto* saveAsAction = menu.addAction("Save As...\tCtrl+Shift+S");
    auto* copyColorAction = menu.addAction("Copy Color");
    menu.addSeparator();
    auto* rotateLeftAction = menu.addAction(IconProvider::icon(IconName::RotateLeft), "Rotate Left");
    auto* rotateRightAction = menu.addAction(IconProvider::icon(IconName::RotateRight), "Rotate Right");
    auto* flipHAction = menu.addAction(IconProvider::icon(IconName::FlipHorizontal), "Flip Horizontal");
    auto* flipVAction = menu.addAction(IconProvider::icon(IconName::FlipVertical), "Flip Vertical");
    auto* alwaysOnTopAction = menu.addAction("Always on Top\tA");
    alwaysOnTopAction->setCheckable(true);
    alwaysOnTopAction->setChecked(item_.state.options.alwaysOnTop);
    auto* clickThroughAction = menu.addAction(IconProvider::icon(IconName::ClickThrough), "Click Through");
    clickThroughAction->setCheckable(true);
    clickThroughAction->setChecked(item_.state.options.clickThrough);
    menu.addSeparator();
    auto* actualSizeAction = menu.addAction("Actual Size (1:1)\tCtrl+0");
    auto* fitScreenAction = menu.addAction("Fit to Screen\tCtrl+9");
    menu.addSeparator();
    auto* closeAction = menu.addAction(IconProvider::icon(IconName::Close), "Close");

    const auto* action = menu.exec(event->globalPos());
    if (action == copyAction) {
        emit copyRequested(renderedImage());
        QToolTip::showText(QCursor::pos(), "Copied to clipboard", this);
    } else if (action == saveAction) {
        emit saveRequested(renderedImage());
    } else if (action == saveAsAction) {
        auto path = QFileDialog::getSaveFileName(this, "Save As", QString(),
            "PNG (*.png);;JPEG (*.jpg *.jpeg)");
        if (!path.isEmpty()) {
            renderedImage().save(path);
        }
    } else if (action == copyColorAction) {
        auto img = renderedImage();
        if (!img.isNull() && !img.size().isEmpty()) {
            int ix = qBound(0, event->pos().x() * img.width() / width(), img.width() - 1);
            int iy = qBound(0, event->pos().y() * img.height() / height(), img.height() - 1);
            QColor pixel = QColor::fromRgba(img.pixel(ix, iy));
            QApplication::clipboard()->setText(pixel.name().toUpper());
            QToolTip::showText(QCursor::pos(), "Copied " + pixel.name().toUpper(), this);
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
        }
        emitStateChanged();
    } else if (action == fitScreenAction) {
        auto imgSize = item_.state.size;
        if (!imgSize.isValid() || imgSize.isNull()) {
            imgSize = renderedImage().size();
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
    switch (event->key()) {
    case Qt::Key_Escape:
        requestClose();
        return;
    case Qt::Key_C:
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            emit copyRequested(renderedImage());
            QToolTip::showText(QCursor::pos(), "Copied to clipboard", this);
            return;
        }
        break;
    case Qt::Key_S:
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            if (event->modifiers().testFlag(Qt::ShiftModifier)) {
                auto path = QFileDialog::getSaveFileName(this, "Save As", QString(),
                    "PNG (*.png);;JPEG (*.jpg *.jpeg)");
                if (!path.isEmpty()) {
                    renderedImage().save(path);
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
            QToolTip::showText(QCursor::pos(), "Undo transform", this);
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
    case Qt::Key_9:
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            auto imgSize = item_.state.size;
            if (!imgSize.isValid() || imgSize.isNull()) {
                imgSize = renderedImage().size();
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
    update();
}

void PinWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->modifiers().testFlag(Qt::ShiftModifier)) {
        thumbnailMode_ = !thumbnailMode_;
        if (thumbnailMode_) {
            fullScale_ = item_.state.transform.scale;
            fullPosition_ = item_.state.position;
            const auto imgSize = renderedImage().size();
            const auto maxDim = std::max(imgSize.width(), imgSize.height());
            if (maxDim > 0) {
                setScale(static_cast<double>(kThumbnailMaxSize) / maxDim);
            }
        } else {
            item_.state.position = fullPosition_;
            item_.state.transform.scale = fullScale_;
            item_.state = normalizedState(item_.state);
            resize(renderedImage().size() * item_.state.transform.scale);
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
                double fit = qMin(static_cast<double>(screenGeo.width()) / renderedImage().width(),
                                  static_cast<double>(screenGeo.height()) / renderedImage().height());
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
    if (!dragging_ && !resizing_) {
        bool onToolbar = false;
        if ((hovered_ || controlsVisible_) && toolbarFits()) {
            const auto btns = toolbarButtonRects();
            static const char* kTooltipLabels[] = {
                "Close", "Rotate Left", "Rotate Right",
                "Flip Horizontal", "Flip Vertical",
                "Click Through", "Always on Top"
            };
            for (int i = 0; i < btns.size(); ++i) {
                if (btns[i].contains(event->pos())) {
                    QToolTip::showText(event->globalPos(), kTooltipLabels[i], this);
                    onToolbar = true;
                    break;
                }
            }
        }
        if (!onToolbar) {
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
        case EdgeNone:      setCursor(Qt::ArrowCursor); break;
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
            QDrag drag(this);
            auto* mimeData = new QMimeData();
            mimeData->setImageData(QVariant(img));
            mimeData->setUrls({QUrl::fromLocalFile(tempPath)});
            drag.setMimeData(mimeData);
            drag.setPixmap(QPixmap::fromImage(
                img.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            drag.exec(Qt::CopyAction);
        }
        event->accept();
        return;
    }

    if (resizing_) {
        setGeometry(constrainedResizeGeometry(event->globalPos()));
        applyResizeToScale();
        update();
        emitStateChanged();
        event->accept();
        return;
    }

    if (!dragging_) {
        return;
    }

    move(event->globalPos() - dragOffset_);
    item_.state.position = frameGeometry().topLeft();
    emitStateChanged();
    event->accept();
}

void PinWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    const auto pos = event->pos();

    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        dragDropping_ = true;
        dragOffset_ = pos;
        event->accept();
        return;
    }

    if ((hovered_ || controlsVisible_) && toolbarFits()) {
        const auto btns = toolbarButtonRects();
        for (int i = 0; i < btns.size(); ++i) {
            if (btns[i].contains(pos)) {
                switch (i) {
                case 0:
                    requestClose();
                    break;
                case 1: rotateBy(-90); break;
                case 2: rotateBy(90); break;
                case 3: flipH(); break;
                case 4: flipV(); break;
                case 5: toggleClickThrough(); break;
                case 6: toggleAlwaysOnTop(); break;
                }
                event->accept();
                return;
            }
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
    event->accept();
}

void PinWindow::mouseReleaseEvent(QMouseEvent* event)
{
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
    painter.drawImage(rect(), renderedImage());

    if (thumbnailMode_) {
        painter.setPen(QColor("#31c7a4"));
        painter.setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter.drawText(rect().adjusted(8, 8, -8, -8), Qt::AlignTop | Qt::AlignLeft, "T");
    }

    const auto showControls = hovered_ || hasFocus() || controlsVisible_;
    if (showControls) {
        painter.fillRect(rect(), QColor(20, 26, 33, 28));
        painter.setPen(QPen(QColor("#31c7a4"), 2));
        painter.drawRoundedRect(rect().adjusted(1, 1, -2, -2), 5, 5);
        painter.setPen(QPen(QColor(255, 255, 255, 110), 1));
        painter.drawRoundedRect(rect().adjusted(4, 4, -5, -5), 3, 3);

        if (toolbarFits()) {
            const auto tb = toolbarRect();
            painter.setBrush(QColor(20, 26, 33, 200));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(tb, 4, 4);
            const auto btns = toolbarButtonRects();
            const IconName icons[] = {
                IconName::Close,
                IconName::RotateLeft,
                IconName::RotateRight,
                IconName::FlipHorizontal,
                IconName::FlipVertical,
                IconName::ClickThrough,
                IconName::Pin
            };
            for (int i = 0; i < btns.size(); ++i) {
                painter.fillRect(btns[i], QColor(255, 255, 255, 24));
                const auto pixmap = IconProvider::icon(icons[i]).pixmap(kToolbarIconSize, kToolbarIconSize);
                const auto iconTopLeft = btns[i].center() - QPoint(kToolbarIconSize / 2, kToolbarIconSize / 2);
                painter.drawPixmap(iconTopLeft, pixmap);
            }
        }
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
        emitStateChanged();
    }
    event->accept();
}

void PinWindow::applyState()
{
    item_.state = normalizedState(item_.state);
    applyWindowFlags();
    resize(renderedImage().size() * item_.state.transform.scale);
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
    item_.state.position = frameGeometry().topLeft();
    item_.state.size = renderedImage().size();
    item_.state = normalizedState(item_.state);
    setWindowOpacity(item_.state.opacity);
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
    resize(renderedImage().size() * item_.state.transform.scale);
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
    resize(renderedImage().size() * item_.state.transform.scale);
    update();
    emitStateChanged();
    QToolTip::showText(QCursor::pos(), QString("Rotated %1\xC2\xB0").arg(degrees), this);
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
    item_.state.opacity = opacity;
    item_.state = normalizedState(item_.state);
    setWindowOpacity(item_.state.opacity);
    emitStateChanged();
}

void PinWindow::invalidateRenderedCache()
{
    renderedCache_ = {};
}

void PinWindow::flipH()
{
    pushUndoState();
    item_.state.transform.flippedHorizontally = !item_.state.transform.flippedHorizontally;
    invalidateRenderedCache();
    update();
    emitStateChanged();
    QToolTip::showText(QCursor::pos(), "Flipped horizontally", this);
}

void PinWindow::flipV()
{
    pushUndoState();
    item_.state.transform.flippedVertically = !item_.state.transform.flippedVertically;
    invalidateRenderedCache();
    update();
    emitStateChanged();
    QToolTip::showText(QCursor::pos(), "Flipped vertically", this);
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

QImage PinWindow::renderedImage() const
{
    if (renderedCache_.isNull()) {
        QTransform transform;
        transform.rotate(item_.state.transform.rotationDegrees);
        transform.scale(item_.state.transform.flippedHorizontally ? -1.0 : 1.0,
                        item_.state.transform.flippedVertically ? -1.0 : 1.0);
        renderedCache_ = item_.image.transformed(transform, Qt::SmoothTransformation);
    }
    return renderedCache_;
}

} // namespace snappaste
