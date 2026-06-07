#pragma once

#include "presentation/editor/IToolHandler.h"

#include <QColor>
#include <QPoint>
#include <QPointF>
#include <QVariant>

#include <functional>
#include <memory>
#include <unordered_map>

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

    // Cross-cutting mouse handlers (not tool-specific)
    void handlePanningPress(QMouseEvent* event);
    bool handlePickingColorPress(QMouseEvent* event);
    void handleMovePan(QMouseEvent* event);

    // Text editing keyboard handlers (stays on event handler due to
    // tight coupling with key event dispatch chain)
    bool handleTextEditingKey(QKeyEvent* event);
    bool handleTextConfirmCancel(QKeyEvent* event, Annotation& textAnn, int editIdx);
    bool handleTextCtrlShortcuts(QKeyEvent* event, Annotation& textAnn, int editIdx);
    bool handleTextNavigation(QKeyEvent* event, Annotation& textAnn);
    bool handleTextDeletion(QKeyEvent* event, Annotation& textAnn, int editIdx);
    bool handleTextInput(QKeyEvent* event, Annotation& textAnn, int editIdx);
    void finishTextEditing();

    // Key event sub-handlers
    bool handleEscapeKey(QKeyEvent* event);
    bool handleDeleteKey(QKeyEvent* event);
    bool handleCtrlShortcuts(QKeyEvent* event);
    bool handleToolShortcuts(QKeyEvent* event);
    bool handleNudgeOrFontSize(QKeyEvent* event);

    void handleAnnotationDeleteKey();
    void handleDuplicateKey();
    void handleLayerReorderKey(int direction);
    void handleNudgeKey(int key);
    void handleFontSizeChange(int delta);
    void handleZoomFit();

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

    // Tool dispatch table
    IToolHandler* handlerFor(AnnotationTool tool) const;
    std::unordered_map<AnnotationTool, std::unique_ptr<IToolHandler>> toolHandlers_;

    AnnotationCanvas& canvas_;
    AnnotationToolManager& toolManager_;
    AnnotationRenderer& renderer_;
};

} // namespace snappaste
