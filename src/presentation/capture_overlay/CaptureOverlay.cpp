#include "presentation/capture_overlay/CaptureOverlay.h"

#include "domain/capture/CaptureSelectionHistory.h"
#include "domain/capture/IScreenPixelSampler.h"
#include "domain/capture/IScreenRegionDetector.h"
#include "presentation/capture_actions/CaptureActionBar.h"

#include <QApplication>
#include <QClipboard>
#include <QEasingCurve>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QScreen>
#include <QTimer>

#include <algorithm>
#include <cmath>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace snappaste {

namespace {

constexpr int kHandleSize = 8;
constexpr int kHandleHitSlop = 5;
constexpr int kMinSelectionSize = 8;
constexpr int kNudgeStep = 1;
constexpr int kFastNudgeStep = 10;
constexpr int kDragThreshold = 4;
constexpr int kOverlayFrameIntervalMs = 6;
constexpr int kSmartCandidateIntervalMs = 120;
constexpr int kFullScreenSelectionInset = 1;
constexpr int kOverlayMaskAlpha = 96;
constexpr int kSizeLabelHeight = 20;
constexpr int kSizeLabelPaddingX = 10;
constexpr int kSizeLabelRadius = 3;
constexpr int kOverlayMargin = 8;

const QColor kSelectionColor("#2fbf9f");
const QColor kSelectionShadowColor(8, 12, 16, 170);
const QColor kHandleBorderColor(12, 18, 24, 190);
const QColor kLabelBackgroundColor(14, 20, 26, 218);
const QColor kLabelTextColor("#f4fbff");
const QColor kCandidateColor(47, 191, 159, 155);
const QColor kCandidateFillColor(47, 191, 159, 22);

QRect normalizedWithMinimum(QRect rect)
{
    rect = rect.normalized();
    if (rect.width() < kMinSelectionSize) {
        rect.setWidth(kMinSelectionSize);
    }
    if (rect.height() < kMinSelectionSize) {
        rect.setHeight(kMinSelectionSize);
    }
    return rect;
}

QRect clampedTo(QRect rect, const QRect& bounds)
{
    if (rect.width() > bounds.width()) {
        rect.setWidth(bounds.width());
    }
    if (rect.height() > bounds.height()) {
        rect.setHeight(bounds.height());
    }
    if (rect.left() < bounds.left()) {
        rect.moveLeft(bounds.left());
    }
    if (rect.top() < bounds.top()) {
        rect.moveTop(bounds.top());
    }
    if (rect.right() > bounds.right()) {
        rect.moveRight(bounds.right());
    }
    if (rect.bottom() > bounds.bottom()) {
        rect.moveBottom(bounds.bottom());
    }
    return rect;
}

QRect insetIfFullScreen(QRect rect, const QRect& desktopBounds)
{
    rect = rect.normalized();
    if (!rect.isValid()) {
        return rect;
    }

    auto inset = [](const QRect& region) {
        if (region.width() <= kFullScreenSelectionInset * 2
            || region.height() <= kFullScreenSelectionInset * 2) {
            return region;
        }
        return region.adjusted(kFullScreenSelectionInset,
                               kFullScreenSelectionInset,
                               -kFullScreenSelectionInset,
                               -kFullScreenSelectionInset);
    };

    if (rect == desktopBounds) {
        return inset(rect);
    }

    for (auto* screen : QGuiApplication::screens()) {
        if (screen != nullptr && rect == screen->geometry()) {
            return inset(rect);
        }
    }

    return rect;
}

QRect selectableRegion(QRect rect, const QRect& bounds)
{
    return insetIfFullScreen(clampedTo(rect, bounds), bounds);
}

void applyNativeDesktopBounds(QWidget& widget)
{
#ifdef Q_OS_WIN
    const auto hwnd = reinterpret_cast<HWND>(widget.winId());
    if (hwnd == nullptr) {
        return;
    }

    SetWindowPos(hwnd,
                 HWND_TOPMOST,
                 GetSystemMetrics(SM_XVIRTUALSCREEN),
                 GetSystemMetrics(SM_YVIRTUALSCREEN),
                 GetSystemMetrics(SM_CXVIRTUALSCREEN),
                 GetSystemMetrics(SM_CYVIRTUALSCREEN),
                 SWP_SHOWWINDOW);
#else
    Q_UNUSED(widget)
#endif
}

} // namespace

CaptureOverlay::CaptureOverlay(IScreenRegionDetector& regionDetector,
                               IScreenPixelSampler& pixelSampler,
                               CaptureSelectionHistory& selectionHistory,
                               QWidget* parent)
    : QWidget(parent)
    , regionDetector_(regionDetector)
    , pixelSampler_(pixelSampler)
    , selectionHistory_(selectionHistory)
    , actionBar_(new CaptureActionBar(this))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::CrossCursor);

    connect(actionBar_, &CaptureActionBar::copyRequested, this, [this] { confirmSelection(&CaptureOverlay::emitCopy); });
    connect(actionBar_, &CaptureActionBar::pinRequested, this, [this] { confirmSelection(&CaptureOverlay::emitPin); });
    connect(actionBar_, &CaptureActionBar::saveRequested, this, [this] { confirmSelection(&CaptureOverlay::emitSave); });
    connect(actionBar_, &CaptureActionBar::editRequested, this, [this] { confirmSelection(&CaptureOverlay::emitEdit); });
    connect(actionBar_, &CaptureActionBar::ocrRequested, this, [this] { confirmSelection(&CaptureOverlay::emitOcr); });
    connect(actionBar_, &CaptureActionBar::cancelRequested, this, [this] { cancel(); });
    actionBar_->hide();

    fadeAnimation_ = new QPropertyAnimation(this, "windowOpacity", this);
    fadeAnimation_->setDuration(80);
    fadeAnimation_->setEasingCurve(QEasingCurve::OutCubic);
}

void CaptureOverlay::prepareForCapture()
{
    cachedDesktopBounds_ = computeDesktopBounds();
    desktopBoundsValid_ = true;
    pixelSampler_.refresh(cachedDesktopBounds_);
}

void CaptureOverlay::showForCapture()
{
    if (!desktopBoundsValid_) {
        cachedDesktopBounds_ = computeDesktopBounds();
        desktopBoundsValid_ = true;
    }
    if (cachedDesktopBounds_.isValid()) {
        setGeometry(cachedDesktopBounds_);
    }

    show();
    applyNativeDesktopBounds(*this);
    raise();
    activateWindow();
    setFocus();
}

void CaptureOverlay::showEvent(QShowEvent* event)
{
    Q_UNUSED(event)

    const auto bounds = desktopBounds();
    if (bounds.isValid() && geometry() != bounds) {
        setGeometry(bounds);
    }
    applyNativeDesktopBounds(*this);
    selection_ = {};
    origin_ = {};
    current_ = {};
    lastMouseGlobal_ = {};
    smartCandidates_.clear();
    smartCandidateIndex_ = -1;
    pressedCandidate_ = {};
    sampledColor_ = std::nullopt;
    state_ = State::Idle;
    activeHandle_ = Handle::None;
    actionBar_->hide();
    setCursor(Qt::CrossCursor);
    setFocus();
    fadeAnimation_->stop();
    setWindowOpacity(0.0);
    fadeAnimation_->setStartValue(0.0);
    fadeAnimation_->setEndValue(1.0);
    fadeAnimation_->start();
    smartCandidateLimiter_.invalidate();
    scheduleOverlayUpdate();
}

void CaptureOverlay::keyPressEvent(QKeyEvent* event)
{
    const auto region = selectedRegion();

    if (event->key() == Qt::Key_Escape) {
        if (state_ == State::Selecting || state_ == State::Moving || state_ == State::Resizing || state_ == State::CandidatePressed) {
            state_ = State::Idle;
            clearSmartCandidates();
            selection_ = {};
            origin_ = {};
            current_ = {};
            setCursor(Qt::CrossCursor);
            scheduleOverlayUpdate();
        } else {
            cancel();
        }
        return;
    }

    if (event->key() == Qt::Key_Space && state_ == State::Selecting) {
        spaceRepositioning_ = true;
        spaceRepositionAnchor_ = lastMouseGlobal_;
        spaceRepositionStartOrigin_ = origin_;
        spaceRepositionStartCurrent_ = current_;
        return;
    }

    if (event->key() == Qt::Key_Z && event->modifiers().testFlag(Qt::ControlModifier)) {
        undoSelection();
        return;
    }

    if (event->key() == Qt::Key_C && sampledColor_.has_value()) {
        copySampledColor();
        return;
    }

    if (event->key() == Qt::Key_F1) {
        if (state_ == State::Idle && candidateRegion().isValid()) {
            selection_ = candidateRegion();
            finishReady();
            confirmSelection(&CaptureOverlay::emitCopy);
        } else if (state_ == State::Ready && selectedRegion().isValid()) {
            confirmSelection(&CaptureOverlay::emitCopy);
        }
        return;
    }

    if (event->key() == Qt::Key_Tab && state_ == State::Idle && !smartCandidates_.isEmpty()) {
        cycleCandidate(event->modifiers().testFlag(Qt::ShiftModifier) ? -1 : 1);
        return;
    }

    if (state_ == State::Idle && (event->key() == Qt::Key_Comma || event->key() == Qt::Key_BracketLeft)) {
        applyHistorySelection(false);
        return;
    }

    if (state_ == State::Idle && (event->key() == Qt::Key_Period || event->key() == Qt::Key_BracketRight)) {
        applyHistorySelection(true);
        return;
    }

    if (state_ == State::Idle && candidateRegion().isValid()) {
        switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            selection_ = candidateRegion();
            finishReady();
            confirmSelection(&CaptureOverlay::emitCopy);
            return;
        case Qt::Key_F3:
            selection_ = candidateRegion();
            finishReady();
            confirmSelection(&CaptureOverlay::emitPin);
            return;
        case Qt::Key_Space:
            selection_ = candidateRegion();
            finishReady();
            confirmSelection(&CaptureOverlay::emitEdit);
            return;
        case Qt::Key_O:
            selection_ = candidateRegion();
            finishReady();
            confirmSelection(&CaptureOverlay::emitOcr);
            return;
        case Qt::Key_S:
            if (event->modifiers().testFlag(Qt::ControlModifier)) {
                selection_ = candidateRegion();
                finishReady();
                confirmSelection(&CaptureOverlay::emitSave);
                return;
            }
            break;
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            cycleCandidate(event->key() == Qt::Key_Left || event->key() == Qt::Key_Up ? -1 : 1);
            return;
        default:
            break;
        }
    }

    if (state_ != State::Ready || !region.isValid()) {
        QWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        confirmSelection(&CaptureOverlay::emitCopy);
        return;
    case Qt::Key_F3:
        confirmSelection(&CaptureOverlay::emitPin);
        return;
    case Qt::Key_Space:
        confirmSelection(&CaptureOverlay::emitEdit);
        return;
    case Qt::Key_O:
        confirmSelection(&CaptureOverlay::emitOcr);
        return;
    case Qt::Key_Tab: {
        refreshSmartCandidates(selection_.center());
        if (!smartCandidates_.isEmpty()) {
            int dir = event->modifiers().testFlag(Qt::ShiftModifier) ? -1 : 1;
            int best = -1;
            for (int i = 0; i < smartCandidates_.size(); ++i) {
                if (smartCandidates_[i] == selection_) {
                    best = i;
                    break;
                }
            }
            if (best < 0) best = 0;
            best = ((best + dir) % smartCandidates_.size() + smartCandidates_.size()) % smartCandidates_.size();
            selection_ = smartCandidates_[best];
            origin_ = selection_.topLeft();
            current_ = selection_.bottomRight();
            clearSmartCandidates();
            scheduleOverlayUpdate();
        }
        return;
    }
    case Qt::Key_W: {
        const auto cursorPos = QCursor::pos();
        refreshSmartCandidates(cursorPos);
        if (!smartCandidates_.isEmpty()) {
            int best = 0;
            for (int i = 0; i < smartCandidates_.size(); ++i) {
                if (smartCandidates_[i].contains(cursorPos)) {
                    best = i;
                    break;
                }
            }
            selection_ = smartCandidates_[best];
            origin_ = selection_.topLeft();
            current_ = selection_.bottomRight();
            clearSmartCandidates();
            scheduleOverlayUpdate();
        }
        return;
    }
    case Qt::Key_F: {
        auto* screen = QGuiApplication::screenAt(QCursor::pos());
        if (screen) {
            selection_ = screen->geometry();
            origin_ = selection_.topLeft();
            current_ = selection_.bottomRight();
            scheduleOverlayUpdate();
        }
        return;
    }
    case Qt::Key_S:
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            confirmSelection(&CaptureOverlay::emitSave);
            return;
        }
        break;
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down: {
        const auto step = event->modifiers().testFlag(Qt::ControlModifier) ? kFastNudgeStep : kNudgeStep;
        const auto dx = event->key() == Qt::Key_Left ? -step : event->key() == Qt::Key_Right ? step : 0;
        const auto dy = event->key() == Qt::Key_Up ? -step : event->key() == Qt::Key_Down ? step : 0;
        if (event->modifiers().testFlag(Qt::ShiftModifier)) {
            resizeSelectionBy(dx, dy);
        } else {
            moveSelectionBy(dx, dy);
        }
        return;
    }
    default:
        break;
    }

    QWidget::keyPressEvent(event);
}

void CaptureOverlay::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        cancel();
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    actionBar_->hide();
    lastMouseGlobal_ = event->globalPos();
    updateSampledColor(lastMouseGlobal_);

    if (state_ == State::Ready && selectedRegion().isValid()) {
        activeHandle_ = hitTest(event->globalPos());
        dragStart_ = event->globalPos();
        dragStartSelection_ = selection_;
        if (activeHandle_ == Handle::Inside) {
            state_ = State::Moving;
            return;
        }
        if (activeHandle_ != Handle::None) {
            state_ = State::Resizing;
            return;
        }
    }

    const auto candidate = candidateRegion();
    if (state_ == State::Idle && candidate.isValid()) {
        state_ = State::CandidatePressed;
        pressGlobal_ = event->globalPos();
        pressedCandidate_ = candidate;
        origin_ = pressGlobal_;
        current_ = pressGlobal_;
        return;
    }

    state_ = State::Selecting;
    clearSmartCandidates();
    origin_ = event->globalPos();
    current_ = origin_;
    selection_ = QRect(origin_, current_).normalized();
    setCursor(Qt::CrossCursor);
    scheduleOverlayUpdate();
}

void CaptureOverlay::mouseMoveEvent(QMouseEvent* event)
{
    lastMouseGlobal_ = event->globalPos();
    updateSampledColor(lastMouseGlobal_);

    if (state_ == State::CandidatePressed) {
        const auto distance = event->globalPos() - pressGlobal_;
        if (std::abs(distance.x()) <= kDragThreshold && std::abs(distance.y()) <= kDragThreshold) {
            scheduleOverlayUpdate();
            return;
        }
        state_ = State::Selecting;
        clearSmartCandidates();
        origin_ = pressGlobal_;
        current_ = event->globalPos();
        selection_ = QRect(origin_, current_).normalized();
        scheduleOverlayUpdate();
        return;
    }

    if (spaceRepositioning_) {
        auto delta = event->globalPos() - spaceRepositionAnchor_;
        origin_ = spaceRepositionStartOrigin_ + delta;
        current_ = spaceRepositionStartCurrent_ + delta;
        selection_ = QRect(origin_, current_).normalized();
        scheduleOverlayUpdate();
        return;
    }

    if (state_ == State::Selecting) {
        current_ = event->globalPos();
        selection_ = QRect(origin_, current_).normalized();
        scheduleOverlayUpdate();
        return;
    }
    if (state_ == State::Moving) {
        applyDrag(event->globalPos());
        scheduleOverlayUpdate();
        return;
    }
    if (state_ == State::Resizing) {
        applyResize(event->globalPos());
        scheduleOverlayUpdate();
        return;
    }

    if (state_ == State::Idle) {
        refreshSmartCandidates(event->globalPos());
    }
    updateCursorFor(event->globalPos());
    scheduleOverlayUpdate();
}

void CaptureOverlay::mouseReleaseEvent(QMouseEvent* event)
{
    if (state_ == State::CandidatePressed) {
        selection_ = pressedCandidate_;
        clearSmartCandidates();
        finishReady();
        return;
    }

    if (state_ == State::Selecting) {
        const auto distance = event->globalPos() - origin_;
        if (std::abs(distance.x()) <= kDragThreshold && std::abs(distance.y()) <= kDragThreshold) {
            state_ = State::Idle;
            selection_ = {};
            origin_ = {};
            current_ = {};
            clearSmartCandidates();
            actionBar_->hide();
            scheduleOverlayUpdate();
            return;
        }
    }

    if (state_ == State::Selecting || state_ == State::Moving || state_ == State::Resizing) {
        finishReady();
    }
}

void CaptureOverlay::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && spaceRepositioning_) {
        spaceRepositioning_ = false;
        scheduleOverlayUpdate();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void CaptureOverlay::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || state_ != State::Idle) {
        return;
    }

    selection_ = desktopBounds();
    clearSmartCandidates();
    actionBar_->hide();
    selectionHistory_.add(selection_);
    state_ = State::ActionPending;
    event->accept();
    hide();
    emit copyRequested(selection_);
}

void CaptureOverlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(0, 0, 0, kOverlayMaskAlpha));

    const auto globalRegion = selectedRegion();
    if (!globalRegion.isValid()) {
        const auto candidate = candidateRegion();
        if (candidate.isValid()) {
            drawCandidate(painter, candidate);
            drawMagnifier(painter);
            painter.setPen(QColor(255, 255, 255, 80));
            painter.setFont(QFont("Microsoft YaHei UI", 11));
            painter.drawText(rect().adjusted(0, 0, 0, -8), Qt::AlignBottom | Qt::AlignHCenter,
                "Tab / Arrow keys to cycle  ·  Enter to capture");
        } else {
            if (state_ == State::Idle) {
                drawMagnifier(painter);
                painter.setPen(QColor(255, 255, 255, 80));
                painter.setFont(QFont("Microsoft YaHei UI", 11));
                painter.drawText(rect().adjusted(0, 0, 0, -24), Qt::AlignBottom | Qt::AlignHCenter,
                    "Drag to select area  ·  Double-click to capture full screen");
            }
        }
        return;
    }

    const auto localRegion = localSelectedRegion();
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillRect(localRegion, Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    painter.setPen(QPen(kSelectionShadowColor, 4));
    painter.drawRect(localRegion.adjusted(-1, -1, 0, 0));
    painter.setPen(QPen(kSelectionColor, 2));
    painter.drawRect(localRegion.adjusted(0, 0, -1, -1));

    painter.setBrush(kSelectionColor);
    painter.setPen(QPen(kHandleBorderColor, 1));
    const auto handles = {Handle::TopLeft, Handle::Top, Handle::TopRight, Handle::Right,
                          Handle::BottomRight, Handle::Bottom, Handle::BottomLeft, Handle::Left};
    for (const auto handle : handles) {
        const auto handleLocal = handleRect(handle).translated(-geometry().topLeft());
        painter.drawRoundedRect(handleLocal, 2, 2);
    }

    drawSizeLabel(painter, localRegion, globalRegion.size());
    if (state_ == State::Ready && !lastMouseGlobal_.isNull()) {
        painter.setPen(kLabelTextColor);
        painter.setFont(QFont("Microsoft YaHei UI", 11));
        painter.drawText(localRegion.bottomLeft() + QPoint(0, 20),
            QString("(%1, %2)").arg(lastMouseGlobal_.x()).arg(lastMouseGlobal_.y()));
    }
    if (state_ == State::Selecting || state_ == State::Moving || state_ == State::Resizing || state_ == State::CandidatePressed) {
        drawMagnifier(painter);
    }
}

QRect CaptureOverlay::selectedRegion() const noexcept
{
    return selection_.normalized();
}

QRect CaptureOverlay::localSelectedRegion() const
{
    return selectedRegion().translated(-geometry().topLeft());
}

QRect CaptureOverlay::localScreenGeometryFor(const QRect& globalRegion) const
{
    QScreen* screen = QGuiApplication::screenAt(globalRegion.center());
    if (screen == nullptr) {
        int largestIntersection = 0;
        for (auto* candidate : QGuiApplication::screens()) {
            if (candidate == nullptr) {
                continue;
            }
            const auto intersection = candidate->geometry().intersected(globalRegion);
            const auto area = intersection.width() * intersection.height();
            if (area > largestIntersection) {
                largestIntersection = area;
                screen = candidate;
            }
        }
    }

    const auto screenGeometry = screen != nullptr ? screen->geometry() : desktopBounds();
    return screenGeometry.translated(-geometry().topLeft());
}

QRect CaptureOverlay::availableGeometry() const
{
    return geometry();
}

QRect CaptureOverlay::computeDesktopBounds() const
{
    QRect bounds;
    for (auto* screen : QGuiApplication::screens()) {
        if (screen != nullptr) {
            bounds = bounds.united(screen->geometry());
        }
    }
    if (!bounds.isValid() && QGuiApplication::primaryScreen() != nullptr) {
        bounds = QGuiApplication::primaryScreen()->geometry();
    }
    return bounds;
}

QRect CaptureOverlay::desktopBounds() const
{
    if (desktopBoundsValid_) {
        return cachedDesktopBounds_;
    }
    return computeDesktopBounds();
}

QRect CaptureOverlay::candidateRegion() const
{
    if (smartCandidateIndex_ < 0 || smartCandidateIndex_ >= smartCandidates_.size()) {
        return {};
    }
    return selectableRegion(smartCandidates_.at(smartCandidateIndex_), availableGeometry());
}

QRect CaptureOverlay::handleRect(Handle handle) const
{
    const auto region = selectedRegion();
    const auto half = kHandleSize / 2;
    QPoint center;
    switch (handle) {
    case Handle::TopLeft:
        center = region.topLeft();
        break;
    case Handle::Top:
        center = QPoint(region.center().x(), region.top());
        break;
    case Handle::TopRight:
        center = region.topRight();
        break;
    case Handle::Right:
        center = QPoint(region.right(), region.center().y());
        break;
    case Handle::BottomRight:
        center = region.bottomRight();
        break;
    case Handle::Bottom:
        center = QPoint(region.center().x(), region.bottom());
        break;
    case Handle::BottomLeft:
        center = region.bottomLeft();
        break;
    case Handle::Left:
        center = QPoint(region.left(), region.center().y());
        break;
    case Handle::Inside:
    case Handle::None:
        return {};
    }
    return QRect(center.x() - half, center.y() - half, kHandleSize, kHandleSize);
}

CaptureOverlay::Handle CaptureOverlay::hitTest(const QPoint& globalPosition) const
{
    const auto handles = {Handle::TopLeft, Handle::Top, Handle::TopRight, Handle::Right,
                          Handle::BottomRight, Handle::Bottom, Handle::BottomLeft, Handle::Left};
    for (const auto handle : handles) {
        if (handleRect(handle).adjusted(-kHandleHitSlop, -kHandleHitSlop, kHandleHitSlop, kHandleHitSlop).contains(globalPosition)) {
            return handle;
        }
    }
    if (selectedRegion().contains(globalPosition)) {
        return Handle::Inside;
    }
    return Handle::None;
}

void CaptureOverlay::refreshSmartCandidates(const QPoint& globalPosition)
{
    if (smartCandidateLimiter_.isValid()
        && smartCandidateLimiter_.elapsed() < kSmartCandidateIntervalMs) {
        return;
    }

    smartCandidateLimiter_.restart();
    smartCandidates_ = regionDetector_.regionsAt(globalPosition, availableGeometry());
    smartCandidateIndex_ = smartCandidates_.isEmpty() ? -1 : 0;
}

void CaptureOverlay::pushSelectionUndo()
{
    if (selection_.isValid()) {
        selectionUndoStack_.push_back(selection_);
        if (selectionUndoStack_.size() > kMaxSelectionUndo) {
            selectionUndoStack_.removeFirst();
        }
    }
}

void CaptureOverlay::undoSelection()
{
    if (selectionUndoStack_.isEmpty()) return;
    selection_ = selectionUndoStack_.takeLast();
    origin_ = selection_.topLeft();
    current_ = selection_.bottomRight();
    scheduleOverlayUpdate();
}

void CaptureOverlay::clearSmartCandidates()
{
    smartCandidates_.clear();
    smartCandidateIndex_ = -1;
    smartCandidateLimiter_.invalidate();
}

void CaptureOverlay::selectCandidate(int index)
{
    if (smartCandidates_.isEmpty()) {
        smartCandidateIndex_ = -1;
        return;
    }
    const auto size = static_cast<int>(smartCandidates_.size());
    smartCandidateIndex_ = ((index % size) + size) % size;
    scheduleOverlayUpdate();
}

void CaptureOverlay::cycleCandidate(int step)
{
    if (smartCandidates_.isEmpty()) {
        return;
    }
    const auto base = smartCandidateIndex_ < 0 ? 0 : smartCandidateIndex_;
    selectCandidate(base + step);
}

void CaptureOverlay::confirmSelection(void (CaptureOverlay::*signalEmitter)(const QRect&))
{
    const auto region = selectedRegion();
    if (!region.isValid()) {
        return;
    }

    state_ = State::ActionPending;
    actionBar_->hide();
    selectionHistory_.add(region);
    hide();
    (this->*signalEmitter)(region);
}

void CaptureOverlay::emitCopy(const QRect& region)
{
    emit copyRequested(region);
}

void CaptureOverlay::emitPin(const QRect& region)
{
    emit pinRequested(region);
}

void CaptureOverlay::emitSave(const QRect& region)
{
    emit saveRequested(region);
}

void CaptureOverlay::emitEdit(const QRect& region)
{
    emit editRequested(region);
}

void CaptureOverlay::emitOcr(const QRect& region)
{
    emit ocrRequested(region);
}

void CaptureOverlay::applyHistorySelection(bool forward)
{
    const auto region = forward ? selectionHistory_.next() : selectionHistory_.previous();
    if (!region.isValid()) {
        return;
    }

    clearSmartCandidates();
    actionBar_->hide();
    selection_ = selectableRegion(region, availableGeometry());
    state_ = State::Ready;
    activeHandle_ = Handle::None;
    showActionBar();
    scheduleOverlayUpdate();
}

void CaptureOverlay::updateSampledColor(const QPoint& globalPosition)
{
    sampledColor_ = pixelSampler_.sample(globalPosition);
}

void CaptureOverlay::copySampledColor()
{
    if (!sampledColor_.has_value()) {
        return;
    }
    QApplication::clipboard()->setText(sampledColor_->name(QColor::HexRgb).toUpper());
}

void CaptureOverlay::applyDrag(const QPoint& globalPosition)
{
    const auto delta = globalPosition - dragStart_;
    selection_ = selectableRegion(dragStartSelection_.translated(delta), availableGeometry());
}

void CaptureOverlay::applyResize(const QPoint& globalPosition)
{
    auto rect = dragStartSelection_;
    switch (activeHandle_) {
    case Handle::TopLeft:
        rect.setTopLeft(globalPosition);
        break;
    case Handle::Top:
        rect.setTop(globalPosition.y());
        break;
    case Handle::TopRight:
        rect.setTopRight(globalPosition);
        break;
    case Handle::Right:
        rect.setRight(globalPosition.x());
        break;
    case Handle::BottomRight:
        rect.setBottomRight(globalPosition);
        break;
    case Handle::Bottom:
        rect.setBottom(globalPosition.y());
        break;
    case Handle::BottomLeft:
        rect.setBottomLeft(globalPosition);
        break;
    case Handle::Left:
        rect.setLeft(globalPosition.x());
        break;
    case Handle::Inside:
    case Handle::None:
        break;
    }
    selection_ = selectableRegion(normalizedWithMinimum(rect), availableGeometry());
}

void CaptureOverlay::cancel()
{
    state_ = State::Cancelled;
    clearSmartCandidates();
    actionBar_->hide();
    desktopBoundsValid_ = false;
    hide();
    emit cancelled();
}

void CaptureOverlay::finishReady()
{
    const auto rawSelection = selection_.normalized();
    if (!availableGeometry().intersects(rawSelection) || rawSelection.width() <= kMinSelectionSize || rawSelection.height() <= kMinSelectionSize) {
        cancel();
        return;
    }
    selection_ = selectableRegion(normalizedWithMinimum(rawSelection), availableGeometry());
    state_ = State::Ready;
    activeHandle_ = Handle::None;
    clearSmartCandidates();
    showActionBar();
    scheduleOverlayUpdate();
}

void CaptureOverlay::moveSelectionBy(int dx, int dy)
{
    pushSelectionUndo();
    selection_ = selectableRegion(selection_.translated(dx, dy), availableGeometry());
    showActionBar();
    scheduleOverlayUpdate();
}

void CaptureOverlay::resizeSelectionBy(int dx, int dy)
{
    pushSelectionUndo();
    auto rect = selection_;
    if (dx < 0) {
        rect.setLeft(rect.left() + dx);
    } else if (dx > 0) {
        rect.setRight(rect.right() + dx);
    }
    if (dy < 0) {
        rect.setTop(rect.top() + dy);
    } else if (dy > 0) {
        rect.setBottom(rect.bottom() + dy);
    }
    selection_ = selectableRegion(normalizedWithMinimum(rect), availableGeometry());
    showActionBar();
    scheduleOverlayUpdate();
}

void CaptureOverlay::showActionBar()
{
    actionBar_->showForRegion(localSelectedRegion(), localScreenGeometryFor(selectedRegion()));
}

void CaptureOverlay::updateCursorFor(const QPoint& globalPosition)
{
    switch (hitTest(globalPosition)) {
    case Handle::TopLeft:
    case Handle::BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case Handle::TopRight:
    case Handle::BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case Handle::Top:
    case Handle::Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case Handle::Left:
    case Handle::Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case Handle::Inside:
        setCursor(Qt::SizeAllCursor);
        break;
    case Handle::None:
        setCursor(state_ == State::Idle ? Qt::CrossCursor : Qt::ArrowCursor);
        break;
    }
}

void CaptureOverlay::scheduleOverlayUpdate()
{
    if (!frameLimiter_.isValid() || frameLimiter_.elapsed() >= kOverlayFrameIntervalMs) {
        repaintQueued_ = false;
        frameLimiter_.restart();
        update();
        return;
    }

    if (repaintQueued_) {
        return;
    }

    repaintQueued_ = true;
    const auto delay = static_cast<int>(std::max<qint64>(1, kOverlayFrameIntervalMs - frameLimiter_.elapsed()));
    QPointer<CaptureOverlay> guard(this);
    QTimer::singleShot(delay, this, [guard] {
        if (guard.isNull()) return;
        guard->repaintQueued_ = false;
        guard->frameLimiter_.restart();
        guard->update();
    });
}

void CaptureOverlay::drawCandidate(QPainter& painter, const QRect& globalRegion)
{
    const auto localRegion = globalRegion.translated(-geometry().topLeft());

    painter.setPen(QPen(kCandidateColor, 1, Qt::DashLine));
    painter.setBrush(kCandidateFillColor);
    painter.drawRect(localRegion.adjusted(0, 0, -1, -1));
    // drawSizeLabel removed per user request
}

void CaptureOverlay::drawSizeLabel(QPainter& painter, const QRect& localRegion, const QSize& regionSize)
{
    const auto text = QString("%1 x %2").arg(regionSize.width()).arg(regionSize.height());
    const auto metrics = painter.fontMetrics();
    const auto labelWidth = metrics.horizontalAdvance(text) + (kSizeLabelPaddingX * 2);

    const auto clampedX = std::max(kOverlayMargin,
                                   std::min(localRegion.left(), width() - labelWidth - kOverlayMargin));
    QRect label(clampedX,
                localRegion.top() - kSizeLabelHeight - kOverlayMargin,
                labelWidth,
                kSizeLabelHeight);
    if (!rect().contains(label)) {
        label.moveTop(localRegion.bottom() + kOverlayMargin);
    }
    if (!rect().contains(label)) {
        label.moveTop(localRegion.top() + kOverlayMargin);
    }
    if (label.bottom() > height() - kOverlayMargin) {
        label.moveBottom(height() - kOverlayMargin);
    }
    if (label.top() < kOverlayMargin) {
        label.moveTop(kOverlayMargin);
    }
    if (label.right() > width() - kOverlayMargin) {
        label.moveRight(width() - kOverlayMargin);
    }
    if (label.left() < kOverlayMargin) {
        label.moveLeft(kOverlayMargin);
    }

    if (localRegion.height() < kSizeLabelHeight + (kOverlayMargin * 2) && label.intersects(localRegion)) {
        if (localRegion.bottom() + kOverlayMargin + kSizeLabelHeight <= height() - kOverlayMargin) {
            label.moveTop(localRegion.bottom() + kOverlayMargin);
        } else if (localRegion.top() - kOverlayMargin - kSizeLabelHeight >= kOverlayMargin) {
            label.moveTop(localRegion.top() - kOverlayMargin - kSizeLabelHeight);
        }
    }

    painter.setPen(QPen(QColor(255, 255, 255, 36), 1));
    painter.setBrush(QColor(14, 20, 26, 200));
    painter.drawRoundedRect(label, kSizeLabelRadius, kSizeLabelRadius);
    painter.setPen(kLabelTextColor);
    painter.drawText(label, Qt::AlignCenter, text);
}

void CaptureOverlay::drawDimensionLines(QPainter& painter, const QRect& local, const QSize& size)
{
    if (local.width() < 60 || local.height() < 30)
        return;

    const int kOff = 18;

    painter.setFont(QFont("Segoe UI", 9));
    const auto fm = painter.fontMetrics();

    int l = local.left(), r = local.right(), t = local.top(), b = local.bottom();
    int cx = local.center().x(), cy = local.center().y();

    // Width line below
    int wY = b + kOff;
    bool wFlip = (wY + 10 > height() - 6);
    if (wFlip) wY = t - 10;

    if (wY >= 6 && wY <= height() - 6) {
        painter.setPen(QPen(kSelectionColor, 1));
        painter.drawLine(l, wY, r, wY);
        painter.drawLine(l, wY - 3, l, wY + 3);
        painter.drawLine(r, wY - 3, r, wY + 3);

        QString w = QString::number(size.width());
        int tw = fm.horizontalAdvance(w) + 8;
        int tx = qBound(l + 4, cx - tw / 2, r - tw - 4);
        painter.fillRect(tx, wY - 8, tw, 16, QColor(14, 20, 26, 200));
        painter.setPen(kLabelTextColor);
        painter.drawText(tx + 4, wY - 7, tw - 8, 14, Qt::AlignCenter, w);
    }

    // Height line on right
    int hX = r + kOff;
    bool hFlip = (hX + 10 > width() - 6);
    if (hFlip) hX = l - 10;

    if (hX >= 6 && hX <= width() - 6) {
        painter.setPen(QPen(kSelectionColor, 1));
        painter.drawLine(hX, t, hX, b);
        painter.drawLine(hX - 3, t, hX + 3, t);
        painter.drawLine(hX - 3, b, hX + 3, b);

        QString h = QString::number(size.height());
        int th = fm.height() + 8;
        int ty = qBound(t + 4, cy - th / 2, b - th - 4);
        painter.fillRect(hX - 8, ty, 16, th, QColor(14, 20, 26, 200));
        painter.setPen(kLabelTextColor);
        painter.drawText(hX - 7, ty + 4, 14, th - 8, Qt::AlignCenter, h);
    }
}

void CaptureOverlay::drawMagnifier(QPainter& painter)
{
    if (lastMouseGlobal_.isNull() || state_ == State::Ready || state_ == State::ActionPending) {
        return;
    }

    constexpr int kZoom = 8;
    constexpr int kPixels = 11;
    constexpr int kGrid = kPixels * kZoom;
    constexpr int kPad = 8;
    constexpr int kInfoH = 48;
    constexpr int kSpacing = 6;
    constexpr int kW = kPad * 2 + kGrid;
    constexpr int kH = kPad * 2 + kGrid + kSpacing + kInfoH;

    const auto local = lastMouseGlobal_ - geometry().topLeft();
    const QPoint offsets[] = {QPoint(18, 18), QPoint(-18 - kW, -18 - kH),
                              QPoint(-18 - kW, 18), QPoint(18, -18 - kH)};
    auto rect = QRect(local + offsets[0], QSize(kW, kH));
    for (int i = 0; i < 4; ++i) {
        const auto test = QRect(local + offsets[i], QSize(kW, kH));
        if (test.right() <= width() - kOverlayMargin && test.bottom() <= height() - kOverlayMargin
            && test.left() >= kOverlayMargin && test.top() >= kOverlayMargin) {
            rect = test;
            break;
        }
    }
    if (rect.left() < kOverlayMargin) rect.moveLeft(kOverlayMargin);
    if (rect.top() < kOverlayMargin) rect.moveTop(kOverlayMargin);
    if (rect.right() > width() - kOverlayMargin) rect.moveRight(width() - kOverlayMargin);
    if (rect.bottom() > height() - kOverlayMargin) rect.moveBottom(height() - kOverlayMargin);
    rect = rect.intersected(QRect(kOverlayMargin, kOverlayMargin,
                                  width() - 2 * kOverlayMargin, height() - 2 * kOverlayMargin));
    if (rect.isEmpty()) {
        return;
    }

    painter.setPen(QPen(kSelectionColor, 1));
    painter.setBrush(QColor(14, 20, 26, 224));
    painter.drawRoundedRect(rect, 6, 6);

    const int half = kPixels / 2;
    auto pixels = pixelSampler_.sampleRegion(lastMouseGlobal_, half);
    if (!pixels.isNull()) {
        const auto gridRect = QRect(rect.left() + kPad, rect.top() + kPad, kGrid, kGrid);
        painter.drawImage(gridRect, pixels.scaled(kGrid, kGrid, Qt::IgnoreAspectRatio, Qt::FastTransformation));

        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(255, 255, 255, 30), 1));
        for (int i = 1; i < kPixels; ++i) {
            int x = gridRect.left() + i * kZoom;
            painter.drawLine(x, gridRect.top(), x, gridRect.bottom());
            int y = gridRect.top() + i * kZoom;
            painter.drawLine(gridRect.left(), y, gridRect.right(), y);
        }

        const auto cx = gridRect.left() + half * kZoom + kZoom / 2;
        const auto cy = gridRect.top() + half * kZoom + kZoom / 2;
        painter.setPen(QPen(kLabelTextColor, 2));
        painter.drawLine(gridRect.left() + half * kZoom, cy,
                         gridRect.left() + half * kZoom + kZoom - 1, cy);
        painter.drawLine(cx, gridRect.top() + half * kZoom,
                         cx, gridRect.top() + half * kZoom + kZoom - 1);
    }

    painter.setPen(kLabelTextColor);
    const auto infoRect = rect.adjusted(kPad, kPad + kGrid + kSpacing, -kPad, -kPad);
    if (sampledColor_.has_value()) {
        const auto color = sampledColor_.value();
        painter.drawText(infoRect, Qt::AlignLeft | Qt::AlignTop,
            QString("%1\nrgb(%2,%3,%4)\n%5,%6")
                .arg(color.name(QColor::HexRgb).toUpper())
                .arg(color.red()).arg(color.green()).arg(color.blue())
                .arg(lastMouseGlobal_.x()).arg(lastMouseGlobal_.y()));
    } else {
        painter.drawText(infoRect, Qt::AlignLeft | Qt::AlignTop,
            QString("%1,%2").arg(lastMouseGlobal_.x()).arg(lastMouseGlobal_.y()));
    }
}

} // namespace snappaste
