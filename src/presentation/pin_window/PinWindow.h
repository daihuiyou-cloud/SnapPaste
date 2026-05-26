#pragma once

#include "domain/pin/PinnedItem.h"
#include "platform/windows/window/WindowInteractionService.h"

#include <QPropertyAnimation>
#include <QRect>
#include <QVector>
#include <QWidget>

namespace snappaste {

class PinWindow final : public QWidget {
    Q_OBJECT

public:
    explicit PinWindow(PinnedItem item, QWidget* parent = nullptr);

    qint64 id() const noexcept;
    const PinnedImageState& state() const noexcept;
    void setPinnedVisible(bool visible);
    void restoreInteraction();

signals:
    void stateChanged(qint64 id, const PinnedImageState& state);
    void closeRequested(qint64 id);
    void copyRequested(const QImage& image);
    void saveRequested(const QImage& image);

protected:
    void closeEvent(QCloseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void enterEvent(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    enum ResizeEdge {
        EdgeNone,
        EdgeLeft, EdgeRight, EdgeTop, EdgeBottom,
        EdgeTopLeft, EdgeTopRight, EdgeBottomLeft, EdgeBottomRight
    };

    void applyState();
    void applyWindowFlags();
    void emitStateChanged();
    void requestClose();
    void pushUndoState();
    void undoTransform();
    void rotateBy(int degrees);
    void setScale(double scale);
    void setOpacityValue(double opacity);
    QImage renderedImage() const;
    void invalidateRenderedCache();
    void flipH();
    void flipV();
    void toggleAlwaysOnTop();
    void toggleClickThrough();
    ResizeEdge resizeEdgeAt(const QPoint& pos) const;
    QRect constrainedResizeGeometry(const QPoint& globalPos) const;
    void applyResizeToScale();
    QRect toolbarRect() const;
    QVector<QRect> toolbarButtonRects() const;
    bool toolbarFits() const;

    PinnedItem item_;
    WindowInteractionService windowInteraction_;
    QPoint dragOffset_;
    bool dragging_ = false;
    bool resizing_ = false;
    ResizeEdge resizeEdge_ = EdgeNone;
    QRect resizeStartGeometry_;
    QPoint resizeStartGlobal_;
    bool hovered_ = false;
    bool controlsVisible_ = false;
    bool firstShow_ = true;
    bool closeRequested_ = false;
    bool thumbnailMode_ = false;
    double fullScale_ = 1.0;
    QPoint fullPosition_;
    bool dragDropping_ = false;
    QPropertyAnimation* showAnimation_ = nullptr;
    mutable QImage renderedCache_;
    static constexpr int kMaxPinUndo = 20;
    QVector<PinnedImageState> undoStack_;
};

} // namespace snappaste
