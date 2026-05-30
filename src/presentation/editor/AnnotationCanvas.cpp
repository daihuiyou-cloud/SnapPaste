#define _USE_MATH_DEFINES
#include "presentation/editor/AnnotationCanvas.h"
#include "presentation/editor/ImageBlur.h"

#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFont>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace snappaste {

AnnotationCanvas::AnnotationCanvas(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setMinimumSize(640, 360);
    QSettings settings;
    fontSize_ = settings.value("editor/fontSize", 14).toInt();
    const auto saved = settings.value("editor/recentColors").toList();
    for (const auto& v : saved) {
        QColor c(v.toString());
        if (c.isValid())
            customColors_.append(c);
    }
}

QPoint AnnotationCanvas::toImage(QPoint widgetPt) const
{
    return QPoint(static_cast<int>(widgetPt.x() / zoomFactor_),
                  static_cast<int>(widgetPt.y() / zoomFactor_));
}

void AnnotationCanvas::setImage(QImage image)
{
    zoomFactor_ = 1.0;
    image_ = std::move(image);
    annotations_.clear();
    undoStack_.clear();
    redoStack_.clear();
    modified_ = false;
    cacheValid_ = false;
    nextNumber_ = 1;
    editingTextIndex_ = -1;
    preeditString_.clear();
    auto dpr = image_.devicePixelRatio();
    QSize logicalSize(image_.size() / dpr);
    setMinimumSize(logicalSize);
    resize(logicalSize);
    updateWindowTitle();
    update();
}

void AnnotationCanvas::applyCrop(QRect cropRect)
{
    auto dpr = image_.devicePixelRatio();
    QRect physicalCrop(qRound(cropRect.x() * dpr),
                       qRound(cropRect.y() * dpr),
                       qRound(cropRect.width() * dpr),
                       qRound(cropRect.height() * dpr));
    physicalCrop = physicalCrop.intersected(image_.rect());
    if (physicalCrop.width() < 5 || physicalCrop.height() < 5) return;

    image_ = image_.copy(physicalCrop);
    annotationCache_ = {};
    cacheValid_ = false;
    annotations_.clear();
    annotations_.squeeze();
    selectedIndex_ = -1;
    undoStack_.clear();
    redoStack_.clear();
    nextNumber_ = 1;
    markModified();

    auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
    if (scrollArea) {
        auto vp = scrollArea->viewport()->size();
        auto logicalSize = image_.size() / image_.devicePixelRatio();
        double fit = qMin(static_cast<double>(vp.width()) / logicalSize.width(),
                           static_cast<double>(vp.height()) / logicalSize.height());
        if (fit > 1.0) fit = 1.0;
        zoomFactor_ = fit;
    }

    auto logicalSize = image_.size() / image_.devicePixelRatio();
    setMinimumSize(logicalSize);
    resize(logicalSize);
    updateWindowTitle();
    update();
    emit imageEdited(image_);
}

void AnnotationCanvas::clearModified() { modified_ = false; updateWindowTitle(); }

bool AnnotationCanvas::isModified() const { return modified_; }

void AnnotationCanvas::markModified()
{
    modified_ = true;
    cacheValid_ = false;
    if (onModified_) onModified_();
    updateWindowTitle();
    update();
}

void AnnotationCanvas::zoomAt(double factor, QPoint center)
{
    const auto oldCenter = toImage(center);
    zoomFactor_ = std::max(0.1, std::min(5.0, factor));
    auto logicalSize = image_.size() / image_.devicePixelRatio();
    QSize newSize(static_cast<int>(logicalSize.width() * zoomFactor_),
                  static_cast<int>(logicalSize.height() * zoomFactor_));
    setMinimumSize(newSize);
    resize(newSize);
    const auto newWidgetCenter = QPoint(static_cast<int>(oldCenter.x() * zoomFactor_),
                                        static_cast<int>(oldCenter.y() * zoomFactor_));
    auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
    if (scrollArea) {
        auto vp = scrollArea->viewport();
        auto scrollDelta = newWidgetCenter - center;
        scrollArea->horizontalScrollBar()->setValue(
            scrollArea->horizontalScrollBar()->value() + scrollDelta.x());
        scrollArea->verticalScrollBar()->setValue(
            scrollArea->verticalScrollBar()->value() + scrollDelta.y());
    }
    updateWindowTitle();
    update();
    if (onZoomChanged_) onZoomChanged_(zoomFactor_);
}

void AnnotationCanvas::updateWindowTitle()
{
    auto* w = window();
    if (w) {
        QString title;
        if (modified_) title += "* ";
        title += tr("SnapPaste Editor");
        if (!image_.isNull()) {
            title += tr(" - %1x%2").arg(image_.width()).arg(image_.height());
        }
        title += tr(" - %1%").arg(static_cast<int>(zoomFactor_ * 100));
        int annCount = annotations_.size();
        if (annCount > 0) {
            title += tr(" - %1 ann").arg(annCount);
        }
        w->setWindowTitle(title);
    }
}

QImage AnnotationCanvas::renderedImage() const
{
    if (image_.isNull()) {
        return {};
    }

    if (cacheValid_) {
        return annotationCache_;
    }

    annotationCache_ = image_.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    {
        QPainter painter(&annotationCache_);
        drawAnnotations(&painter, image_, false);
    }
    cacheValid_ = true;
    return annotationCache_;
}

void AnnotationCanvas::setTool(AnnotationTool tool)
{
    if (currentTool_ == tool) return;
    currentTool_ = tool;
    switch (tool) {
    case AnnotationTool::Pen: setCursor(Qt::CrossCursor); break;
    case AnnotationTool::Text: setCursor(Qt::IBeamCursor); break;
    case AnnotationTool::Eraser:
    case AnnotationTool::Mosaic: setCursor(Qt::PointingHandCursor); break;
    case AnnotationTool::Crop: setCursor(Qt::CrossCursor); break;
    default: setCursor(Qt::CrossCursor); break;
    }
    recentTools_.removeAll(tool);
    recentTools_.prepend(tool);
    if (recentTools_.size() > 4) recentTools_.resize(4);
    update();
    emit toolChanged(tool);
}

void AnnotationCanvas::setColor(const QColor& color)
{
    currentColor_ = color;
}

void AnnotationCanvas::setStrokeWidth(int width)
{
    currentStrokeWidth_ = std::clamp(width, 1, 12);
}

void AnnotationCanvas::setPickingColor(bool picking)
{
    pickingColor_ = picking;
    setCursor(picking ? Qt::CrossCursor : Qt::ArrowCursor);
    if (onPickingColorChanged_) onPickingColorChanged_(picking);
    update();
}

void AnnotationCanvas::setOnPickingColorChanged(std::function<void(bool)> cb) { onPickingColorChanged_ = std::move(cb); }

void AnnotationCanvas::setMosaicBlurred(bool blurred)
{
    mosaicBlurred_ = blurred;
}

void AnnotationCanvas::setTextOutlineEnabled(bool enabled)
{
    textOutlineEnabled_ = enabled;
}

void AnnotationCanvas::setFilled(bool filled)
{
    filled_ = filled;
}

void AnnotationCanvas::updateTextBounds(int index)
{
    if (index < 0 || index >= annotations_.size()) return;
    auto& a = annotations_[index];
    if (a.tool != AnnotationTool::Text) return;
    QFont font("Microsoft YaHei UI", a.textFontSize > 0 ? a.textFontSize : fontSize_);
    QFontMetrics fm(font);
    const auto textRect = fm.boundingRect(QRect(0, 0, 4096, 4096), Qt::AlignLeft | Qt::AlignTop, a.text);
    QRect newBounds(a.bounds.topLeft(), QSize(qMax(textRect.width() + 8, 20), qMax(textRect.height() + 8, 20)));
    auto logicalW = image_.width() / image_.devicePixelRatio();
    if (newBounds.right() > logicalW) {
        newBounds.moveRight(logicalW - 4);
    }
    a.bounds = newBounds;
}

int AnnotationCanvas::fontSize() const { return fontSize_; }
void AnnotationCanvas::setFontSize(int size)
{
    size = qBound(8, size, 72);
    if (size != fontSize_) {
        fontSize_ = size;
        if (currentTool_ == AnnotationTool::Text || currentTool_ == AnnotationTool::Numbered) {
            markModified();
        }
        update();
        if (onFontSizeChanged_) onFontSizeChanged_(fontSize_);
        QSettings().setValue("editor/fontSize", fontSize_);
    }
}
void AnnotationCanvas::setOnFontSizeChanged(std::function<void(int)> cb) { onFontSizeChanged_ = std::move(cb); }
double AnnotationCanvas::zoomFactor() const { return zoomFactor_; }
QSize AnnotationCanvas::imageSize() const { return image_.size(); }
QColor AnnotationCanvas::color() const { return currentColor_; }
int AnnotationCanvas::strokeWidth() const { return currentStrokeWidth_; }
void AnnotationCanvas::setOnZoomChanged(std::function<void(double)> cb) { onZoomChanged_ = std::move(cb); }

int AnnotationCanvas::strokeAlpha() const { return strokeAlpha_; }
void AnnotationCanvas::setStrokeAlpha(int alpha)
{
    strokeAlpha_ = std::clamp(alpha, 0, 255);
    if (onStrokeAlphaChanged_) onStrokeAlphaChanged_(strokeAlpha_);
}
void AnnotationCanvas::setOnStrokeAlphaChanged(std::function<void(int)> cb) { onStrokeAlphaChanged_ = std::move(cb); }

ArrowStyle AnnotationCanvas::arrowStyle() const { return arrowStyle_; }
void AnnotationCanvas::setArrowStyle(ArrowStyle style)
{
    arrowStyle_ = style;
    if (onArrowStyleChanged_) onArrowStyleChanged_(static_cast<int>(style));
}
void AnnotationCanvas::setOnArrowStyleChanged(std::function<void(int)> cb) { onArrowStyleChanged_ = std::move(cb); }

int AnnotationCanvas::cornerRadius() const { return cornerRadius_; }
void AnnotationCanvas::setCornerRadius(int radius)
{
    cornerRadius_ = std::clamp(radius, 0, 40);
    if (onCornerRadiusChanged_) onCornerRadiusChanged_(cornerRadius_);
}
void AnnotationCanvas::setOnCornerRadiusChanged(std::function<void(int)> cb) { onCornerRadiusChanged_ = std::move(cb); }

bool AnnotationCanvas::gridEnabled() const { return gridEnabled_; }
void AnnotationCanvas::setGridEnabled(bool enabled)
{
    gridEnabled_ = enabled;
    update();
}

QPointF AnnotationCanvas::mouseImagePos() const { return mouseImagePos_; }
QColor AnnotationCanvas::mousePixelColor() const { return mousePixelColor_; }
void AnnotationCanvas::setOnMouseInfoChanged(std::function<void(QPointF, QColor)> cb) { onMouseInfoChanged_ = std::move(cb); }
void AnnotationCanvas::setOnModified(std::function<void()> cb) { onModified_ = std::move(cb); }

const QVector<AnnotationTool>& AnnotationCanvas::recentTools() const { return recentTools_; }

int AnnotationCanvas::undoCount() const { return undoStack_.size(); }
int AnnotationCanvas::redoCount() const { return redoStack_.size(); }

const QVector<QColor>& AnnotationCanvas::recentColors() const { return customColors_; }

void AnnotationCanvas::addRecentColor(const QColor& color)
{
    customColors_.removeAll(color);
    customColors_.prepend(color);
    if (customColors_.size() > 6) {
        customColors_.resize(6);
    }
    QVariantList saved;
    for (const auto& c : customColors_)
        saved.append(c.name());
    QSettings().setValue("editor/recentColors", saved);
}

void AnnotationCanvas::undo()
{
    if (undoStack_.isEmpty()) {
        return;
    }
    redoStack_.push_back(annotations_);
    annotations_ = undoStack_.takeLast();
    markModified();
}

void AnnotationCanvas::pushUndo()
{
    undoStack_.push_back(annotations_);
    if (undoStack_.size() > kMaxUndo) {
        undoStack_.removeFirst();
    }
}

void AnnotationCanvas::redo()
{
    if (redoStack_.isEmpty()) {
        return;
    }
    pushUndo();
    annotations_ = redoStack_.takeLast();
    markModified();
}

void AnnotationCanvas::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void AnnotationCanvas::dropEvent(QDropEvent* event)
{
    const auto urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        const auto path = urls.first().toLocalFile();
        QImage img(path);
        if (!img.isNull()) {
            setImage(img);
            event->acceptProposedAction();
        }
    }
}

void AnnotationCanvas::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (image_.isNull()) {
        return;
    }
    if (currentTool_ == AnnotationTool::Select && selectedIndex_ >= 0) {
        if (annotations_[selectedIndex_].tool == AnnotationTool::Text) {
            bool ok = false;
            const auto newText = QInputDialog::getMultiLineText(
                static_cast<QWidget*>(parent()), tr("Edit Text"), tr("Edit text:"),
                annotations_[selectedIndex_].text, &ok);
            if (ok && !newText.isEmpty() && newText != annotations_[selectedIndex_].text) {
                pushUndo();
                redoStack_.clear();
                annotations_[selectedIndex_].text = newText;
                update();
            }
        } else {
            annotations_.removeAt(selectedIndex_);
            selectedIndex_ = -1;
            markModified();
            update();
        }
        event->accept();
        return;
    }
    if (currentTool_ != AnnotationTool::Text) {
        return;
    }

    const auto pos = toImage(event->pos());

    for (int i = annotations_.size() - 1; i >= 0; --i) {
        if (annotations_[i].tool == AnnotationTool::Text && annotations_[i].bounds.contains(pos)) {
            bool ok = false;
            const auto newText = QInputDialog::getMultiLineText(
                static_cast<QWidget*>(parent()), tr("Edit Text"), tr("Edit text:"), annotations_[i].text, &ok);
            if (ok && !newText.isEmpty() && newText != annotations_[i].text) {
                undoStack_.push_back(annotations_);
                redoStack_.clear();
                annotations_[i].text = newText;
                update();
            }
            return;
        }
    }
}

void AnnotationCanvas::handlePanningPress(QMouseEvent* event)
{
    panning_ = true;
    panStart_ = event->pos();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
}

bool AnnotationCanvas::handlePickingColorPress(QMouseEvent* event)
{
    if (!pickingColor_) return false;
    pickingColor_ = false;
    setCursor(Qt::ArrowCursor);
    const auto pos = toImage(event->pos());
    QRect logicalImageRect(QPoint(0, 0), image_.size() / image_.devicePixelRatio());
    if (logicalImageRect.contains(pos)) {
        QImage composited = image_.copy();
        QPainter p(&composited);
        drawAnnotations(&p, image_, false);
        p.end();
        const auto dpr = image_.devicePixelRatio();
        QPoint physicalPos(static_cast<int>(pos.x() * dpr),
                           static_cast<int>(pos.y() * dpr));
        currentColor_ = QColor::fromRgba(composited.pixel(physicalPos));
    }
    update();
    return true;
}

bool AnnotationCanvas::handleSelectPress(const QPoint& pos)
{
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        const auto r = annotations_[selectedIndex_].bounds;
        const QPoint corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
        for (int ci = 0; ci < 4; ++ci) {
            if (QRect(corners[ci].x() - 8, corners[ci].y() - 8, 16, 16).contains(pos)) {
                undoStack_.push_back(annotations_);
                redoStack_.clear();
                resizing_ = true;
                resizeCorner_ = ci;
                resizeStartBounds_ = r;
                resizeStartPoints_ = annotations_[selectedIndex_].points;
                return true;
            }
        }
    }
    for (int i = annotations_.size() - 1; i >= 0; --i) {
        if (hitTestAnnotation(annotations_.at(i), pos)) {
            undoStack_.push_back(annotations_);
            redoStack_.clear();
            selectedIndex_ = i;
            moving_ = true;
            moveOffset_ = annotations_[i].bounds.topLeft() - pos;
            update();
            return true;
        }
    }
    selectedIndex_ = -1;
    update();
    return true;
}

bool AnnotationCanvas::handleEraserPress(const QPoint& pos)
{
    for (int i = annotations_.size() - 1; i >= 0; --i) {
        if (hitTestAnnotation(annotations_.at(i), pos)) {
            undoStack_.push_back(annotations_);
            redoStack_.clear();
            annotations_.removeAt(i);
            markModified();
            return true;
        }
    }
    return true;
}

bool AnnotationCanvas::handleNumberedPress(const QPoint& pos)
{
    pushUndo();
    redoStack_.clear();
    Annotation ann;
    ann.tool = AnnotationTool::Numbered;
    ann.bounds = QRect(pos.x() - kDefaultNumberedSize / 2, pos.y() - kDefaultNumberedSize / 2,
                       kDefaultNumberedSize, kDefaultNumberedSize);
    ann.color = currentColor_;
    ann.number = nextNumber_++;
    annotations_.push_back(std::move(ann));
    selectedIndex_ = annotations_.size() - 1;
    markModified();
    return true;
}

bool AnnotationCanvas::handleTextPress(const QPoint& pos)
{
    if (editingTextIndex_ >= 0) {
        editingTextIndex_ = -1;
        preeditString_.clear();
        update();
    }
    for (int i = annotations_.size() - 1; i >= 0; --i) {
        if (annotations_.at(i).tool == AnnotationTool::Text && hitTestAnnotation(annotations_.at(i), pos)) {
            selectedIndex_ = i;
            editingTextIndex_ = i;
            update();
            return true;
        }
    }
    QFont font("Microsoft YaHei UI", fontSize_);
    QFontMetrics fm(font);
    QRect bounds(pos.x(), pos.y(), 28, fm.height() + 8);
    auto logicalW = image_.width() / image_.devicePixelRatio();
    if (bounds.right() > logicalW) {
        bounds.moveRight(logicalW - 4);
    }
    pushUndo();
    redoStack_.clear();
    Annotation ann;
    ann.tool = AnnotationTool::Text;
    ann.bounds = bounds;
    ann.text = QString();
    ann.color = currentColor_;
    ann.strokeWidth = 2;
    ann.textFontSize = fontSize_;
    ann.textOutline = textOutlineEnabled_;
    annotations_.push_back(std::move(ann));
    selectedIndex_ = annotations_.size() - 1;
    editingTextIndex_ = selectedIndex_;
    markModified();
    setFocus();
    return true;
}

bool AnnotationCanvas::handleExistingAnnotationPress(const QPoint& pos)
{
    if (currentTool_ == AnnotationTool::Select || currentTool_ == AnnotationTool::Eraser
        || currentTool_ == AnnotationTool::Numbered) {
        return false;
    }
    for (int i = annotations_.size() - 1; i >= 0; --i) {
        if (hitTestAnnotation(annotations_.at(i), pos)) {
            if (i != selectedIndex_) {
                selectedIndex_ = i;
                update();
            }
            return true;
        }
    }
    return false;
}

void AnnotationCanvas::startDrawingAnnotation(const QPoint& pos)
{
    drawing_ = true;
    start_ = pos;
    current_ = start_;
    draft_ = Annotation{};
    draft_.tool = currentTool_;
    draft_.color = currentColor_;
    draft_.color.setAlpha(strokeAlpha_);
    draft_.strokeWidth = currentStrokeWidth_;
    draft_.blurRadius = (currentTool_ == AnnotationTool::Mosaic && mosaicBlurred_) ? currentStrokeWidth_ : 0;
    draft_.filled = filled_;
    draft_.arrowStyle = arrowStyle_;
    draft_.cornerRadius = cornerRadius_;
    draft_.textFontSize = fontSize_;
    draft_.bounds = QRect(start_, current_);
    draft_.points = {start_};
    update();
}

void AnnotationCanvas::mousePressEvent(QMouseEvent* event)
{
    if (image_.isNull()) return;

    setFocus();

    if (editingTextIndex_ >= 0 && event->button() == Qt::LeftButton) {
        const auto pos = toImage(event->pos());
        bool clickedSelf = (editingTextIndex_ < annotations_.size()
            && annotations_[editingTextIndex_].tool == AnnotationTool::Text
            && hitTestAnnotation(annotations_[editingTextIndex_], pos));
        if (!clickedSelf) {
            editingTextIndex_ = -1;
            preeditString_.clear();
            update();
        }
    }

    if (event->button() == Qt::MiddleButton) {
        handlePanningPress(event);
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (handlePickingColorPress(event)) return;

    const auto pos = toImage(event->pos());

    if (currentTool_ == AnnotationTool::Select) { handleSelectPress(pos); return; }
    if (currentTool_ == AnnotationTool::Eraser) { handleEraserPress(pos); return; }
    if (currentTool_ == AnnotationTool::Numbered) { handleNumberedPress(pos); return; }
    if (currentTool_ == AnnotationTool::Text) { handleTextPress(pos); return; }

    if (!handleExistingAnnotationPress(pos)) {
        startDrawingAnnotation(pos);
    }
}

void AnnotationCanvas::updateMouseInfo(QMouseEvent* event)
{
    if (onMouseInfoChanged_ && !image_.isNull()) {
        mouseImagePos_ = QPointF(toImage(event->pos()));
        auto dpr = image_.devicePixelRatio();
        QPoint px(static_cast<int>(mouseImagePos_.x() * dpr),
                  static_cast<int>(mouseImagePos_.y() * dpr));
        QRect imgRect(QPoint(0, 0), image_.size());
        if (imgRect.contains(px)) {
            mousePixelColor_ = QColor::fromRgba(image_.pixel(px));
            onMouseInfoChanged_(mouseImagePos_, mousePixelColor_);
        }
    }
}

void AnnotationCanvas::updateMoveCursor(QMouseEvent* event)
{
    if (currentTool_ == AnnotationTool::Select && selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        const auto pos = toImage(event->pos());
        const auto r = annotations_[selectedIndex_].bounds;
        const QPoint corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
        Qt::CursorShape cursor = Qt::ArrowCursor;
        for (int ci = 0; ci < 4; ++ci) {
            if (QRect(corners[ci].x() - 8, corners[ci].y() - 8, 16, 16).contains(pos)) {
                cursor = (ci == 0 || ci == 3) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
                break;
            }
        }
        setCursor(cursor);
    } else if (currentTool_ == AnnotationTool::Text)
        setCursor(Qt::IBeamCursor);
    else if (currentTool_ == AnnotationTool::Eraser || currentTool_ == AnnotationTool::Mosaic)
        setCursor(Qt::PointingHandCursor);
    else
        setCursor(Qt::ArrowCursor);
}

void AnnotationCanvas::handleMovePan(QMouseEvent* event)
{
    auto delta = event->pos() - panStart_;
    auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
    if (scrollArea) {
        scrollArea->horizontalScrollBar()->setValue(
            scrollArea->horizontalScrollBar()->value() - delta.x());
        scrollArea->verticalScrollBar()->setValue(
            scrollArea->verticalScrollBar()->value() - delta.y());
    }
    panStart_ = event->pos();
}

void AnnotationCanvas::handleMoveSelect(QMouseEvent* event)
{
    if (resizing_) {
        auto& a = annotations_[selectedIndex_];
        auto b = resizeStartBounds_;
        const auto p = toImage(event->pos());
        switch (resizeCorner_) {
        case 0: b.setTopLeft(p); break;
        case 1: b.setTopRight(p); break;
        case 2: b.setBottomLeft(p); break;
        case 3: b.setBottomRight(p); break;
        }
        a.bounds = b.normalized();
        if (event->modifiers().testFlag(Qt::ShiftModifier)) {
            double aspect = static_cast<double>(resizeStartBounds_.width()) / resizeStartBounds_.height();
            auto newSize = a.bounds.size();
            newSize.setWidth(static_cast<int>(newSize.height() * aspect));
            switch (resizeCorner_) {
            case 0: a.bounds.setTopLeft(QPoint(a.bounds.right() - newSize.width() + 1, a.bounds.bottom() - newSize.height() + 1)); break;
            case 1: a.bounds.setTopRight(QPoint(a.bounds.left() + newSize.width() - 1, a.bounds.bottom() - newSize.height() + 1)); break;
            case 2: a.bounds.setBottomLeft(QPoint(a.bounds.right() - newSize.width() + 1, a.bounds.top() + newSize.height() - 1)); break;
            case 3: a.bounds.setSize(newSize); break;
            }
        }
        if (a.tool == AnnotationTool::Pen) {
            if (resizeStartBounds_.width() > 0 && resizeStartBounds_.height() > 0) {
                const auto sx = a.bounds.width() / static_cast<double>(resizeStartBounds_.width());
                const auto sy = a.bounds.height() / static_cast<double>(resizeStartBounds_.height());
                a.points.clear();
                a.points.reserve(resizeStartPoints_.size());
                for (const auto& pt : resizeStartPoints_) {
                    a.points.push_back(QPoint(
                        resizeStartBounds_.x() + static_cast<int>((pt.x() - resizeStartBounds_.x()) * sx),
                        resizeStartBounds_.y() + static_cast<int>((pt.y() - resizeStartBounds_.y()) * sy)));
                }
            }
        }
        update();
        return;
    }
    if (moving_) {
        const auto imagePos = toImage(event->pos());
        const auto newPos = imagePos + moveOffset_;
        const auto delta = newPos - annotations_.at(selectedIndex_).bounds.topLeft();
        annotations_[selectedIndex_].bounds.translate(delta);
        if (annotations_[selectedIndex_].tool == AnnotationTool::Pen) {
            for (auto& pt : annotations_[selectedIndex_].points) {
                pt += delta;
            }
        }
        moveOffset_ = annotations_[selectedIndex_].bounds.topLeft() - imagePos;
        update();
    }
}

void AnnotationCanvas::updateDrawingStroke(QMouseEvent* event)
{
    if (draft_.tool == AnnotationTool::Numbered) {
        return;
    }

    auto rawPos = event->pos();
    if (!(rawPos == start_) && event->modifiers().testFlag(Qt::ShiftModifier)) {
        if (draft_.tool == AnnotationTool::Rectangle || draft_.tool == AnnotationTool::Ellipse) {
            auto dx = rawPos.x() - start_.x();
            auto dy = rawPos.y() - start_.y();
            int side = std::max(std::abs(dx), std::abs(dy));
            rawPos.setX(start_.x() + (dx >= 0 ? side : -side));
            rawPos.setY(start_.y() + (dy >= 0 ? side : -side));
        } else if (draft_.tool == AnnotationTool::Arrow || draft_.tool == AnnotationTool::Line) {
            double angle = std::atan2(rawPos.y() - start_.y(), rawPos.x() - start_.x());
            double snapped = std::round(angle / (M_PI / 4)) * (M_PI / 4);
            double dist = std::sqrt(std::pow(rawPos.x() - start_.x(), 2) +
                                    std::pow(rawPos.y() - start_.y(), 2));
            rawPos.setX(start_.x() + static_cast<int>(dist * std::cos(snapped)));
            rawPos.setY(start_.y() + static_cast<int>(dist * std::sin(snapped)));
        } else if (draft_.tool == AnnotationTool::Pen) {
            auto dx = std::abs(rawPos.x() - start_.x());
            auto dy = std::abs(rawPos.y() - start_.y());
            if (dx >= dy) {
                rawPos.setY(start_.y());
            } else {
                rawPos.setX(start_.x());
            }
        }
    }
    auto oldCurrent = current_;
    current_ = rawPos;
    draft_.bounds = QRect(start_, current_).normalized();
    if (draft_.tool == AnnotationTool::Pen) {
        draft_.points.push_back(current_);
        const int margin = currentStrokeWidth_ + 4;
        update(QRect(oldCurrent, current_).normalized().adjusted(-margin, -margin, margin, margin));
    } else if (draft_.tool == AnnotationTool::Mosaic) {
        draft_.points.push_back(current_);
        const int margin = currentStrokeWidth_ * 4 + 4;
        update(QRect(oldCurrent, current_).normalized().adjusted(-margin, -margin, margin, margin));
    } else {
        update();
    }
}

void AnnotationCanvas::mouseMoveEvent(QMouseEvent* event)
{
    updateMouseInfo(event);
    if (!drawing_) {
        updateMoveCursor(event);
    }
    if (panning_) {
        handleMovePan(event);
        event->accept();
        return;
    }
    if (currentTool_ == AnnotationTool::Select && selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        handleMoveSelect(event);
        if (resizing_ || moving_) {
            event->accept();
            return;
        }
    }
    if (!drawing_) {
        return;
    }
    updateDrawingStroke(event);
}

void AnnotationCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (panning_) {
        panning_ = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    if (currentTool_ == AnnotationTool::Select) {
        if (resizing_ || moving_) {
            resizing_ = false;
            moving_ = false;
            return;
        }
    }

    if (!drawing_ || draft_.tool == AnnotationTool::Numbered) {
        return;
    }

    drawing_ = false;
    current_ = toImage(event->pos());
    draft_.bounds = QRect(start_, current_).normalized();
    if (draft_.tool == AnnotationTool::Crop) {
        if (draft_.bounds.width() > 5 && draft_.bounds.height() > 5) {
            applyCrop(draft_.bounds);
        }
        update();
        return;
    }
    if (draft_.tool == AnnotationTool::Arrow) {
        draft_.points = {start_, current_};
    }
    if (draft_.bounds.width() > 2 || draft_.bounds.height() > 2 || draft_.tool == AnnotationTool::Pen || draft_.tool == AnnotationTool::Mosaic) {
        pushUndo();
        redoStack_.clear();
        annotations_.push_back(draft_);
        selectedIndex_ = annotations_.size() - 1;
        markModified();
        if (currentTool_ != AnnotationTool::Select && currentTool_ != AnnotationTool::Text
            && currentTool_ != AnnotationTool::Numbered && currentTool_ != AnnotationTool::Mosaic
            && currentTool_ != AnnotationTool::Eraser) {
            setTool(AnnotationTool::Select);
        }
    } else {
        update();
    }
}

void AnnotationCanvas::handleTextEditingKey(QKeyEvent* event)
{
    auto& textAnn = annotations_[editingTextIndex_];
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        editingTextIndex_ = -1;
        preeditString_.clear();
        update();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (textAnn.text.isEmpty()) {
            pushUndo();
            redoStack_.clear();
            annotations_.removeAt(editingTextIndex_);
            selectedIndex_ = -1;
        }
        editingTextIndex_ = -1;
        preeditString_.clear();
        markModified();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Backspace) {
        if (!textAnn.text.isEmpty()) {
            textAnn.text.chop(1);
            updateTextBounds(editingTextIndex_);
            markModified();
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete) {
        event->accept();
        return;
    }
    QString text = event->text();
    if (!text.isEmpty() && text[0].isPrint()) {
        textAnn.text += text;
        updateTextBounds(editingTextIndex_);
        markModified();
        event->accept();
    }
}

void AnnotationCanvas::handleZoomFit()
{
    auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
    if (scrollArea && !image_.isNull()) {
        auto vp = scrollArea->viewport()->size();
        auto logicalSize = image_.size() / image_.devicePixelRatio();
        double fit = qMin(static_cast<double>(vp.width()) / logicalSize.width(),
                           static_cast<double>(vp.height()) / logicalSize.height());
        int newW = static_cast<int>(logicalSize.width() * fit);
        int newH = static_cast<int>(logicalSize.height() * fit);
        if (newW > 0 && newH > 0) {
            QSize newSize(newW, newH);
            setMinimumSize(newSize);
            resize(newSize);
            zoomFactor_ = fit;
            updateWindowTitle();
            update();
            if (onZoomChanged_) onZoomChanged_(zoomFactor_);
        }
    }
}

void AnnotationCanvas::handleAnnotationDeleteKey()
{
    pushUndo();
    redoStack_.clear();
    annotations_.removeAt(selectedIndex_);
    selectedIndex_ = -1;
    markModified();
}

void AnnotationCanvas::handleDuplicateKey()
{
    editingTextIndex_ = -1;
    preeditString_.clear();
    auto dup = annotations_.at(selectedIndex_);
    dup.bounds.translate(10, 10);
    pushUndo();
    redoStack_.clear();
    annotations_.push_back(std::move(dup));
    selectedIndex_ = annotations_.size() - 1;
    markModified();
}

void AnnotationCanvas::handleLayerReorderKey(int direction)
{
    int swap = selectedIndex_ + direction;
    if (swap >= 0 && swap < annotations_.size()) {
        pushUndo();
        redoStack_.clear();
        qSwap(annotations_[selectedIndex_], annotations_[swap]);
        selectedIndex_ = swap;
        markModified();
    }
}

void AnnotationCanvas::handleNudgeKey(int key)
{
    int step = (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) ? 10 : 1;
    QPoint delta(0, 0);
    if (key == Qt::Key_Up) delta.setY(-step);
    else if (key == Qt::Key_Down) delta.setY(step);
    else if (key == Qt::Key_Left) delta.setX(-step);
    else if (key == Qt::Key_Right) delta.setX(step);
    annotations_[selectedIndex_].bounds.translate(delta);
    if (annotations_[selectedIndex_].tool == AnnotationTool::Pen) {
        for (auto& pt : annotations_[selectedIndex_].points) pt += delta;
    }
    markModified();
}

void AnnotationCanvas::handleFontSizeChange(int delta)
{
    int newSize = fontSize_ + delta;
    if (newSize >= 8 && newSize <= 72) {
        fontSize_ = newSize;
        markModified();
        update();
        if (onFontSizeChanged_) onFontSizeChanged_(fontSize_);
        QSettings().setValue("editor/fontSize", fontSize_);
    }
}

void AnnotationCanvas::keyPressEvent(QKeyEvent* event)
{
    if (editingTextIndex_ >= 0 && editingTextIndex_ < annotations_.size()
        && annotations_[editingTextIndex_].tool == AnnotationTool::Text) {
        handleTextEditingKey(event);
        return;
    }

    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
        && selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
        && editingTextIndex_ < 0) {
        handleAnnotationDeleteKey();
        event->accept();
        return;
    }
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (event->key() == Qt::Key_Equal || event->key() == Qt::Key_Plus) {
            zoomAt(zoomFactor_ * 1.15, QPoint(width() / 2, height() / 2));
            event->accept(); return;
        }
        if (event->key() == Qt::Key_Minus) {
            zoomAt(zoomFactor_ / 1.15, QPoint(width() / 2, height() / 2));
            event->accept(); return;
        }
        if (event->key() == Qt::Key_0) {
            zoomAt(1.0, QPoint(width() / 2, height() / 2));
            event->accept(); return;
        }
        if (event->key() == Qt::Key_9) {
            handleZoomFit();
            event->accept(); return;
        }
        if (event->key() == Qt::Key_D && selectedIndex_ >= 0) {
            handleDuplicateKey();
            event->accept(); return;
        }
        if (event->key() == Qt::Key_A && !annotations_.isEmpty()) {
            selectedIndex_ = annotations_.size() - 1;
            markModified();
            event->accept(); return;
        }
        if (event->modifiers().testFlag(Qt::ShiftModifier)
            && (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
            && selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
            int dir = (event->key() == Qt::Key_Up) ? 1 : -1;
            handleLayerReorderKey(dir);
            event->accept(); return;
        }
        QWidget::keyPressEvent(event);
        return;
    }
    switch (event->key()) {
    case Qt::Key_R: setTool(AnnotationTool::Rectangle); event->accept(); return;
    case Qt::Key_E: setTool(AnnotationTool::Ellipse); event->accept(); return;
    case Qt::Key_A: setTool(AnnotationTool::Arrow); event->accept(); return;
    case Qt::Key_L: setTool(AnnotationTool::Line); event->accept(); return;
    case Qt::Key_P: setTool(AnnotationTool::Pen); event->accept(); return;
    case Qt::Key_T: setTool(AnnotationTool::Text); event->accept(); return;
    case Qt::Key_H: setTool(AnnotationTool::Highlight); event->accept(); return;
    case Qt::Key_N: setTool(AnnotationTool::Numbered); event->accept(); return;
    case Qt::Key_M: setTool(AnnotationTool::Mosaic); event->accept(); return;
    case Qt::Key_V: setTool(AnnotationTool::Select); event->accept(); return;
    case Qt::Key_X: setTool(AnnotationTool::Eraser); event->accept(); return;
    case Qt::Key_C: setTool(AnnotationTool::Crop); event->accept(); return;
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Left:
    case Qt::Key_Right:
        if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
            handleNudgeKey(event->key());
            event->accept(); return;
        }
        break;
    case Qt::Key_BracketLeft:
        handleFontSizeChange(-2);
        event->accept(); return;
    case Qt::Key_BracketRight:
        handleFontSizeChange(2);
        event->accept(); return;
    case Qt::Key_F1:
    case Qt::Key_Slash:
        if (event->modifiers().testFlag(Qt::ShiftModifier) || event->key() == Qt::Key_F1) {
            QMessageBox::information(static_cast<QWidget*>(window()), tr("Keyboard Shortcuts"),
                tr("<b>Tools</b><br>"
                "R - Rectangle<br>E - Ellipse<br>A - Arrow<br>L - Line<br>P - Pen<br>"
                "T - Text<br>H - Highlight<br>N - Numbered<br>M - Mosaic<br>"
                "V - Select<br>X - Eraser<br>C - Crop<br><br>"
                "<b>Edit</b><br>"
                    "Ctrl+Z - Undo<br>Ctrl+Y - Redo<br>Ctrl+D - Duplicate annotation<br>"
                    "Ctrl+A - Select last annotation<br>"
                    "Delete/Backspace - Remove annotation<br>"
                    "Arrow keys - Nudge annotation (Shift+Arrow = 10px)<br>"
                    "Ctrl+Shift+Arrow Up/Down - Bring forward / Send backward<br>"
                    "Double-click - Delete annotation (Text tool: edit text)<br>"
                    "[ / ] - Decrease/Increase text font size<br><br>"
                "<b>Zoom</b><br>"
                "Ctrl+Scroll / Ctrl++ / Ctrl+- - Zoom<br>"
                "Ctrl+0 - 100%<br>Ctrl+9 - Fit to window<br><br>"
                "<b>File</b><br>"
                "Ctrl+C - Copy image<br>Ctrl+S - Save<br>"
                "Ctrl+Shift+S - Export...<br>"
                "F3 - Pin image<br>"
                "Escape - Close editor"));
            event->accept(); return;
        }
        break;
    default: break;
    }
    QWidget::keyPressEvent(event);
}

void AnnotationCanvas::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu;
    auto* copyImage = menu.addAction(tr("Copy Image\tCtrl+C"));
    auto* saveAs = menu.addAction(tr("Export..."));
    QAction* deleteAnn = nullptr;
    QAction* duplicateAnn = nullptr;
    QAction* bringForward = nullptr;
    QAction* sendBackward = nullptr;
    if (selectedIndex_ >= 0) {
        menu.addSeparator();
        deleteAnn = menu.addAction(tr("Delete Annotation\tDel"));
        duplicateAnn = menu.addAction(tr("Duplicate Annotation"));
        if (selectedIndex_ < annotations_.size() - 1)
            bringForward = menu.addAction(tr("Bring Forward\tCtrl+Shift+Up"));
        if (selectedIndex_ > 0)
            sendBackward = menu.addAction(tr("Send Backward\tCtrl+Shift+Down"));
    }
    menu.addSeparator();
    auto* zoomIn = menu.addAction(tr("Zoom In\tCtrl++"));
    auto* zoomOut = menu.addAction(tr("Zoom Out\tCtrl+-"));
    auto* zoom100 = menu.addAction(tr("Actual Size (100%)\tCtrl+0"));
    auto* zoomFit = menu.addAction(tr("Fit to Window\tCtrl+9"));
    menu.addSeparator();
    auto* clearAll = menu.addAction(tr("Clear All Annotations"));
    auto* action = menu.exec(event->globalPos());
    if (action == deleteAnn && selectedIndex_ >= 0) {
        pushUndo();
        redoStack_.clear();
        annotations_.removeAt(selectedIndex_);
        selectedIndex_ = -1;
        markModified();
    } else if (action == duplicateAnn && selectedIndex_ >= 0) {
        pushUndo();
        redoStack_.clear();
        auto dup = annotations_.at(selectedIndex_);
        dup.bounds.translate(10, 10);
        annotations_.push_back(std::move(dup));
        selectedIndex_ = annotations_.size() - 1;
        markModified();
    } else if (action == bringForward && selectedIndex_ >= 0 && selectedIndex_ < annotations_.size() - 1) {
        pushUndo();
        redoStack_.clear();
        qSwap(annotations_[selectedIndex_], annotations_[selectedIndex_ + 1]);
        selectedIndex_++;
        markModified();
    } else if (action == sendBackward && selectedIndex_ > 0 && selectedIndex_ < annotations_.size()) {
        pushUndo();
        redoStack_.clear();
        qSwap(annotations_[selectedIndex_], annotations_[selectedIndex_ - 1]);
        selectedIndex_--;
        markModified();
    } else if (action == copyImage) {
        QApplication::clipboard()->setImage(renderedImage());
    } else if (action == saveAs) {
        auto path = QFileDialog::getSaveFileName(this, tr("Save As"), QString(),
            tr("PNG (*.png);;JPEG (*.jpg *.jpeg)"));
        if (!path.isEmpty()) {
            renderedImage().save(path);
        }
    } else if (action == zoomIn) {
        zoomAt(zoomFactor_ * 1.15, QPoint(width() / 2, height() / 2));
    } else if (action == zoomOut) {
        zoomAt(zoomFactor_ / 1.15, QPoint(width() / 2, height() / 2));
    } else if (action == zoom100) {
        zoomAt(1.0, QPoint(width() / 2, height() / 2));
    } else if (action == zoomFit) {
        auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
        if (scrollArea && !image_.isNull()) {
            auto vp = scrollArea->viewport()->size();
            auto logicalSize = image_.size() / image_.devicePixelRatio();
            double fit = qMin(static_cast<double>(vp.width()) / logicalSize.width(),
                               static_cast<double>(vp.height()) / logicalSize.height());
            int newW = static_cast<int>(logicalSize.width() * fit);
            int newH = static_cast<int>(logicalSize.height() * fit);
            if (newW > 0 && newH > 0) {
                QSize newSize(newW, newH);
                setMinimumSize(newSize);
                resize(newSize);
                zoomFactor_ = fit;
    updateWindowTitle();
    update();
    if (onZoomChanged_) onZoomChanged_(zoomFactor_);
}
        }
    } else if (action == clearAll) {
        if (!annotations_.isEmpty()) {
            auto ret = QMessageBox::question(this, tr("Clear All Annotations"),
                tr("Are you sure you want to clear all annotations?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                pushUndo();
                redoStack_.clear();
                annotations_.clear();
                selectedIndex_ = -1;
                markModified();
            }
        }
    }
}

void AnnotationCanvas::drawCheckerboard(QPainter& painter)
{
    int tile = 8;
    auto dpr = image_.devicePixelRatio();
    auto logicalH = image_.height() / dpr;
    auto logicalW = image_.width() / dpr;
    for (int y = 0; y < logicalH; y += tile) {
        for (int x = 0; x < logicalW; x += tile) {
            bool light = ((x / tile) + (y / tile)) % 2 == 0;
            painter.fillRect(x, y, tile, tile, light ? QColor("#cccccc") : QColor("#888888"));
        }
    }
}

void AnnotationCanvas::drawGridOverlay(QPainter& painter)
{
    auto dpr = image_.devicePixelRatio();
    auto w = static_cast<int>(image_.width() / dpr * zoomFactor_);
    auto h = static_cast<int>(image_.height() / dpr * zoomFactor_);
    int step = static_cast<int>(50 * zoomFactor_);
    if (step < 8) step = 8;
    painter.save();
    painter.setClipRect(0, 0, w, h);
    painter.setPen(QPen(QColor(255, 255, 255, 22), 1));
    for (int x = step; x < w; x += step)
        painter.drawLine(x, 0, x, h);
    for (int y = step; y < h; y += step)
        painter.drawLine(0, y, w, y);
    painter.restore();
}

void AnnotationCanvas::drawTextEditCursor(QPainter& painter)
{
    painter.save();
    painter.scale(zoomFactor_, zoomFactor_);
    if (editingTextIndex_ >= 0 && editingTextIndex_ < annotations_.size()) {
        const auto& a = annotations_[editingTextIndex_];
        if (a.tool == AnnotationTool::Text) {
            QFont font("Microsoft YaHei UI", a.textFontSize > 0 ? a.textFontSize : fontSize_);
            painter.setFont(font);
            int textWidth = painter.fontMetrics().horizontalAdvance(a.text);
            int cx = a.bounds.left() + 4 + textWidth;
            int cy = a.bounds.top() + 4;
            int ch = painter.fontMetrics().height();
            if (!preeditString_.isEmpty()) {
                painter.setPen(QPen(a.color, 1));
                painter.drawText(cx, cy, painter.fontMetrics().horizontalAdvance(preeditString_) + 4, ch,
                    Qt::AlignLeft | Qt::AlignTop, preeditString_);
                int preeditWidth = painter.fontMetrics().horizontalAdvance(preeditString_);
                painter.setPen(QPen(a.color, 1, Qt::DashLine));
                painter.drawLine(cx, cy + ch + 1, cx + preeditWidth, cy + ch + 1);
                textWidth += preeditWidth;
                cx += preeditWidth;
            }
            painter.setPen(QPen(a.color, 1.5));
            painter.drawLine(cx, cy, cx, cy + ch);
        }
    }
    painter.restore();
}

void AnnotationCanvas::drawDraftSizeLabel(QPainter& painter)
{
    if (!drawing_ || draft_.tool == AnnotationTool::Pen || draft_.tool == AnnotationTool::Numbered || draft_.tool == AnnotationTool::Crop) {
        return;
    }
    auto dims = draft_.bounds.size();
    QString label = tr("%1 × %2").arg(dims.width()).arg(dims.height());
    painter.setPen(Qt::NoPen);
    auto textRect = painter.fontMetrics().boundingRect(label);
    auto labelPos = current_;
    labelPos = QPoint(static_cast<int>(labelPos.x() * zoomFactor_),
                      static_cast<int>(labelPos.y() * zoomFactor_));
    labelPos += QPoint(12, -textRect.height() - 8);
    textRect = QRect(labelPos.x() - 4, labelPos.y() - 2,
                     textRect.width() + 8, textRect.height() + 4);
    painter.setBrush(QColor(0, 0, 0, 160));
    painter.drawRoundedRect(textRect, 3, 3);
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, label);
}

void AnnotationCanvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), QColor("#101418"));
    if (!image_.isNull()) {
        painter.save();
        painter.scale(zoomFactor_, zoomFactor_);
        if (image_.hasAlphaChannel()) {
            drawCheckerboard(painter);
        }
        painter.drawImage(QPoint(0, 0), image_);
        drawAnnotations(&painter, image_, true);
        if (drawing_) {
            drawAnnotation(&painter, image_, draft_, fontSize_);
        }
        painter.restore();
        if (gridEnabled_) {
            drawGridOverlay(painter);
        }
        drawTextEditCursor(painter);
    }
    drawDraftSizeLabel(painter);
}

void AnnotationCanvas::wheelEvent(QWheelEvent* event)
{
    if (image_.isNull() || !event->modifiers().testFlag(Qt::ControlModifier)) {
        QWidget::wheelEvent(event);
        return;
    }
    auto delta = event->angleDelta().y();
    if (delta == 0) return;
    zoomAt(zoomFactor_ * (delta > 0 ? 1.15 : 0.85), event->pos());
    event->accept();
}

void AnnotationCanvas::inputMethodEvent(QInputMethodEvent* event)
{
    if (editingTextIndex_ < 0 || editingTextIndex_ >= annotations_.size()
        || annotations_[editingTextIndex_].tool != AnnotationTool::Text) {
        QWidget::inputMethodEvent(event);
        return;
    }
    auto& a = annotations_[editingTextIndex_];
    if (!event->commitString().isEmpty()) {
        a.text += event->commitString();
        preeditString_.clear();
        updateTextBounds(editingTextIndex_);
        markModified();
    }
    preeditString_ = event->preeditString();
    update();
    event->accept();
}

QVariant AnnotationCanvas::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (editingTextIndex_ >= 0 && editingTextIndex_ < annotations_.size()
        && annotations_[editingTextIndex_].tool == AnnotationTool::Text) {
        const auto& a = annotations_[editingTextIndex_];
        const auto& img = image_;
        switch (query) {
        case Qt::ImCursorRectangle: {
            QFont font("Microsoft YaHei UI", a.textFontSize > 0 ? a.textFontSize : fontSize_);
            QFontMetrics fm(font);
            int textWidth = fm.horizontalAdvance(a.text + preeditString_);
            int cx = static_cast<int>((a.bounds.left() + 4 + textWidth) * zoomFactor_);
            int cy = static_cast<int>((a.bounds.top() + 4) * zoomFactor_);
            int ch = static_cast<int>(fm.height() * zoomFactor_);
            return QRect(mapToGlobal(QPoint(0, 0)) + QPoint(cx, cy), QSize(4, ch));
        }
        case Qt::ImEnabled:
            return true;
        case Qt::ImFont:
            return QFont("Microsoft YaHei UI", a.textFontSize > 0 ? a.textFontSize : fontSize_);
        case Qt::ImCursorPosition:
            return a.text.length() + preeditString_.length();
        case Qt::ImSurroundingText:
            return a.text + preeditString_;
        case Qt::ImCurrentSelection:
            return QString();
        case Qt::ImAnchorPosition:
            return a.text.length() + preeditString_.length();
        default:
            break;
        }
    }
    return QWidget::inputMethodQuery(query);
}

void AnnotationCanvas::drawAnnotations(QPainter* painter, const QImage& sourceImage, bool includeSelectionChrome) const
{
    for (int i = 0; i < annotations_.size(); ++i) {
        drawAnnotation(painter, sourceImage, annotations_.at(i), fontSize_);
        if (includeSelectionChrome && i == selectedIndex_) {
            painter->setPen(QPen(QColor("#2fbf9f"), 1, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(annotations_.at(i).bounds.adjusted(-3, -3, 3, 3));
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor("#2fbf9f"));
            const auto r = annotations_.at(i).bounds;
            const QPoint corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
            for (const auto& c : corners) {
                painter->drawRect(QRect(c.x() - 4, c.y() - 4, 8, 8));
            }
        }
    }
}

bool AnnotationCanvas::hitTestAnnotation(const Annotation& annotation, const QPoint& pos)
{
    constexpr int kMargin = 6;
    if (annotation.tool == AnnotationTool::Pen) {
        for (const auto& pt : annotation.points) {
            if (QRect(pt.x() - kMargin, pt.y() - kMargin, kMargin * 2, kMargin * 2).contains(pos)) {
                return true;
            }
        }
        return false;
    }
    return annotation.bounds.adjusted(-kMargin, -kMargin, kMargin, kMargin).contains(pos);
}

void AnnotationCanvas::drawRectAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(annotation.filled ? annotation.color : Qt::NoBrush);
    if (annotation.cornerRadius > 0)
        painter->drawRoundedRect(annotation.bounds, annotation.cornerRadius, annotation.cornerRadius);
    else
        painter->drawRect(annotation.bounds);
}

void AnnotationCanvas::drawEllipseAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(annotation.filled ? annotation.color : Qt::NoBrush);
    painter->drawEllipse(annotation.bounds);
}

void AnnotationCanvas::drawArrowAnnotation(QPainter* painter, const Annotation& annotation)
{
    const auto from = annotation.points.size() >= 2 ? annotation.points.first() : annotation.bounds.topLeft();
    const auto to = annotation.points.size() >= 2 ? annotation.points.last() : annotation.bounds.bottomRight();
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(from, to);
    constexpr double kArrowSize = 12.0;
    const auto angle = std::atan2(to.y() - from.y(), to.x() - from.x());
    painter->setBrush(annotation.color);
    painter->setPen(Qt::NoPen);
    if (annotation.arrowStyle == ArrowStyle::CircleArrow) {
        auto cx = to.x() - kArrowSize * 0.5 * std::cos(angle);
        auto cy = to.y() - kArrowSize * 0.5 * std::sin(angle);
        painter->drawEllipse(QPointF(cx, cy), kArrowSize * 0.5, kArrowSize * 0.5);
    } else if (annotation.arrowStyle == ArrowStyle::SquareArrow) {
        auto cx = to.x() - kArrowSize * 0.5 * std::cos(angle);
        auto cy = to.y() - kArrowSize * 0.5 * std::sin(angle);
        painter->drawRect(QRectF(cx - kArrowSize * 0.4, cy - kArrowSize * 0.4,
                                 kArrowSize * 0.8, kArrowSize * 0.8));
    } else {
        const auto p1 = QPointF(to.x() - kArrowSize * std::cos(angle - M_PI / 6),
                                 to.y() - kArrowSize * std::sin(angle - M_PI / 6));
        const auto p2 = QPointF(to.x() - kArrowSize * std::cos(angle + M_PI / 6),
                                 to.y() - kArrowSize * std::sin(angle + M_PI / 6));
        QPolygonF arrowHead;
        arrowHead << to << p1 << p2;
        painter->drawPolygon(arrowHead);
    }
}

void AnnotationCanvas::drawLineAnnotation(QPainter* painter, const Annotation& annotation)
{
    const auto from = annotation.points.size() >= 2 ? annotation.points.first() : annotation.bounds.topLeft();
    const auto to = annotation.points.size() >= 2 ? annotation.points.last() : annotation.bounds.bottomRight();
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(from, to);
}

void AnnotationCanvas::drawPenAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for (int i = 1; i < annotation.points.size(); ++i) {
        painter->drawLine(annotation.points.at(i - 1), annotation.points.at(i));
    }
}

void AnnotationCanvas::drawTextAnnotation(QPainter* painter, const Annotation& annotation, int fontSize)
{
    QFont font("Microsoft YaHei UI", annotation.textFontSize > 0 ? annotation.textFontSize : fontSize);
    painter->setFont(font);
    const auto flags = Qt::AlignLeft | Qt::AlignTop;
    if (annotation.textOutline) {
        painter->setPen(QColor(255, 255, 255, 220));
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                painter->drawText(annotation.bounds.adjusted(dx, dy, dx, dy), flags, annotation.text);
            }
        }
    }
    painter->setPen(QPen(annotation.color, 1));
    painter->drawText(annotation.bounds, flags, annotation.text);
}

void AnnotationCanvas::drawMosaicAnnotation(QPainter* painter, const QImage& sourceImage, const Annotation& annotation)
{
    if (!annotation.points.isEmpty()) {
        const int blockSize = qMax(4, annotation.strokeWidth * 4);
        for (const auto& pt : annotation.points) {
            QRect blockRect(pt.x() - blockSize / 2, pt.y() - blockSize / 2, blockSize, blockSize);
            const auto clipped = blockRect.intersected(sourceImage.rect());
            if (clipped.isEmpty()) continue;
            if (annotation.blurRadius > 0) {
                auto region = sourceImage.copy(clipped);
                painter->drawImage(clipped.topLeft(), blurImage(region, annotation.blurRadius));
            } else {
                constexpr int kBlock = 8;
                const int bw = qMax(1, clipped.width() / kBlock);
                const int bh = qMax(1, clipped.height() / kBlock);
                auto region = sourceImage.copy(clipped);
                auto pixelated = region.scaled(bw, bh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                      .scaled(clipped.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);
                painter->drawImage(clipped.topLeft(), pixelated);
            }
        }
    } else {
        const auto clipped = annotation.bounds.intersected(sourceImage.rect());
        if (clipped.isEmpty()) return;
        if (annotation.blurRadius > 0) {
            auto region = sourceImage.copy(clipped);
            painter->drawImage(clipped.topLeft(), blurImage(region, annotation.blurRadius));
        } else {
            constexpr int kBlockSize = 8;
            const int bw = qMax(1, clipped.width() / kBlockSize);
            const int bh = qMax(1, clipped.height() / kBlockSize);
            auto region = sourceImage.copy(clipped);
            auto pixelated = region.scaled(bw, bh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                  .scaled(clipped.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);
            painter->drawImage(clipped.topLeft(), pixelated);
        }
    }
}

void AnnotationCanvas::drawHighlightAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->fillRect(annotation.bounds, QColor(annotation.color.red(), annotation.color.green(), annotation.color.blue(), 100));
}

void AnnotationCanvas::drawNumberedAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    const auto center = annotation.bounds.center();
    const auto r = kDefaultNumberedSize / 2;
    painter->setPen(QPen(annotation.color, 2));
    painter->setBrush(annotation.color);
    painter->drawEllipse(center, r, r);
    painter->setPen(Qt::white);
    painter->setFont(QFont("Segoe UI", r, QFont::Bold));
    painter->drawText(QRect(center.x() - r, center.y() - r, kDefaultNumberedSize, kDefaultNumberedSize),
                      Qt::AlignCenter, QString::number(annotation.number));
}

void AnnotationCanvas::drawAnnotation(QPainter* painter, const QImage& sourceImage, const Annotation& annotation, int fontSize)
{
    painter->setRenderHint(QPainter::Antialiasing, true);

    switch (annotation.tool) {
    case AnnotationTool::Rectangle:
        drawRectAnnotation(painter, annotation);
        break;
    case AnnotationTool::Ellipse:
        drawEllipseAnnotation(painter, annotation);
        break;
    case AnnotationTool::Arrow:
        drawArrowAnnotation(painter, annotation);
        break;
    case AnnotationTool::Line:
        drawLineAnnotation(painter, annotation);
        break;
    case AnnotationTool::Pen:
        drawPenAnnotation(painter, annotation);
        break;
    case AnnotationTool::Text:
        drawTextAnnotation(painter, annotation, fontSize);
        break;
    case AnnotationTool::Mosaic:
        drawMosaicAnnotation(painter, sourceImage, annotation);
        break;
    case AnnotationTool::Highlight:
        drawHighlightAnnotation(painter, annotation);
        break;
    case AnnotationTool::Numbered:
        drawNumberedAnnotation(painter, annotation);
        break;
    case AnnotationTool::Select:
    case AnnotationTool::Eraser:
        break;
    }
}

} // namespace snappaste
