#include "presentation/editor/ToolHandlers.h"

#include "presentation/editor/AnnotationCanvas.h"
#include "presentation/editor/AnnotationRenderer.h"
#include "presentation/editor/AnnotationToolManager.h"

#include <QApplication>
#include <QInputDialog>
#include <QMouseEvent>
#include <QScrollArea>
#include <QScrollBar>

#include <algorithm>
#include <cmath>

namespace snappaste {

// ============================================================================
// SelectToolHandler
// ============================================================================

SelectToolHandler::SelectToolHandler(AnnotationCanvas& canvas,
                                     AnnotationToolManager& toolManager,
                                     AnnotationRenderer& renderer)
    : canvas_(canvas)
    , toolManager_(toolManager)
    , renderer_(renderer)
{
}

bool SelectToolHandler::onMousePress(const QPoint& imagePos, QMouseEvent* event)
{
    Q_UNUSED(event);
    int sel = toolManager_.selectedIndex();
    if (sel >= 0 && sel < toolManager_.annotationCount()) {
        const auto r = toolManager_.annotationAt(sel).bounds;
        // resizing corner hit test
        const QPoint corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
        for (int ci = 0; ci < 4; ++ci) {
            if (QRect(corners[ci].x() - 8, corners[ci].y() - 8, 16, 16).contains(imagePos)) {
                toolManager_.pushUndo();
                toolManager_.startResizing(ci, r, toolManager_.annotationAt(sel).points);
                return true;
            }
        }
        // edge midpoint hit test
        const QPoint midpoints[] = {
            QPoint(r.center().x(), r.top()), QPoint(r.right(), r.center().y()),
            QPoint(r.center().x(), r.bottom()), QPoint(r.left(), r.center().y())};
        for (int mi = 0; mi < 4; ++mi) {
            if (QRect(midpoints[mi].x() - 7, midpoints[mi].y() - 7, 14, 14).contains(imagePos)) {
                toolManager_.pushUndo();
                toolManager_.startResizing(4 + mi, r, toolManager_.annotationAt(sel).points);
                return true;
            }
        }
    }
    // annotation body hit test → start moving
    for (int i = toolManager_.annotationCount() - 1; i >= 0; --i) {
        if (AnnotationRenderer::hitTestAnnotation(toolManager_.annotationAt(i), imagePos)) {
            toolManager_.pushUndo();
            toolManager_.setSelectedIndex(i);
            toolManager_.startMoving(toolManager_.annotationAt(i).bounds.topLeft() - imagePos);
            canvas_.update();
            if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
            return true;
        }
    }
    toolManager_.setSelectedIndex(-1);
    canvas_.update();
    if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
    return true;
}

bool SelectToolHandler::onMouseMove(const QPoint& imagePos, QMouseEvent* event)
{
    if (!toolManager_.resizing() && !toolManager_.moving()) {
        return false;
    }
    if (toolManager_.resizing()) {
        renderer_.invalidateCache();
        toolManager_.updateResize(imagePos, event->modifiers().testFlag(Qt::ShiftModifier));
        canvas_.update();
        return true;
    }
    if (toolManager_.moving()) {
        toolManager_.updateMove(imagePos);
        canvas_.update();
        return true;
    }
    return false;
}

void SelectToolHandler::onMouseRelease(const QPoint& imagePos, QMouseEvent* event)
{
    Q_UNUSED(imagePos);
    Q_UNUSED(event);
    if (toolManager_.resizing() || toolManager_.moving()) {
        toolManager_.setResizing(false);
        toolManager_.setMoving(false);
        canvas_.markModified();
    }
}

void SelectToolHandler::onMouseDoubleClick(QMouseEvent* event)
{
    int idx = toolManager_.selectedIndex();
    if (idx < 0 || idx >= toolManager_.annotationCount()) {
        return;
    }
    if (toolManager_.annotationAt(idx).tool == AnnotationTool::Text) {
        bool ok = false;
        auto newText = QInputDialog::getMultiLineText(
            static_cast<QWidget*>(canvas_.parent()), canvas_.tr("Edit Text"), canvas_.tr("Edit text:"),
            toolManager_.annotationAt(idx).text, &ok);
        if (ok && !newText.isEmpty() && newText != toolManager_.annotationAt(idx).text) {
            toolManager_.pushUndo();
            toolManager_.annotationsMut()[idx].text = std::move(newText);
            canvas_.markModified();
            canvas_.update();
        }
    } else {
        toolManager_.pushUndo();
        toolManager_.annotationsMut().removeAt(idx);
        toolManager_.setSelectedIndex(-1);
        canvas_.markModified();
        canvas_.update();
        if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
    }
    event->accept();
}

void SelectToolHandler::onUpdateMoveCursor(const QPoint& imagePos)
{
    int sel = toolManager_.selectedIndex();
    if (sel < 0 || sel >= toolManager_.annotationCount()) {
        canvas_.setCursor(Qt::ArrowCursor);
        return;
    }
    const auto r = toolManager_.annotationAt(sel).bounds;
    const QPoint corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
    Qt::CursorShape cursor = Qt::ArrowCursor;
    for (int ci = 0; ci < 4; ++ci) {
        if (QRect(corners[ci].x() - 8, corners[ci].y() - 8, 16, 16).contains(imagePos)) {
            cursor = (ci == 0 || ci == 3) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
            break;
        }
    }
    if (cursor == Qt::ArrowCursor) {
        const QPoint midpoints[] = {
            QPoint(r.center().x(), r.top()), QPoint(r.right(), r.center().y()),
            QPoint(r.center().x(), r.bottom()), QPoint(r.left(), r.center().y())};
        const Qt::CursorShape midCursors[] = {
            Qt::SizeVerCursor, Qt::SizeHorCursor,
            Qt::SizeVerCursor, Qt::SizeHorCursor};
        for (int mi = 0; mi < 4; ++mi) {
            if (QRect(midpoints[mi].x() - 7, midpoints[mi].y() - 7, 14, 14).contains(imagePos)) {
                cursor = midCursors[mi];
                break;
            }
        }
    }
    canvas_.setCursor(cursor);
}

// ============================================================================
// EraserToolHandler
// ============================================================================

EraserToolHandler::EraserToolHandler(AnnotationCanvas& canvas,
                                     AnnotationToolManager& toolManager,
                                     AnnotationRenderer& renderer)
    : canvas_(canvas)
    , toolManager_(toolManager)
    , renderer_(renderer)
{
}

bool EraserToolHandler::onMousePress(const QPoint& imagePos, QMouseEvent* event)
{
    Q_UNUSED(event);
    int eraserRadius = toolManager_.strokeWidth();
    QRect eraserRect(imagePos.x() - eraserRadius, imagePos.y() - eraserRadius,
                     eraserRadius * 2, eraserRadius * 2);
    for (int i = toolManager_.annotationCount() - 1; i >= 0; --i) {
        const auto& a = toolManager_.annotationAt(i);
        bool hit = false;
        if (a.tool == AnnotationTool::Pen) {
            for (const auto& pt : a.points) {
                if (eraserRect.contains(pt)) {
                    hit = true;
                    break;
                }
            }
        } else {
            hit = a.bounds.adjusted(-eraserRadius, -eraserRadius, eraserRadius, eraserRadius)
                      .contains(imagePos);
        }
        if (hit) {
            int sel = toolManager_.selectedIndex();
            toolManager_.pushUndo();
            toolManager_.annotationsMut().removeAt(i);
            if (sel == i) toolManager_.setSelectedIndex(-1);
            else if (sel > i) toolManager_.setSelectedIndex(sel - 1);
            canvas_.markModified();
            if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
            return true;
        }
    }
    return true;
}

// ============================================================================
// NumberedToolHandler
// ============================================================================

NumberedToolHandler::NumberedToolHandler(AnnotationCanvas& canvas,
                                         AnnotationToolManager& toolManager,
                                         AnnotationRenderer& renderer)
    : canvas_(canvas)
    , toolManager_(toolManager)
    , renderer_(renderer)
{
}

bool NumberedToolHandler::onMousePress(const QPoint& imagePos, QMouseEvent* event)
{
    Q_UNUSED(event);
    toolManager_.pushUndo();
    Annotation ann;
    ann.tool = AnnotationTool::Numbered;
    ann.bounds = QRect(imagePos.x() - kDefaultNumberedSize / 2,
                       imagePos.y() - kDefaultNumberedSize / 2,
                       kDefaultNumberedSize, kDefaultNumberedSize);
    ann.color = toolManager_.color();
    ann.color.setAlpha(toolManager_.strokeAlpha());
    ann.number = toolManager_.nextNumber();
    toolManager_.advanceNumber();
    toolManager_.annotationsMut().push_back(std::move(ann));
    toolManager_.setSelectedIndex(toolManager_.annotationCount() - 1);
    canvas_.markModified();
    if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
    return true;
}

// ============================================================================
// TextToolHandler
// ============================================================================

TextToolHandler::TextToolHandler(AnnotationCanvas& canvas,
                                 AnnotationToolManager& toolManager,
                                 AnnotationRenderer& renderer)
    : canvas_(canvas)
    , toolManager_(toolManager)
    , renderer_(renderer)
{
}

bool TextToolHandler::onMousePress(const QPoint& imagePos, QMouseEvent* event)
{
    Q_UNUSED(event);
    if (toolManager_.editingTextIndex() >= 0) {
        toolManager_.setEditingTextIndex(-1);
        toolManager_.setCursorPos(0);
        toolManager_.setPreeditString(QString());
        canvas_.update();
    }
    for (int i = toolManager_.annotationCount() - 1; i >= 0; --i) {
        if (toolManager_.annotationAt(i).tool == AnnotationTool::Text
            && AnnotationRenderer::hitTestAnnotation(toolManager_.annotationAt(i), imagePos)) {
            toolManager_.pushUndo();
            toolManager_.setSelectedIndex(i);
            toolManager_.setEditingTextIndex(i);
            canvas_.update();
            if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
            return true;
        }
    }
    QFont font(toolManager_.fontFamily().isEmpty() ? QApplication::font().family() : toolManager_.fontFamily());
    font.setPixelSize(toolManager_.fontSize());
    font.setBold(toolManager_.bold());
    font.setItalic(toolManager_.italic());
    font.setUnderline(toolManager_.underline());
    QFontMetrics fm(font);
    int defaultWidth = qMax(fm.horizontalAdvance(QStringLiteral("    ")) + 8, 60);
    QRect bounds(imagePos.x(), imagePos.y(), defaultWidth, fm.height() + 8);
    const auto& img = toolManager_.image();
    auto logicalW = img.width() / img.devicePixelRatio();
    if (bounds.right() > logicalW) {
        bounds.setRight(logicalW - 4);
        bounds.setWidth(qMin(bounds.width(), static_cast<int>(logicalW - bounds.left() - 4)));
    }
    toolManager_.pushUndo();
    Annotation ann;
    ann.tool = AnnotationTool::Text;
    ann.bounds = bounds;
    ann.text = QString();
    ann.color = toolManager_.color();
    ann.color.setAlpha(toolManager_.strokeAlpha());
    ann.strokeWidth = 2;
    ann.textFontSize = toolManager_.fontSize();
    ann.textOutline = toolManager_.textOutlineEnabled();
    ann.textBackground = toolManager_.textBackgroundEnabled();
    ann.textBackgroundColor = toolManager_.textBackgroundColor();
    ann.fontFamily = toolManager_.fontFamily();
    ann.bold = toolManager_.bold();
    ann.italic = toolManager_.italic();
    ann.underline = toolManager_.underline();
    ann.textAlignment = toolManager_.textAlignment();
    toolManager_.annotationsMut().push_back(std::move(ann));
    toolManager_.setSelectedIndex(toolManager_.annotationCount() - 1);
    toolManager_.setEditingTextIndex(toolManager_.selectedIndex());
    canvas_.markModified();
    if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
    canvas_.setFocus();
    return true;
}

void TextToolHandler::onMouseDoubleClick(QMouseEvent* event)
{
    Q_UNUSED(event);
    for (int i = toolManager_.annotationCount() - 1; i >= 0; --i) {
        if (toolManager_.annotationAt(i).tool == AnnotationTool::Text) {
            QPoint localPos = canvas_.mapFromGlobal(event->globalPos());
            // double-click position needs to be relative to canvas
            auto* scrollArea = qobject_cast<QScrollArea*>(canvas_.parentWidget());
            QPoint canvasPos = localPos;
            if (scrollArea) {
                canvasPos += QPoint(scrollArea->horizontalScrollBar()->value(),
                                    scrollArea->verticalScrollBar()->value());
            }
            QPoint imagePos(static_cast<int>(canvasPos.x() / toolManager_.zoomFactor()),
                            static_cast<int>(canvasPos.y() / toolManager_.zoomFactor()));
            if (toolManager_.annotationAt(i).bounds.contains(imagePos)) {
                bool ok = false;
                auto newText = QInputDialog::getMultiLineText(
                    static_cast<QWidget*>(canvas_.parent()), canvas_.tr("Edit Text"), canvas_.tr("Edit text:"),
                    toolManager_.annotationAt(i).text, &ok);
                if (ok && !newText.isEmpty() && newText != toolManager_.annotationAt(i).text) {
                    toolManager_.pushUndo();
                    toolManager_.annotationsMut()[i].text = std::move(newText);
                    canvas_.markModified();
                    canvas_.update();
                }
                return;
            }
        }
    }
}

void TextToolHandler::onUpdateMoveCursor(const QPoint& imagePos)
{
    Q_UNUSED(imagePos);
    canvas_.setCursor(Qt::IBeamCursor);
}

// ============================================================================
// DrawingToolHandler
// ============================================================================

DrawingToolHandler::DrawingToolHandler(AnnotationCanvas& canvas,
                                       AnnotationToolManager& toolManager,
                                       AnnotationRenderer& renderer)
    : canvas_(canvas)
    , toolManager_(toolManager)
    , renderer_(renderer)
{
}

bool DrawingToolHandler::onMousePress(const QPoint& imagePos, QMouseEvent* event)
{
    Q_UNUSED(event);
    // Try to select existing annotation first
    for (int i = toolManager_.annotationCount() - 1; i >= 0; --i) {
        if (AnnotationRenderer::hitTestAnnotation(toolManager_.annotationAt(i), imagePos)) {
            if (i != toolManager_.selectedIndex()) {
                toolManager_.setSelectedIndex(i);
                canvas_.update();
                if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
            }
            return true;
        }
    }
    // Start drawing new annotation
    toolManager_.startDrawing(imagePos);
    return true;
}

bool DrawingToolHandler::onMouseMove(const QPoint& imagePos, QMouseEvent* event)
{
    if (!toolManager_.drawing()) {
        return false;
    }
    auto rawPos = imagePos;
    auto start = toolManager_.start();
    auto draftTool = toolManager_.draft().tool;

    if (draftTool == AnnotationTool::Numbered) {
        return true;
    }

    if (!(rawPos == start) && toolManager_.cropAspectRatio() > 0.0 && draftTool == AnnotationTool::Crop) {
        auto dx = rawPos.x() - start.x();
        auto dy = rawPos.y() - start.y();
        double adx = std::abs(dx);
        double ady = std::abs(dy);
        double nh = adx / toolManager_.cropAspectRatio();
        if (nh > ady) {
            rawPos.setY(start.y() + (dy >= 0 ? static_cast<int>(nh) : -static_cast<int>(nh)));
        } else {
            double nw = ady * toolManager_.cropAspectRatio();
            rawPos.setX(start.x() + (dx >= 0 ? static_cast<int>(nw) : -static_cast<int>(nw)));
        }
    } else if (!(rawPos == start) && event->modifiers().testFlag(Qt::ShiftModifier)) {
        if (draftTool == AnnotationTool::Rectangle || draftTool == AnnotationTool::Ellipse) {
            auto dx = rawPos.x() - start.x();
            auto dy = rawPos.y() - start.y();
            int side = std::max(std::abs(dx), std::abs(dy));
            rawPos.setX(start.x() + (dx >= 0 ? side : -side));
            rawPos.setY(start.y() + (dy >= 0 ? side : -side));
        } else if (draftTool == AnnotationTool::Arrow || draftTool == AnnotationTool::Line) {
            double angle = std::atan2(rawPos.y() - start.y(), rawPos.x() - start.x());
            double snapped = std::round(angle / (M_PI / 4)) * (M_PI / 4);
            double dx = rawPos.x() - start.x(), dy = rawPos.y() - start.y();
            double dist = std::sqrt(dx * dx + dy * dy);
            rawPos.setX(start.x() + static_cast<int>(dist * std::cos(snapped)));
            rawPos.setY(start.y() + static_cast<int>(dist * std::sin(snapped)));
        } else if (draftTool == AnnotationTool::Pen) {
            auto dx = std::abs(rawPos.x() - start.x());
            auto dy = std::abs(rawPos.y() - start.y());
            if (dx >= dy) {
                rawPos.setY(start.y());
            } else {
                rawPos.setX(start.x());
            }
        }
    }

    toolManager_.setCurrent(rawPos);
    toolManager_.updateDrawingStroke(rawPos);

    auto oldCurrent = toolManager_.current();
    auto draft = toolManager_.draft();
    if (draftTool == AnnotationTool::Pen || draftTool == AnnotationTool::Mosaic) {
        int margin = (draftTool == AnnotationTool::Mosaic)
            ? toolManager_.strokeWidth() * 4 + 4
            : toolManager_.strokeWidth() + 4;
        QRect oldRect(oldCurrent, rawPos);
        canvas_.update(oldRect.normalized().adjusted(-margin, -margin, margin, margin));
    } else {
        canvas_.update();
    }
    return true;
}

void DrawingToolHandler::onMouseRelease(const QPoint& imagePos, QMouseEvent* event)
{
    Q_UNUSED(event);
    if (!toolManager_.drawing() || toolManager_.draft().tool == AnnotationTool::Numbered) {
        return;
    }

    auto finishPos = imagePos;
    toolManager_.setCurrent(finishPos);
    toolManager_.draftMut().bounds = QRect(toolManager_.start(), finishPos).normalized();
    toolManager_.finishDrawing();

    if (toolManager_.draft().tool == AnnotationTool::Crop) {
        if (toolManager_.draft().bounds.width() > 5 && toolManager_.draft().bounds.height() > 5) {
            canvas_.applyCrop(toolManager_.draft().bounds);
        }
        toolManager_.setTool(AnnotationTool::Select);
        canvas_.update();
        return;
    }

    auto& draft = toolManager_.draftMut();
    if (draft.tool == AnnotationTool::Arrow) {
        draft.points = {toolManager_.start(), finishPos};
    }
    if (draft.bounds.width() > 2 || draft.bounds.height() > 2
        || (draft.tool == AnnotationTool::Pen && draft.points.size() >= 2)
        || (draft.tool == AnnotationTool::Mosaic && draft.points.size() >= 2)) {
        toolManager_.pushUndo();
        toolManager_.annotationsMut().push_back(std::move(draft));
        toolManager_.setSelectedIndex(toolManager_.annotationCount() - 1);
        canvas_.markModified();
        if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
        auto tool = toolManager_.currentTool();
        if (tool != AnnotationTool::Select && tool != AnnotationTool::Text
            && tool != AnnotationTool::Numbered && tool != AnnotationTool::Mosaic
            && tool != AnnotationTool::Eraser) {
            toolManager_.setTool(AnnotationTool::Select);
        }
    } else {
        canvas_.update();
    }
}

} // namespace snappaste
