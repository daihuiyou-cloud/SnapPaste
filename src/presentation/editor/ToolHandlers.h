#pragma once

#include "presentation/editor/IToolHandler.h"

#include <QPoint>
#include <memory>
#include <unordered_map>

class QMouseEvent;

namespace snappaste {

class AnnotationCanvas;
class AnnotationToolManager;
class AnnotationRenderer;

class SelectToolHandler final : public IToolHandler {
public:
    SelectToolHandler(AnnotationCanvas& canvas, AnnotationToolManager& toolManager,
                      AnnotationRenderer& renderer);
    bool onMousePress(const QPoint& imagePos, QMouseEvent* event) override;
    bool onMouseMove(const QPoint& imagePos, QMouseEvent* event) override;
    void onMouseRelease(const QPoint& imagePos, QMouseEvent* event) override;
    void onMouseDoubleClick(QMouseEvent* event) override;
    void onUpdateMoveCursor(const QPoint& imagePos) override;

private:
    AnnotationCanvas& canvas_;
    AnnotationToolManager& toolManager_;
    AnnotationRenderer& renderer_;
};

class EraserToolHandler final : public IToolHandler {
public:
    EraserToolHandler(AnnotationCanvas& canvas, AnnotationToolManager& toolManager,
                      AnnotationRenderer& renderer);
    bool onMousePress(const QPoint& imagePos, QMouseEvent* event) override;

private:
    AnnotationCanvas& canvas_;
    AnnotationToolManager& toolManager_;
    AnnotationRenderer& renderer_;
};

class NumberedToolHandler final : public IToolHandler {
public:
    NumberedToolHandler(AnnotationCanvas& canvas, AnnotationToolManager& toolManager,
                        AnnotationRenderer& renderer);
    bool onMousePress(const QPoint& imagePos, QMouseEvent* event) override;

private:
    AnnotationCanvas& canvas_;
    AnnotationToolManager& toolManager_;
    AnnotationRenderer& renderer_;
};

class TextToolHandler final : public IToolHandler {
public:
    TextToolHandler(AnnotationCanvas& canvas, AnnotationToolManager& toolManager,
                    AnnotationRenderer& renderer);
    bool onMousePress(const QPoint& imagePos, QMouseEvent* event) override;
    void onMouseDoubleClick(QMouseEvent* event) override;
    void onUpdateMoveCursor(const QPoint& imagePos) override;

private:
    AnnotationCanvas& canvas_;
    AnnotationToolManager& toolManager_;
    AnnotationRenderer& renderer_;
};

class DrawingToolHandler final : public IToolHandler {
public:
    DrawingToolHandler(AnnotationCanvas& canvas, AnnotationToolManager& toolManager,
                       AnnotationRenderer& renderer);
    bool onMousePress(const QPoint& imagePos, QMouseEvent* event) override;
    bool onMouseMove(const QPoint& imagePos, QMouseEvent* event) override;
    void onMouseRelease(const QPoint& imagePos, QMouseEvent* event) override;

private:
    AnnotationCanvas& canvas_;
    AnnotationToolManager& toolManager_;
    AnnotationRenderer& renderer_;
};

} // namespace snappaste
