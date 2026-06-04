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
class QAction;
class QMenu;

namespace snappaste {

struct Annotation;

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
    bool handleTextConfirmCancel(QKeyEvent* event, Annotation& textAnn, int editIdx);
    bool handleTextCtrlShortcuts(QKeyEvent* event, Annotation& textAnn, int editIdx);
    bool handleTextNavigation(QKeyEvent* event, Annotation& textAnn);
    bool handleTextDeletion(QKeyEvent* event, Annotation& textAnn, int editIdx);
    bool handleTextInput(QKeyEvent* event, Annotation& textAnn, int editIdx);
    void finishTextEditing();

    void handleAnnotationDeleteKey();
    void handleDuplicateKey();
    void handleLayerReorderKey(int direction);
    void handleNudgeKey(int key);
    void handleFontSizeChange(int delta);
    void handleZoomFit();

    // Key event sub-handlers
    bool handleEscapeKey(QKeyEvent* event);
    bool handleDeleteKey(QKeyEvent* event);
    bool handleCtrlShortcuts(QKeyEvent* event);
    bool handleToolShortcuts(QKeyEvent* event);
    bool handleNudgeOrFontSize(QKeyEvent* event);

    // Context menu helpers
    struct ContextMenuActions {
        QAction* copyImage = nullptr;
        QAction* saveAs = nullptr;
        QAction* deleteAnn = nullptr;
        QAction* duplicateAnn = nullptr;
        QAction* bringForward = nullptr;
        QAction* sendBackward = nullptr;
        QAction* zoomIn = nullptr;
        QAction* zoomOut = nullptr;
        QAction* zoom100 = nullptr;
        QAction* zoomFitAction = nullptr;
        QAction* clearAll = nullptr;
    };
    int resolveContextHit(QPoint widgetPos) const;
    void selectContextHit(int hitIdx);
    void clearContextSelection();
    void buildContextMenu(QMenu& menu, ContextMenuActions& actions);
    void executeContextAction(QAction* action, ContextMenuActions& actions);

    AnnotationCanvas& canvas_;
    AnnotationToolManager& toolManager_;
    AnnotationRenderer& renderer_;
};

} // namespace snappaste
