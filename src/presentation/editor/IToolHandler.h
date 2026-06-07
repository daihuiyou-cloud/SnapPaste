#pragma once

#include "domain/editor/Annotation.h"

#include <QPoint>

class QKeyEvent;
class QMouseEvent;

namespace snappaste {

class AnnotationCanvas;
class AnnotationToolManager;
class AnnotationRenderer;

class IToolHandler {
public:
    virtual ~IToolHandler() = default;

    virtual bool onMousePress(const QPoint& /*imagePos*/, QMouseEvent* /*event*/) { return false; }
    virtual bool onMouseMove(const QPoint& /*imagePos*/, QMouseEvent* /*event*/) { return false; }
    virtual void onMouseRelease(const QPoint& /*imagePos*/, QMouseEvent* /*event*/) {}
    virtual void onMouseDoubleClick(QMouseEvent* /*event*/) {}
    virtual void onUpdateMoveCursor(const QPoint& /*imagePos*/) {}
};

} // namespace snappaste
