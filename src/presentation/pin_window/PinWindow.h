#pragma once

#include "domain/editor/Annotation.h"
#include "domain/ocr/OcrTypes.h"
#include "domain/pin/PinnedItem.h"
#include "platform/windows/window/WindowInteractionService.h"
#include "presentation/editor/AnnotationRenderer.h"
#include "presentation/editor/AnnotationToolManager.h"
#include "presentation/icons/IIconProvider.h"
#include "presentation/pin_window/PinToolbar.h"

class QMoveEvent;

#include <QElapsedTimer>
#include <QPropertyAnimation>
#include <QScreen>
#include <QRect>
#include <QSet>
#include <QVector>
#include <QWidget>

namespace snappaste {

class EditToolbarWidget;

class PinWindow final : public QWidget {
    Q_OBJECT

public:
    explicit PinWindow(PinnedItem item, IIconProvider& iconProvider, QWidget* parent = nullptr);

    qint64 id() const noexcept;
    const PinnedImageState& state() const noexcept;
    void setPinnedVisible(bool visible);
    void restoreInteraction();

    void setOcrResult(OcrResult result);

signals:
    void stateChanged(qint64 id, const PinnedImageState& state);
    void closeRequested(qint64 id);
    void copyRequested(const QImage& image);
    void saveRequested(const QImage& image);
    void ocrRequested(qint64 id, const QImage& image);

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
    void moveEvent(QMoveEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
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
    QSize logicalImageSize() const;
    void invalidateRenderedCache();
    void flipH();
    void flipV();
    void toggleAlwaysOnTop();
    void toggleClickThrough();
    ResizeEdge resizeEdgeAt(const QPoint& pos) const;
    QRect constrainedResizeGeometry(const QPoint& globalPos) const;
    void applyResizeToScale();

    // Mode-specific key event handlers
    void handleEditModeKey(QKeyEvent* event);
    void handleOcrModeKey(QKeyEvent* event);
    void handleNormalModeKey(QKeyEvent* event);

    // Mode-specific context menu
    void showEditContextMenu(QContextMenuEvent* event);
    void showOcrContextMenu(QContextMenuEvent* event);
    void showNormalContextMenu(QContextMenuEvent* event);

    void triggerOcr();
    void clearOcrOverlay();
    void rebuildOcrBlockRects();
    int ocrBlockAt(const QPoint& pos) const;
    void ocrCopySelected();
    void ocrCopyAll();

    void toggleEditMode();
    void applyEditAndExit();
    QPoint toEditImage(QPoint widgetPt) const;

    PinnedItem item_;
    IIconProvider& iconProvider_;
    WindowInteractionService windowInteraction_;
    QPoint dragOffset_;
    bool dragging_ = false;
    QScreen* cachedDragScreen_ = nullptr;
    bool resizing_ = false;
    ResizeEdge resizeEdge_ = EdgeNone;
    QRect resizeStartGeometry_;
    QPoint resizeStartGlobal_;
    bool hovered_ = false;
    int hoveredButton_ = -1;
    bool controlsVisible_ = false;
    bool firstShow_ = true;
    bool closeRequested_ = false;
    bool thumbnailMode_ = false;
    double fullScale_ = 1.0;
    QPoint fullPosition_;
    bool dragDropping_ = false;
    QPropertyAnimation* showAnimation_ = nullptr;
    mutable QImage renderedCache_;
    mutable int cachedRenderedVersion_ = -1;
    int renderedVersion_ = 0;
    bool savedClickThrough_ = false;
    bool visibleSaved_ = false;
    static constexpr int kMaxPinUndo = 20;
    static constexpr int kStateEmitIntervalMs = 50;
    QVector<PinnedImageState> undoStack_;
    QElapsedTimer stateEmitLimiter_;
    void emitStateChangedThrottled();

    bool ocrActive_ = false;
    QVector<OcrBlockInfo> ocrBlocks_;
    QString ocrFullText_;
    int ocrHoveredBlock_ = -1;
    QSet<int> ocrSelectedBlocks_;
    QVector<QRect> ocrBlockWidgetRects_;

    bool editing_ = false;
    AnnotationToolManager editToolManager_;
    AnnotationRenderer editRenderer_;
    PinnedImageState savedEditState_;
    EditToolbarWidget* editToolbar_ = nullptr;
    QPoint toolbarOffset_;
};

} // namespace snappaste
