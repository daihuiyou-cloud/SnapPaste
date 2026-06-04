#include "presentation/editor/AnnotationEventHandler.h"
#include "presentation/editor/AnnotationCanvas.h"
#include "presentation/editor/AnnotationToolManager.h"
#include "presentation/editor/AnnotationRenderer.h"

#include <QApplication>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace snappaste {

AnnotationEventHandler::AnnotationEventHandler(
    AnnotationCanvas& canvas,
    AnnotationToolManager& toolManager,
    AnnotationRenderer& renderer)
    : canvas_(canvas)
    , toolManager_(toolManager)
    , renderer_(renderer)
{
}

QPoint AnnotationEventHandler::toImage(QPoint widgetPt) const
{
    return QPoint(static_cast<int>(widgetPt.x() / toolManager_.zoomFactor()),
                  static_cast<int>(widgetPt.y() / toolManager_.zoomFactor()));
}

// --- Drag & Drop ---

void AnnotationEventHandler::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void AnnotationEventHandler::dropEvent(QDropEvent* event)
{
    const auto urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        const auto path = urls.first().toLocalFile();
        QImage img(path);
        if (!img.isNull()) {
            if (canvas_.isModified()) {
                auto ret = QMessageBox::question(&canvas_, canvas_.tr("Unsaved Changes"),
                    canvas_.tr("Drop image and discard all annotations?"),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (ret != QMessageBox::Yes) {
                    return;
                }
            }
            canvas_.setImage(img);
            event->acceptProposedAction();
        }
    }
}

// --- Mouse events ---

void AnnotationEventHandler::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (toolManager_.image().isNull()) {
        return;
    }
    if (toolManager_.currentTool() == AnnotationTool::Select && toolManager_.selectedIndex() >= 0) {
        int idx = toolManager_.selectedIndex();
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
        return;
    }
    if (toolManager_.currentTool() != AnnotationTool::Text) {
        return;
    }

    const auto pos = toImage(event->pos());

    for (int i = toolManager_.annotationCount() - 1; i >= 0; --i) {
        if (toolManager_.annotationAt(i).tool == AnnotationTool::Text
            && toolManager_.annotationAt(i).bounds.contains(pos)) {
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

void AnnotationEventHandler::handlePanningPress(QMouseEvent* event)
{
    toolManager_.setDrawing(false);
    toolManager_.setPanning(true);
    toolManager_.setPanStart(event->pos());
    canvas_.setCursor(Qt::ClosedHandCursor);
    event->accept();
}

bool AnnotationEventHandler::handlePickingColorPress(QMouseEvent* event)
{
    if (!toolManager_.pickingColor()) return false;
    toolManager_.setPickingColor(false);
    canvas_.setCursor(Qt::ArrowCursor);
    const auto pos = toImage(event->pos());
    const auto& img = toolManager_.image();
    QRect logicalImageRect(QPoint(0, 0), img.size() / img.devicePixelRatio());
    if (logicalImageRect.contains(pos)) {
        const auto dpr = img.devicePixelRatio();
        QPoint physicalPos(static_cast<int>(pos.x() * dpr),
                           static_cast<int>(pos.y() * dpr));
        QRgb pixel = reinterpret_cast<const QRgb*>(img.constScanLine(physicalPos.y()))[physicalPos.x()];
        toolManager_.setColor(QColor::fromRgba(pixel));
    }
    if (toolManager_.onPickingColorChanged) toolManager_.onPickingColorChanged(false);
    canvas_.update();
    return true;
}

bool AnnotationEventHandler::handleSelectPress(const QPoint& pos)
{
    int sel = toolManager_.selectedIndex();
    if (sel >= 0 && sel < toolManager_.annotationCount()) {
        const auto r = toolManager_.annotationAt(sel).bounds;
        const QPoint corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
        for (int ci = 0; ci < 4; ++ci) {
            if (QRect(corners[ci].x() - 8, corners[ci].y() - 8, 16, 16).contains(pos)) {
                toolManager_.pushUndo();
                toolManager_.startResizing(ci, r, toolManager_.annotationAt(sel).points);
                return true;
            }
        }
        const QPoint midpoints[] = {
            QPoint(r.center().x(), r.top()), QPoint(r.right(), r.center().y()),
            QPoint(r.center().x(), r.bottom()), QPoint(r.left(), r.center().y())};
        for (int mi = 0; mi < 4; ++mi) {
            if (QRect(midpoints[mi].x() - 7, midpoints[mi].y() - 7, 14, 14).contains(pos)) {
                toolManager_.pushUndo();
                toolManager_.startResizing(4 + mi, r, toolManager_.annotationAt(sel).points);
                return true;
            }
        }
    }
    for (int i = toolManager_.annotationCount() - 1; i >= 0; --i) {
        if (AnnotationRenderer::hitTestAnnotation(toolManager_.annotationAt(i), pos)) {
            toolManager_.pushUndo();
            toolManager_.setSelectedIndex(i);
            toolManager_.startMoving(toolManager_.annotationAt(i).bounds.topLeft() - pos);
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

bool AnnotationEventHandler::handleEraserPress(const QPoint& pos)
{
    int eraserRadius = toolManager_.strokeWidth();
    QRect eraserRect(pos.x() - eraserRadius, pos.y() - eraserRadius,
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
                     .contains(pos);
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

bool AnnotationEventHandler::handleNumberedPress(const QPoint& pos)
{
    toolManager_.pushUndo();
    Annotation ann;
    ann.tool = AnnotationTool::Numbered;
    ann.bounds = QRect(pos.x() - kDefaultNumberedSize / 2, pos.y() - kDefaultNumberedSize / 2,
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

bool AnnotationEventHandler::handleTextPress(const QPoint& pos)
{
    if (toolManager_.editingTextIndex() >= 0) {
        toolManager_.setEditingTextIndex(-1);
        toolManager_.setCursorPos(0);
        toolManager_.setPreeditString(QString());
        canvas_.update();
    }
    for (int i = toolManager_.annotationCount() - 1; i >= 0; --i) {
        if (toolManager_.annotationAt(i).tool == AnnotationTool::Text
            && AnnotationRenderer::hitTestAnnotation(toolManager_.annotationAt(i), pos)) {
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
    QRect bounds(pos.x(), pos.y(), defaultWidth, fm.height() + 8);
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

bool AnnotationEventHandler::handleExistingAnnotationPress(const QPoint& pos)
{
    auto tool = toolManager_.currentTool();
    if (tool == AnnotationTool::Select || tool == AnnotationTool::Eraser
        || tool == AnnotationTool::Numbered) {
        return false;
    }
    for (int i = toolManager_.annotationCount() - 1; i >= 0; --i) {
        if (AnnotationRenderer::hitTestAnnotation(toolManager_.annotationAt(i), pos)) {
            if (i != toolManager_.selectedIndex()) {
                toolManager_.setSelectedIndex(i);
                canvas_.update();
                if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
            }
            return true;
        }
    }
    return false;
}

void AnnotationEventHandler::startDrawingAnnotation(const QPoint& pos)
{
    toolManager_.startDrawing(pos);
}

void AnnotationEventHandler::mousePressEvent(QMouseEvent* event)
{
    if (toolManager_.image().isNull()) return;

    canvas_.setFocus();

    int editIdx = toolManager_.editingTextIndex();
    if (editIdx >= 0 && event->button() == Qt::LeftButton) {
        const auto pos = toImage(event->pos());
        bool clickedSelf = (editIdx < toolManager_.annotationCount()
            && toolManager_.annotationAt(editIdx).tool == AnnotationTool::Text
            && AnnotationRenderer::hitTestAnnotation(toolManager_.annotationAt(editIdx), pos));
        if (!clickedSelf) {
            if (editIdx < toolManager_.annotationCount()
                && toolManager_.annotationAt(editIdx).tool == AnnotationTool::Text
                && toolManager_.annotationAt(editIdx).text.isEmpty()
                && toolManager_.annotationAt(editIdx).points.isEmpty()) {
                toolManager_.pushUndo();
                toolManager_.annotationsMut().removeAt(editIdx);
                int sel = toolManager_.selectedIndex();
                if (sel >= editIdx) toolManager_.setSelectedIndex(sel - 1);
                canvas_.markModified();
                if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
            }
            toolManager_.setEditingTextIndex(-1);
            toolManager_.setCursorPos(0);
            toolManager_.setPreeditString(QString());
            canvas_.update();
        }
    }

    if (event->button() == Qt::MiddleButton) {
        handlePanningPress(event);
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (handlePickingColorPress(event)) return;

    const auto pos = toImage(event->pos());

    if (toolManager_.currentTool() == AnnotationTool::Select) { handleSelectPress(pos); return; }
    if (toolManager_.currentTool() == AnnotationTool::Eraser) { handleEraserPress(pos); return; }
    if (toolManager_.currentTool() == AnnotationTool::Numbered) { handleNumberedPress(pos); return; }
    if (toolManager_.currentTool() == AnnotationTool::Text) { handleTextPress(pos); return; }

    if (!handleExistingAnnotationPress(pos)) {
        startDrawingAnnotation(pos);
    }
}

void AnnotationEventHandler::updateMoveCursor(QMouseEvent* event)
{
    auto tool = toolManager_.currentTool();
    int sel = toolManager_.selectedIndex();
    if (tool == AnnotationTool::Select && sel >= 0 && sel < toolManager_.annotationCount()) {
        const auto pos = toImage(event->pos());
        const auto r = toolManager_.annotationAt(sel).bounds;
        const QPoint corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
        Qt::CursorShape cursor = Qt::ArrowCursor;
        for (int ci = 0; ci < 4; ++ci) {
            if (QRect(corners[ci].x() - 8, corners[ci].y() - 8, 16, 16).contains(pos)) {
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
                if (QRect(midpoints[mi].x() - 7, midpoints[mi].y() - 7, 14, 14).contains(pos)) {
                    cursor = midCursors[mi];
                    break;
                }
            }
        }
        canvas_.setCursor(cursor);
    } else if (tool == AnnotationTool::Text)
        canvas_.setCursor(Qt::IBeamCursor);
    else if (tool == AnnotationTool::Eraser || tool == AnnotationTool::Mosaic)
        canvas_.setCursor(Qt::PointingHandCursor);
    else
        canvas_.setCursor(Qt::ArrowCursor);
}

void AnnotationEventHandler::handleMovePan(QMouseEvent* event)
{
    auto delta = event->pos() - toolManager_.panStart();
    auto* scrollArea = qobject_cast<QScrollArea*>(canvas_.parentWidget());
    if (scrollArea) {
        scrollArea->horizontalScrollBar()->setValue(
            scrollArea->horizontalScrollBar()->value() - delta.x());
        scrollArea->verticalScrollBar()->setValue(
            scrollArea->verticalScrollBar()->value() - delta.y());
    }
    toolManager_.setPanStart(event->pos());
}

void AnnotationEventHandler::handleMoveSelect(QMouseEvent* event)
{
    if (!toolManager_.resizing() && !toolManager_.moving()) {
        return;
    }
    renderer_.invalidateCache();
    const auto p = toImage(event->pos());
    if (toolManager_.resizing()) {
        toolManager_.updateResize(p, event->modifiers().testFlag(Qt::ShiftModifier));
        canvas_.update();
        return;
    }
    if (toolManager_.moving()) {
        toolManager_.updateMove(p);
        canvas_.update();
    }
}

void AnnotationEventHandler::updateDrawingStroke(QMouseEvent* event)
{
    auto rawPos = toImage(event->pos());
    auto start = toolManager_.start();
    auto draftTool = toolManager_.draft().tool;

    if (draftTool == AnnotationTool::Numbered) {
        return;
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
            double dist = std::sqrt(std::pow(rawPos.x() - start.x(), 2) +
                                    std::pow(rawPos.y() - start.y(), 2));
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

    // Compute dirty region for incremental update
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
}

void AnnotationEventHandler::mouseMoveEvent(QMouseEvent* event)
{
    if (toolManager_.panning()) {
        handleMovePan(event);
        event->accept();
        return;
    }
    if (!toolManager_.drawing()) {
        updateMoveCursor(event);
    }
    auto selTool = toolManager_.currentTool();
    int sel = toolManager_.selectedIndex();
    if (selTool == AnnotationTool::Select && sel >= 0 && sel < toolManager_.annotationCount()) {
        handleMoveSelect(event);
        if (toolManager_.resizing() || toolManager_.moving()) {
            event->accept();
            return;
        }
    }
    if (!toolManager_.drawing()) {
        return;
    }
    updateDrawingStroke(event);
}

void AnnotationEventHandler::mouseReleaseEvent(QMouseEvent* event)
{
    if (toolManager_.panning()) {
        toolManager_.setPanning(false);
        auto tool = toolManager_.currentTool();
        if (tool == AnnotationTool::Pen || tool == AnnotationTool::Eraser || tool == AnnotationTool::Mosaic) {
            canvas_.updateBrushCursor();
        } else {
            switch (tool) {
            case AnnotationTool::Text: canvas_.setCursor(Qt::IBeamCursor); break;
            default: canvas_.setCursor(Qt::CrossCursor); break;
            }
        }
        event->accept();
        return;
    }
    if (toolManager_.currentTool() == AnnotationTool::Select) {
        if (toolManager_.resizing() || toolManager_.moving()) {
            toolManager_.setResizing(false);
            toolManager_.setMoving(false);
            canvas_.markModified();
            return;
        }
    }

    if (!toolManager_.drawing() || toolManager_.draft().tool == AnnotationTool::Numbered) {
        return;
    }

    auto finishPos = toImage(event->pos());
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

// --- Keyboard events ---

bool AnnotationEventHandler::handleTextEditingKey(QKeyEvent* event)
{
    int editIdx = toolManager_.editingTextIndex();
    auto& textAnn = toolManager_.annotationsMut()[editIdx];

    if (handleTextConfirmCancel(event, textAnn, editIdx)) return true;
    if (handleTextCtrlShortcuts(event, textAnn, editIdx)) return true;
    if (handleTextNavigation(event, textAnn)) return true;
    if (handleTextDeletion(event, textAnn, editIdx)) return true;
    if (handleTextInput(event, textAnn, editIdx)) return true;

    return false;
}

bool AnnotationEventHandler::handleTextConfirmCancel(QKeyEvent* event, Annotation& textAnn, int editIdx)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            finishTextEditing();
            event->accept();
            return true;
        }
        textAnn.text.insert(toolManager_.cursorPos(), QChar::LineFeed);
        toolManager_.setCursorPos(toolManager_.cursorPos() + 1);
        toolManager_.updateTextBounds(editIdx);
        canvas_.markModified();
        event->accept();
        return true;
    }

    if (event->key() == Qt::Key_Escape) {
        if (textAnn.text.isEmpty()) {
            toolManager_.pushUndo();
            toolManager_.annotationsMut().removeAt(editIdx);
            toolManager_.setSelectedIndex(-1);
        }
        finishTextEditing();
        canvas_.markModified();
        event->accept();
        return true;
    }
    return false;
}

bool AnnotationEventHandler::handleTextCtrlShortcuts(QKeyEvent* event, Annotation& textAnn, int editIdx)
{
    if (!event->modifiers().testFlag(Qt::ControlModifier)) return false;

    switch (event->key()) {
    case Qt::Key_C:
        { const QSignalBlocker blocker(QApplication::clipboard()); QApplication::clipboard()->setText(textAnn.text); }
        event->accept(); return true;
    case Qt::Key_V: {
        QString clipText = QApplication::clipboard()->text();
        if (!clipText.isEmpty()) {
            textAnn.text.insert(toolManager_.cursorPos(), clipText);
            toolManager_.setCursorPos(toolManager_.cursorPos() + clipText.length());
            toolManager_.updateTextBounds(editIdx);
            canvas_.markModified();
        }
        event->accept(); return true;
    }
    case Qt::Key_A:
        toolManager_.setCursorPos(textAnn.text.length());
        canvas_.update();
        event->accept(); return true;
    }
    return false;
}

bool AnnotationEventHandler::handleTextNavigation(QKeyEvent* event, Annotation& textAnn)
{
    switch (event->key()) {
    case Qt::Key_Left:
        if (toolManager_.cursorPos() > 0)
            toolManager_.setCursorPos(toolManager_.cursorPos() - 1);
        canvas_.update(); event->accept(); return true;
    case Qt::Key_Right:
        if (toolManager_.cursorPos() < textAnn.text.length())
            toolManager_.setCursorPos(toolManager_.cursorPos() + 1);
        canvas_.update(); event->accept(); return true;
    case Qt::Key_Home:
        toolManager_.setCursorPos(0);
        canvas_.update(); event->accept(); return true;
    case Qt::Key_End:
        toolManager_.setCursorPos(textAnn.text.length());
        canvas_.update(); event->accept(); return true;
    }
    return false;
}

bool AnnotationEventHandler::handleTextDeletion(QKeyEvent* event, Annotation& textAnn, int editIdx)
{
    int cp = toolManager_.cursorPos();

    if (event->key() == Qt::Key_Backspace) {
        if (cp > 0) {
            toolManager_.setCursorPos(cp - 1);
            textAnn.text.remove(toolManager_.cursorPos(), 1);
            toolManager_.updateTextBounds(editIdx);
            canvas_.markModified();
        }
        event->accept(); return true;
    }
    if (event->key() == Qt::Key_Delete) {
        if (cp < textAnn.text.length()) {
            textAnn.text.remove(cp, 1);
            toolManager_.updateTextBounds(editIdx);
            canvas_.markModified();
        }
        event->accept(); return true;
    }
    return false;
}

bool AnnotationEventHandler::handleTextInput(QKeyEvent* event, Annotation& textAnn, int editIdx)
{
    QString text = event->text();
    if (!text.isEmpty() && text[0].isPrint()) {
        textAnn.text.insert(toolManager_.cursorPos(), text);
        toolManager_.setCursorPos(toolManager_.cursorPos() + text.length());
        toolManager_.updateTextBounds(editIdx);
        canvas_.markModified();
        event->accept();
        return true;
    }
    return false;
}

void AnnotationEventHandler::finishTextEditing()
{
    toolManager_.setEditingTextIndex(-1);
    toolManager_.setCursorPos(0);
    toolManager_.setPreeditString(QString());
    canvas_.update();
}

void AnnotationEventHandler::handleAnnotationDeleteKey()
{
    int sel = toolManager_.selectedIndex();
    if (sel >= 0 && sel < toolManager_.annotationCount())
        toolManager_.deleteAnnotation(sel);
}

void AnnotationEventHandler::handleDuplicateKey()
{
    int sel = toolManager_.selectedIndex();
    if (sel < 0 || sel >= toolManager_.annotationCount()) return;
    toolManager_.setEditingTextIndex(-1);
    toolManager_.setCursorPos(0);
    toolManager_.setPreeditString(QString());
    toolManager_.duplicateAnnotation(sel);
}

void AnnotationEventHandler::handleLayerReorderKey(int direction)
{
    int sel = toolManager_.selectedIndex();
    int swap = sel + direction;
    if (swap >= 0 && swap < toolManager_.annotationCount()) {
        toolManager_.pushUndo();
        qSwap(toolManager_.annotationsMut()[sel], toolManager_.annotationsMut()[swap]);
        toolManager_.setSelectedIndex(swap);
        canvas_.markModified();
        if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
    }
}

void AnnotationEventHandler::handleNudgeKey(int key)
{
    toolManager_.pushUndo();
    int step = (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) ? 10 : 1;
    QPoint delta(0, 0);
    if (key == Qt::Key_Up) delta.setY(-step);
    else if (key == Qt::Key_Down) delta.setY(step);
    else if (key == Qt::Key_Left) delta.setX(-step);
    else if (key == Qt::Key_Right) delta.setX(step);
    int sel = toolManager_.selectedIndex();
    auto& a = toolManager_.annotationsMut()[sel];
    a.bounds.translate(delta);
    if (a.tool == AnnotationTool::Pen || a.tool == AnnotationTool::Arrow
        || a.tool == AnnotationTool::Line || a.tool == AnnotationTool::Mosaic) {
        for (auto& pt : a.points) pt += delta;
    }
    canvas_.markModified();
}

void AnnotationEventHandler::handleFontSizeChange(int delta)
{
    int newSize = toolManager_.fontSize() + delta;
    if (newSize >= 8 && newSize <= 72) {
        toolManager_.setFontSize(newSize);
    }
}

void AnnotationEventHandler::handleZoomFit()
{
    auto* scrollArea = qobject_cast<QScrollArea*>(canvas_.parentWidget());
    const auto& img = toolManager_.image();
    if (scrollArea && !img.isNull()) {
        auto vp = scrollArea->viewport()->size();
        auto logicalSize = img.size() / img.devicePixelRatio();
        double fit = qMin(static_cast<double>(vp.width()) / logicalSize.width(),
                           static_cast<double>(vp.height()) / logicalSize.height());
        int newW = static_cast<int>(logicalSize.width() * fit);
        int newH = static_cast<int>(logicalSize.height() * fit);
        if (newW > 0 && newH > 0) {
            QSize newSize(newW, newH);
            canvas_.setMinimumSize(newSize);
            canvas_.resize(newSize);
            toolManager_.setZoomFactor(fit);
            canvas_.updateWindowTitle();
            canvas_.update();
            if (toolManager_.onZoomChanged) toolManager_.onZoomChanged(fit);
        }
    }
}

void AnnotationEventHandler::keyPressEvent(QKeyEvent* event)
{
    int editIdx = toolManager_.editingTextIndex();
    if (editIdx >= 0 && editIdx < toolManager_.annotationCount()
        && toolManager_.annotationAt(editIdx).tool == AnnotationTool::Text) {
        if (handleTextEditingKey(event)) return;
    }

    if (handleEscapeKey(event)) return;
    if (handleDeleteKey(event)) return;
    if (handleCtrlShortcuts(event)) return;
    if (handleToolShortcuts(event)) return;
    if (handleNudgeOrFontSize(event)) return;

    if (onKeyPressDefault) onKeyPressDefault(event);
}

bool AnnotationEventHandler::handleEscapeKey(QKeyEvent* event)
{
    if (event->key() != Qt::Key_Escape) return false;
    if (toolManager_.pickingColor()) {
        toolManager_.setPickingColor(false);
        event->accept();
        return true;
    }
    if (toolManager_.drawing()) {
        toolManager_.setDrawing(false);
        canvas_.update();
        event->accept();
        return true;
    }
    return false;
}

bool AnnotationEventHandler::handleDeleteKey(QKeyEvent* event)
{
    if (event->key() != Qt::Key_Delete && event->key() != Qt::Key_Backspace) return false;
    int sel = toolManager_.selectedIndex();
    if (sel >= 0 && sel < toolManager_.annotationCount()
        && toolManager_.editingTextIndex() < 0) {
        handleAnnotationDeleteKey();
        event->accept();
        return true;
    }
    return false;
}

bool AnnotationEventHandler::handleCtrlShortcuts(QKeyEvent* event)
{
    if (!event->modifiers().testFlag(Qt::ControlModifier)) return false;

    switch (event->key()) {
    case Qt::Key_Equal: case Qt::Key_Plus:
        canvas_.zoomAt(toolManager_.zoomFactor() * 1.15, QPoint(canvas_.width() / 2, canvas_.height() / 2));
        event->accept(); return true;
    case Qt::Key_Minus:
        canvas_.zoomAt(toolManager_.zoomFactor() / 1.15, QPoint(canvas_.width() / 2, canvas_.height() / 2));
        event->accept(); return true;
    case Qt::Key_0:
        canvas_.zoomAt(1.0, QPoint(canvas_.width() / 2, canvas_.height() / 2));
        event->accept(); return true;
    case Qt::Key_9:
        handleZoomFit();
        event->accept(); return true;
    case Qt::Key_D: {
        int sel = toolManager_.selectedIndex();
        if (sel >= 0) { handleDuplicateKey(); event->accept(); return true; }
        break;
    }
    case Qt::Key_A:
        if (toolManager_.annotationCount() > 0) {
            toolManager_.setSelectedIndex(toolManager_.annotationCount() - 1);
            canvas_.update();
            if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
            event->accept(); return true;
        }
        break;
    case Qt::Key_Up: case Qt::Key_Down:
        if (event->modifiers().testFlag(Qt::ShiftModifier)) {
            int sel = toolManager_.selectedIndex();
            if (sel >= 0 && sel < toolManager_.annotationCount()) {
                int dir = (event->key() == Qt::Key_Up) ? 1 : -1;
                handleLayerReorderKey(dir);
                event->accept(); return true;
            }
        }
        break;
    case Qt::Key_Z:
        if (!event->modifiers().testFlag(Qt::ShiftModifier)) {
            toolManager_.undo(); event->accept(); return true;
        }
        break;
    case Qt::Key_Y:
        toolManager_.redo(); event->accept(); return true;
    }
    // Shift+Z for redo
    if (event->key() == Qt::Key_Z && event->modifiers().testFlag(Qt::ShiftModifier)) {
        toolManager_.redo(); event->accept(); return true;
    }
    if (onKeyPressDefault) onKeyPressDefault(event);
    return true;
}

bool AnnotationEventHandler::handleToolShortcuts(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_R: toolManager_.setTool(AnnotationTool::Rectangle); event->accept(); return true;
    case Qt::Key_E: toolManager_.setTool(AnnotationTool::Ellipse); event->accept(); return true;
    case Qt::Key_A: toolManager_.setTool(AnnotationTool::Arrow); event->accept(); return true;
    case Qt::Key_L: toolManager_.setTool(AnnotationTool::Line); event->accept(); return true;
    case Qt::Key_P: toolManager_.setTool(AnnotationTool::Pen); event->accept(); return true;
    case Qt::Key_T: toolManager_.setTool(AnnotationTool::Text); event->accept(); return true;
    case Qt::Key_H: toolManager_.setTool(AnnotationTool::Highlight); event->accept(); return true;
    case Qt::Key_N: toolManager_.setTool(AnnotationTool::Numbered); event->accept(); return true;
    case Qt::Key_M: toolManager_.setTool(AnnotationTool::Mosaic); event->accept(); return true;
    case Qt::Key_V: toolManager_.setTool(AnnotationTool::Select); event->accept(); return true;
    case Qt::Key_X: toolManager_.setTool(AnnotationTool::Eraser); event->accept(); return true;
    case Qt::Key_C: toolManager_.setTool(AnnotationTool::Crop); event->accept(); return true;
    }
    return false;
}

bool AnnotationEventHandler::handleNudgeOrFontSize(QKeyEvent* event)
{
    int sel = toolManager_.selectedIndex();
    switch (event->key()) {
    case Qt::Key_Up: case Qt::Key_Down: case Qt::Key_Left: case Qt::Key_Right:
        if (sel >= 0 && sel < toolManager_.annotationCount()
            && toolManager_.editingTextIndex() < 0) {
            handleNudgeKey(event->key());
            event->accept(); return true;
        }
        break;
    case Qt::Key_BracketLeft:
        handleFontSizeChange(-2);
        event->accept(); return true;
    case Qt::Key_BracketRight:
        handleFontSizeChange(2);
        event->accept(); return true;
    }
    return false;
}

// --- Context menu ---

void AnnotationEventHandler::contextMenuEvent(QContextMenuEvent* event)
{
    toolManager_.setDrawing(false);
    if (toolManager_.pickingColor()) {
        toolManager_.setPickingColor(false);
    }
    if (toolManager_.editingTextIndex() >= 0) {
        toolManager_.setEditingTextIndex(-1);
        toolManager_.setCursorPos(0);
        toolManager_.setPreeditString(QString());
        canvas_.update();
    }

    int hitIdx = resolveContextHit(event->pos());
    if (hitIdx >= 0) selectContextHit(hitIdx);
    else clearContextSelection();

    ContextMenuActions actions;
    QMenu menu;
    buildContextMenu(menu, actions);
    executeContextAction(menu.exec(event->globalPos()), actions);
}

int AnnotationEventHandler::resolveContextHit(QPoint widgetPos) const
{
    QPoint clickPos = toImage(widgetPos);
    for (int i = toolManager_.annotationCount() - 1; i >= 0; --i) {
        if (AnnotationRenderer::hitTestAnnotation(toolManager_.annotationAt(i), clickPos))
            return i;
    }
    return -1;
}

void AnnotationEventHandler::selectContextHit(int hitIdx)
{
    if (hitIdx != toolManager_.selectedIndex()) {
        toolManager_.setSelectedIndex(hitIdx);
        if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
        canvas_.update();
    }
}

void AnnotationEventHandler::clearContextSelection()
{
    if (toolManager_.selectedIndex() >= 0) {
        toolManager_.setSelectedIndex(-1);
        if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
        canvas_.update();
    }
}

void AnnotationEventHandler::buildContextMenu(QMenu& menu, ContextMenuActions& actions)
{
    actions.copyImage = menu.addAction(canvas_.tr("Copy Image\tCtrl+C"));
    actions.saveAs = menu.addAction(canvas_.tr("Export..."));

    int sel = toolManager_.selectedIndex();
    if (sel >= 0) {
        menu.addSeparator();
        actions.deleteAnn = menu.addAction(canvas_.tr("Delete Annotation\tDel"));
        actions.duplicateAnn = menu.addAction(canvas_.tr("Duplicate Annotation"));
        if (sel < toolManager_.annotationCount() - 1)
            actions.bringForward = menu.addAction(canvas_.tr("Bring Forward\tCtrl+Shift+Up"));
        if (sel > 0)
            actions.sendBackward = menu.addAction(canvas_.tr("Send Backward\tCtrl+Shift+Down"));
    }

    menu.addSeparator();
    actions.zoomIn = menu.addAction(canvas_.tr("Zoom In\tCtrl++"));
    actions.zoomOut = menu.addAction(canvas_.tr("Zoom Out\tCtrl+-"));
    actions.zoom100 = menu.addAction(canvas_.tr("Actual Size (100%)\tCtrl+0"));
    actions.zoomFitAction = menu.addAction(canvas_.tr("Fit to Window\tCtrl+9"));
    menu.addSeparator();
    actions.clearAll = menu.addAction(canvas_.tr("Clear All Annotations"));
}

void AnnotationEventHandler::executeContextAction(QAction* action, ContextMenuActions& actions)
{
    int sel = toolManager_.selectedIndex();

    if (action == actions.deleteAnn && sel >= 0) {
        toolManager_.pushUndo();
        toolManager_.annotationsMut().removeAt(sel);
        toolManager_.setSelectedIndex(-1);
        canvas_.markModified();
        if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
    } else if (action == actions.duplicateAnn && sel >= 0) {
        toolManager_.pushUndo();
        auto dup = toolManager_.annotationAt(sel);
        dup.bounds.translate(10, 10);
        for (auto& pt : dup.points) pt += QPoint(10, 10);
        toolManager_.annotationsMut().push_back(std::move(dup));
        toolManager_.setSelectedIndex(toolManager_.annotationCount() - 1);
        canvas_.markModified();
        if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
    } else if (action == actions.bringForward && sel >= 0 && sel < toolManager_.annotationCount() - 1) {
        toolManager_.pushUndo();
        qSwap(toolManager_.annotationsMut()[sel], toolManager_.annotationsMut()[sel + 1]);
        toolManager_.setSelectedIndex(sel + 1);
        canvas_.markModified();
        if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
    } else if (action == actions.sendBackward && sel > 0 && sel < toolManager_.annotationCount()) {
        toolManager_.pushUndo();
        qSwap(toolManager_.annotationsMut()[sel], toolManager_.annotationsMut()[sel - 1]);
        toolManager_.setSelectedIndex(sel - 1);
        canvas_.markModified();
        if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
    } else if (action == actions.copyImage) {
        { const QSignalBlocker blocker(QApplication::clipboard()); QApplication::clipboard()->setImage(canvas_.renderedImage()); }
    } else if (action == actions.saveAs) {
        auto path = QFileDialog::getSaveFileName(&canvas_, canvas_.tr("Save As"), QString(),
            canvas_.tr("PNG (*.png);;JPEG (*.jpg *.jpeg)"));
        if (!path.isEmpty()) canvas_.renderedImage().save(path);
    } else if (action == actions.zoomIn) {
        canvas_.zoomAt(toolManager_.zoomFactor() * 1.15, QPoint(canvas_.width() / 2, canvas_.height() / 2));
    } else if (action == actions.zoomOut) {
        canvas_.zoomAt(toolManager_.zoomFactor() / 1.15, QPoint(canvas_.width() / 2, canvas_.height() / 2));
    } else if (action == actions.zoom100) {
        canvas_.zoomAt(1.0, QPoint(canvas_.width() / 2, canvas_.height() / 2));
    } else if (action == actions.zoomFitAction) {
        handleZoomFit();
    } else if (action == actions.clearAll) {
        if (toolManager_.annotationCount() > 0) {
            auto ret = QMessageBox::question(&canvas_, canvas_.tr("Clear All Annotations"),
                canvas_.tr("Are you sure you want to clear all annotations?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                toolManager_.pushUndo();
                toolManager_.clearAnnotations();
                canvas_.markModified();
                if (toolManager_.onSelectionChanged) toolManager_.onSelectionChanged();
            }
        }
    }
}

// --- Wheel ---

void AnnotationEventHandler::wheelEvent(QWheelEvent* event)
{
    if (toolManager_.image().isNull() || !event->modifiers().testFlag(Qt::ControlModifier)) {
        if (onWheelDefault) onWheelDefault(event);
        return;
    }
    auto delta = event->angleDelta().y();
    if (delta == 0) return;
    canvas_.zoomAt(toolManager_.zoomFactor() * (delta > 0 ? 1.15 : 0.85), event->pos());
    event->accept();
}

// --- Input method ---

void AnnotationEventHandler::inputMethodEvent(QInputMethodEvent* event)
{
    int editIdx = toolManager_.editingTextIndex();
    if (editIdx < 0 || editIdx >= toolManager_.annotationCount()
        || toolManager_.annotationAt(editIdx).tool != AnnotationTool::Text) {
        if (onInputMethodDefault) onInputMethodDefault(event);
        return;
    }
    auto& a = toolManager_.annotationsMut()[editIdx];
    if (!event->commitString().isEmpty()) {
        a.text.insert(toolManager_.cursorPos(), event->commitString());
        toolManager_.setCursorPos(toolManager_.cursorPos() + event->commitString().length());
        toolManager_.setPreeditString(QString());
        toolManager_.updateTextBounds(editIdx);
        canvas_.markModified();
    }
    toolManager_.setPreeditString(event->preeditString());
    canvas_.update();
    event->accept();
}

QVariant AnnotationEventHandler::inputMethodQuery(Qt::InputMethodQuery query) const
{
    int editIdx = toolManager_.editingTextIndex();
    if (editIdx >= 0 && editIdx < toolManager_.annotationCount()
        && toolManager_.annotationAt(editIdx).tool == AnnotationTool::Text) {
        const auto& a = toolManager_.annotationAt(editIdx);
        switch (query) {
        case Qt::ImCursorRectangle: {
            QFont font(a.fontFamily.isEmpty() ? QApplication::font().family() : a.fontFamily);
            font.setPixelSize(a.textFontSize > 0 ? a.textFontSize : toolManager_.fontSize());
            font.setBold(a.bold);
            font.setItalic(a.italic);
            font.setUnderline(a.underline);
            QFontMetrics fm(font);
            int textWidth = fm.horizontalAdvance(a.text.left(toolManager_.cursorPos()) + toolManager_.preeditString());
            double zf = toolManager_.zoomFactor();
            int cx = static_cast<int>((a.bounds.left() + 4 + textWidth) * zf);
            int cy = static_cast<int>((a.bounds.top() + 4) * zf);
            int ch = static_cast<int>(fm.height() * zf);
            return QRect(canvas_.mapToGlobal(QPoint(0, 0)) + QPoint(cx, cy), QSize(4, ch));
        }
        case Qt::ImEnabled:
            return true;
        case Qt::ImFont: {
            QFont f(a.fontFamily.isEmpty() ? QApplication::font().family() : a.fontFamily);
            f.setPixelSize(a.textFontSize > 0 ? a.textFontSize : toolManager_.fontSize());
            f.setBold(a.bold);
            f.setItalic(a.italic);
            f.setUnderline(a.underline);
            return f;
        }
        case Qt::ImCursorPosition:
            return toolManager_.cursorPos();
        case Qt::ImSurroundingText:
            return a.text + toolManager_.preeditString();
        case Qt::ImCurrentSelection:
            return QString();
        case Qt::ImAnchorPosition:
            return toolManager_.cursorPos();
        default:
            break;
        }
    }
    if (onInputMethodQueryDefault) return onInputMethodQueryDefault(query);
    return QVariant();
}

} // namespace snappaste
