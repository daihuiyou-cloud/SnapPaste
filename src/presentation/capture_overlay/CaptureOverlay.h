#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QPoint>
#include <QPropertyAnimation>
#include <QRect>
#include <QVector>
#include <QWidget>

#include <optional>

namespace snappaste {

class CaptureSelectionHistory;
class CaptureActionBar;
class IScreenPixelSampler;
class IScreenRegionDetector;

class CaptureOverlay final : public QWidget {
    Q_OBJECT

public:
    CaptureOverlay(IScreenRegionDetector& regionDetector,
                   IScreenPixelSampler& pixelSampler,
                   CaptureSelectionHistory& selectionHistory,
                   QWidget* parent = nullptr);

    void prepareForCapture();
    void showForCapture();

signals:
    void regionSelected(const QRect& region);
    void copyRequested(const QRect& region);
    void pinRequested(const QRect& region);
    void saveRequested(const QRect& region);
    void editRequested(const QRect& region);
    void ocrRequested(const QRect& region);
    void cancelled();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    enum class State {
        Idle,
        CandidatePressed,
        Selecting,
        Moving,
        Resizing,
        Ready,
        ActionPending,
        Cancelled
    };

    enum class Handle {
        None,
        Inside,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left
    };

    QRect selectedRegion() const noexcept;
    QRect localSelectedRegion() const;
    QRect localScreenGeometryFor(const QRect& globalRegion) const;
    QRect availableGeometry() const;
    QRect desktopBounds() const;
    QRect candidateRegion() const;
    QRect handleRect(Handle handle) const;
    Handle hitTest(const QPoint& globalPosition) const;
    void refreshSmartCandidates(const QPoint& globalPosition);
    void clearSmartCandidates();
    void selectCandidate(int index);
    void cycleCandidate(int step);
    void confirmSelection(void (CaptureOverlay::*signalEmitter)(const QRect&));
    void emitCopy(const QRect& region);
    void emitPin(const QRect& region);
    void emitSave(const QRect& region);
    void emitEdit(const QRect& region);
    void emitOcr(const QRect& region);
    void applyHistorySelection(bool forward);
    void updateSampledColor(const QPoint& globalPosition);
    void copySampledColor();
    void applyDrag(const QPoint& globalPosition);
    void applyResize(const QPoint& globalPosition);
    void cancel();
    void finishReady();
    void moveSelectionBy(int dx, int dy);
    void resizeSelectionBy(int dx, int dy);
    void showActionBar();
    void updateCursorFor(const QPoint& globalPosition);
    void scheduleOverlayUpdate();
    void drawCandidate(QPainter& painter, const QRect& globalRegion);
    void drawSizeLabel(QPainter& painter, const QRect& localRegion, const QSize& regionSize);
    void drawMagnifier(QPainter& painter);

    IScreenRegionDetector& regionDetector_;
    IScreenPixelSampler& pixelSampler_;
    CaptureSelectionHistory& selectionHistory_;
    QPoint origin_;
    QPoint current_;
    QRect selection_;
    QVector<QRect> smartCandidates_;
    int smartCandidateIndex_ = -1;
    QRect dragStartSelection_;
    QPoint dragStart_;
    QPoint pressGlobal_;
    QRect pressedCandidate_;
    QPoint lastMouseGlobal_;
    std::optional<QColor> sampledColor_;
    State state_ = State::Idle;
    Handle activeHandle_ = Handle::None;
    CaptureActionBar* actionBar_ = nullptr;
    QPropertyAnimation* fadeAnimation_ = nullptr;
    QElapsedTimer frameLimiter_;
    QElapsedTimer smartCandidateLimiter_;
    bool repaintQueued_ = false;
};

} // namespace snappaste
