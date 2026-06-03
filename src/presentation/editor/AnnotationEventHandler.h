#pragma once

#include <QColor>
#include <QPoint>
#include <QPointF>
#include <QVariant>

#include <functional>

class QDragEnterEvent;
class QDropEvent;
class QKeyEvent;
class QMouseEvent;
class QContextMenuEvent;
class QWheelEvent;
class QInputMethodEvent;

namespace snappaste {

class AnnotationCanvas;
class AnnotationToolManager;
class AnnotationRenderer;

class AnnotationEventHandler {
public:
    AnnotationEventHandler(AnnotationCanvas& canvas,
                           AnnotationToolManager& toolManager,
                           AnnotationRenderer& renderer);

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

    void mouseDoubleClickEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    void keyPressEvent(QKeyEvent* event);

    void contextMenuEvent(QContextMenuEvent* event);

    void wheelEvent(QWheelEvent* event);

    void inputMethodEvent(QInputMethodEvent* event);
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

    // Callbacks for default event handling (wired by AnnotationCanvas)
    std::function<void(QKeyEvent*)> onKeyPressDefault;
    std::function<void(QWheelEvent*)> onWheelDefault;
    std::function<void(QInputMethodEvent*)> onInputMethodDefault;
    std::function<QVariant(Qt::InputMethodQuery)> onInputMethodQueryDefault;

private:
    QPoint toImage(QPoint widgetPt) const;
    void updateMouseInfo(QMouseEvent* event);
    void updateMoveCursor(QMouseEvent* event);

    void handlePanningPress(QMouseEvent* event);
    bool handlePickingColorPress(QMouseEvent* event);
    bool handleSelectPress(const QPoint& pos);
    bool handleEraserPress(const QPoint& pos);
    bool handleNumberedPress(const QPoint& pos);
    bool handleTextPress(const QPoint& pos);
    bool handleExistingAnnotationPress(const QPoint& pos);
    void startDrawingAnnotation(const QPoint& pos);

    void handleMovePan(QMouseEvent* event);
    void handleMoveSelect(QMouseEvent* event);
    void updateDrawingStroke(QMouseEvent* event);

    bool handleTextEditingKey(QKeyEvent* event);

    void handleAnnotationDeleteKey();
    void handleDuplicateKey();
    void handleLayerReorderKey(int direction);
    void handleNudgeKey(int key);
    void handleFontSizeChange(int delta);
    void handleZoomFit();

    AnnotationCanvas& canvas_;
    AnnotationToolManager& toolManager_;
    AnnotationRenderer& renderer_;
};

} // namespace snappaste
