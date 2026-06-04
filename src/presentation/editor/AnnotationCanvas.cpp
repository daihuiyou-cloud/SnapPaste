#include "presentation/editor/AnnotationCanvas.h"

#include <QApplication>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollArea>
#include <QSettings>
#include <QWheelEvent>

#include <algorithm>

namespace snappaste {

namespace {
    const QColor kCanvasBg("#1f2329");
    const QColor kAccentColor("#2fbf9f");
    const QColor kWhite("#ffffff");
}

AnnotationCanvas::AnnotationCanvas(QWidget* parent)
    : QWidget(parent)
    , toolManager_()
    , eventHandler_(*this, toolManager_, renderer_)
{
    setMouseTracking(true);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setMinimumSize(640, 360);

    wireCallbacks();
}

void AnnotationCanvas::wireCallbacks()
{
    toolManager_.onModified = [this] {
        markModified();
    };

    toolManager_.onSelectionChanged = [this] {
        emit selectionChanged();
    };

    toolManager_.onToolChanged = [this](AnnotationTool tool) {
        emit toolChanged(tool);
    };

    toolManager_.onFontSizeChanged = [this](int size) {
        emit fontSizeChanged(size);
    };

    toolManager_.onPickingColorChanged = [this](bool picking) {
        emit pickingColorChanged(picking);
    };

    toolManager_.onZoomChanged = [this](double factor) {
        emit zoomChanged(factor);
    };

    toolManager_.onStrokeAlphaChanged = [this](int alpha) {
        emit strokeAlphaChanged(alpha);
    };

    toolManager_.onArrowStyleChanged = [this](int style) {
        emit arrowStyleChanged(style);
    };

    toolManager_.onCornerRadiusChanged = [this](int radius) {
        emit cornerRadiusChanged(radius);
    };

    toolManager_.onTextPropertiesChanged = [this] {
        emit textPropertiesChanged();
    };

    toolManager_.onCropAspectRatioChanged = [this](double ratio) {
        emit cropAspectRatioChanged(ratio);
    };

    toolManager_.onImageEdited = [this] {
        emit imageEdited(renderedImage());
    };

    toolManager_.onUpdateRequired = [this] {
        update();
    };

    toolManager_.onSetCursor = [this](QCursor cursor) {
        setCursor(cursor);
    };

    toolManager_.onWindowTitleUpdate = [this] {
        updateWindowTitle();
    };

    toolManager_.onResizeCanvas = [this](const QSize& size) {
        setMinimumSize(size);
        resize(size);
    };

    toolManager_.onScrollBy = [this](int dx, int dy) {
        auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
        if (scrollArea) {
            scrollArea->horizontalScrollBar()->setValue(
                scrollArea->horizontalScrollBar()->value() + dx);
            scrollArea->verticalScrollBar()->setValue(
                scrollArea->verticalScrollBar()->value() + dy);
        }
    };

    toolManager_.onImageHistoryRestored = [this] {
        image_ = toolManager_.image();
        baseImage_ = toolManager_.baseImage();
        brightness_ = toolManager_.brightness();
        contrast_ = toolManager_.contrast();
        rebuildBackingCache();
        update();
    };

    // Wire EventHandler default callbacks
    eventHandler_.onKeyPressDefault = [this](QKeyEvent* event) {
        QWidget::keyPressEvent(event);
    };
    eventHandler_.onWheelDefault = [this](QWheelEvent* event) {
        QWidget::wheelEvent(event);
    };
    eventHandler_.onInputMethodDefault = [this](QInputMethodEvent* event) {
        QWidget::inputMethodEvent(event);
    };
    eventHandler_.onInputMethodQueryDefault = [this](Qt::InputMethodQuery query) -> QVariant {
        return QWidget::inputMethodQuery(query);
    };
}

QPoint AnnotationCanvas::toImage(QPoint widgetPt) const
{
    return QPoint(static_cast<int>(widgetPt.x() / toolManager_.zoomFactor()),
                  static_cast<int>(widgetPt.y() / toolManager_.zoomFactor()));
}

void AnnotationCanvas::setImage(QImage image)
{
    if (image.format() != QImage::Format_ARGB32_Premultiplied)
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    baseImage_ = image;
    brightness_ = 0;
    contrast_ = 0;
    image_ = std::move(image);
    modified_ = false;
    renderer_.invalidateCache();
    backingCacheDirty_ = true;

    auto dpr = image_.devicePixelRatio();
    toolManager_.setImage(image_, 1.0);

    QSize logicalSize(image_.size() / dpr);
    setMinimumSize(logicalSize);

    // Auto-fit to viewport if image is larger than the scroll area;
    // otherwise restore the last-used zoom from settings.
    auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
    if (scrollArea && !image_.isNull()) {
        auto vp = scrollArea->viewport()->size();
        if (vp.width() > 0 && vp.height() > 0) {
            double fit = qMin(static_cast<double>(vp.width()) / logicalSize.width(),
                               static_cast<double>(vp.height()) / logicalSize.height());
            if (fit < 1.0) {
                toolManager_.setZoomFactor(fit);
            } else {
                double saved = QSettings().value("editor/zoomFactor", 1.0).toDouble();
                if (saved >= 0.1 && saved <= 5.0)
                    toolManager_.setZoomFactor(saved);
            }
        }
    }
    double zf = toolManager_.zoomFactor();
    QSize zoomedSize(static_cast<int>(logicalSize.width() * zf),
                     static_cast<int>(logicalSize.height() * zf));
    resize(zoomedSize);
    updateWindowTitle();
    update();
    emit zoomChanged(zf);
}

void AnnotationCanvas::rebuildBackingCache()
{
    if (image_.isNull()) return;
    QSize cacheSize = size();
    if (cacheSize.isEmpty()) cacheSize = QSize(640, 360);

    if (backingCache_.size() != cacheSize) {
        backingCache_ = QPixmap(cacheSize);
    }
    backingCache_.fill(kCanvasBg);

    QPainter p(&backingCache_);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    if (!image_.isNull()) {
        double zf = toolManager_.zoomFactor();
        p.save();
        p.scale(zf, zf);
        if (image_.hasAlphaChannel())
            renderer_.drawCheckerboard(p, image_);
        p.drawImage(QPoint(0, 0), image_);
        renderer_.drawAnnotations(p, image_, toolManager_.annotations(), toolManager_.fontSize());
        p.restore();
    }
    p.end();
    backingZoom_ = toolManager_.zoomFactor();
    backingCacheDirty_ = false;
}

void AnnotationCanvas::reapplyAdjustments()
{
    image_ = baseImage_.copy();
    if (brightness_ != 0 || contrast_ != 0) {
        double contrastFactor = (contrast_ + 100.0) / 100.0;

        quint8 lut[256];
        for (int i = 0; i < 256; ++i) {
            int v = static_cast<int>((i - 128) * contrastFactor + 128 + brightness_);
            lut[i] = static_cast<quint8>(qBound(0, v, 255));
        }

        int w = image_.width(), h = image_.height();
        for (int y = 0; y < h; ++y) {
            auto* line = reinterpret_cast<QRgb*>(image_.scanLine(y));
            for (int x = 0; x < w; ++x) {
                auto px = line[x];
                line[x] = qRgba(lut[qRed(px)], lut[qGreen(px)], lut[qBlue(px)], qAlpha(px));
            }
        }
    }
    toolManager_.setImageDirect(image_);
    auto logicalSize = image_.size() / image_.devicePixelRatio();
    setMinimumSize(logicalSize);
    resize(logicalSize);

    // Annotation cache (non-mosaic) is independent of image pixels; keep it valid
    const auto& annotations = toolManager_.annotations();
    bool hasMosaic = std::any_of(annotations.begin(), annotations.end(),
        [](const Annotation& a) { return a.tool == AnnotationTool::Mosaic; });
    if (hasMosaic) {
        renderer_.invalidateCache();
    }

    modified_ = true;
    backingCacheDirty_ = true;
    emit modified();
    updateWindowTitle();
    update();
}

void AnnotationCanvas::beginImageAdjust()
{
    if (baseImage_.isNull()) return;
    toolManager_.syncImageState(image_, baseImage_, brightness_, contrast_);
    toolManager_.pushUndo();
}

void AnnotationCanvas::previewAdjustImage(int brightness, int contrast)
{
    if (baseImage_.isNull()) return;
    brightness = qBound(-100, brightness, 100);
    contrast = qBound(-100, contrast, 100);
    if (brightness == brightness_ && contrast == contrast_) return;
    brightness_ = brightness;
    contrast_ = contrast;
    reapplyAdjustments();
}

void AnnotationCanvas::adjustImage(int brightness, int contrast)
{
    beginImageAdjust();
    previewAdjustImage(brightness, contrast);
}

void AnnotationCanvas::rotateImage(int degrees)
{
    if (baseImage_.isNull()) return;
    degrees = ((degrees % 360) + 360) % 360;
    if (degrees == 0) return;
    QTransform transform;
    if (degrees == 90)       transform.rotate(90);
    else if (degrees == 180) transform.rotate(180);
    else if (degrees == 270) transform.rotate(270);
    else return;
    toolManager_.pushUndo();
    baseImage_ = baseImage_.transformed(transform, Qt::SmoothTransformation);
    toolManager_.clearAnnotations();
    reapplyAdjustments();
}

void AnnotationCanvas::flipImage(bool horizontal, bool vertical)
{
    if (baseImage_.isNull()) return;
    if (!horizontal && !vertical) return;
    if (horizontal && vertical) {
        baseImage_ = baseImage_.mirrored(true, true);
    } else if (horizontal) {
        baseImage_ = baseImage_.mirrored(true, false);
    } else {
        baseImage_ = baseImage_.mirrored(false, true);
    }
    toolManager_.pushUndo();
    toolManager_.clearAnnotations();
    reapplyAdjustments();
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

    toolManager_.pushUndo();

    // Keep annotations inside crop rect, discard those outside
    {
        QPoint offset = -physicalCrop.topLeft();
        auto& annotations = toolManager_.annotationsMut();
        QVector<Annotation> kept;
        kept.reserve(annotations.size());
        int newSelected = -1;
        int maxNumber = 0;
        int sel = toolManager_.selectedIndex();
        for (int i = 0; i < annotations.size(); ++i) {
            auto& a = annotations[i];
            if (!physicalCrop.intersects(a.bounds)) continue;
            a.bounds.translate(offset);
            for (auto& pt : a.points) pt += offset;
            if (i == sel) newSelected = kept.size();
            if (a.number > maxNumber) maxNumber = a.number;
            kept.push_back(std::move(a));
        }
        annotations = std::move(kept);
        toolManager_.setSelectedIndex(newSelected);
        toolManager_.setNextNumber(maxNumber + 1);
    }

    image_ = image_.copy(physicalCrop);
    toolManager_.setImageDirect(image_);
    renderer_.invalidateCache();
    markModified();

    auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
    if (scrollArea) {
        auto vp = scrollArea->viewport()->size();
        auto logicalSize = image_.size() / image_.devicePixelRatio();
        double fit = qMin(static_cast<double>(vp.width()) / logicalSize.width(),
                           static_cast<double>(vp.height()) / logicalSize.height());
        if (fit > 1.0) fit = 1.0;
        toolManager_.setZoomFactor(fit);
    }

    baseImage_ = image_;
    brightness_ = 0;
    contrast_ = 0;
    auto logicalSize = image_.size() / image_.devicePixelRatio();
    double zf = toolManager_.zoomFactor();
    QSize zoomedSize(static_cast<int>(logicalSize.width() * zf),
                     static_cast<int>(logicalSize.height() * zf));
    setMinimumSize(zoomedSize);
    resize(zoomedSize);
    updateWindowTitle();
    update();
    emit imageEdited(image_);
    emit zoomChanged(zf);
}

void AnnotationCanvas::clearModified() { modified_ = false; updateWindowTitle(); }

bool AnnotationCanvas::isModified() const { return modified_; }

void AnnotationCanvas::markModified()
{
    modified_ = true;
    renderer_.invalidateCache();
    backingCacheDirty_ = true;
    emit modified();
    emit annotationsChanged();
    updateWindowTitle();
    update();
}

void AnnotationCanvas::zoomAt(double factor, QPoint center)
{
    const auto oldCenter = toImage(center);
    double newZoom = std::max(0.1, std::min(5.0, factor));
    toolManager_.setZoomFactor(newZoom);
    renderer_.invalidateCache();
    auto logicalSize = image_.size() / image_.devicePixelRatio();
    QSize newSize(static_cast<int>(logicalSize.width() * newZoom),
                  static_cast<int>(logicalSize.height() * newZoom));
    setMinimumSize(newSize);
    resize(newSize);
    const auto newWidgetCenter = QPoint(static_cast<int>(oldCenter.x() * newZoom),
                                        static_cast<int>(oldCenter.y() * newZoom));
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
    emit zoomChanged(newZoom);
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
        title += tr(" - %1%").arg(static_cast<int>(toolManager_.zoomFactor() * 100));
        int annCount = toolManager_.annotationCount();
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
    return renderer_.renderToImage(image_, toolManager_.annotations(), toolManager_.fontSize());
}

// --- Delegated to ToolManager ---

void AnnotationCanvas::setTool(AnnotationTool tool) { toolManager_.setTool(tool); }
void AnnotationCanvas::setColor(const QColor& color) { toolManager_.setColor(color); }
void AnnotationCanvas::setStrokeWidth(int width) { toolManager_.setStrokeWidth(width); }
void AnnotationCanvas::setPickingColor(bool picking) { toolManager_.setPickingColor(picking); }
void AnnotationCanvas::setMosaicBlurred(bool blurred) { toolManager_.setMosaicBlurred(blurred); }
void AnnotationCanvas::setTextOutlineEnabled(bool enabled) { toolManager_.setTextOutlineEnabled(enabled); }
void AnnotationCanvas::setFilled(bool filled) { toolManager_.setFilled(filled); }
QColor AnnotationCanvas::fillColor() const { return toolManager_.fillColor(); }
void AnnotationCanvas::setFillColor(const QColor& color) { toolManager_.setFillColor(color); }
void AnnotationCanvas::setTextBackgroundEnabled(bool enabled) { toolManager_.setTextBackgroundEnabled(enabled); }
bool AnnotationCanvas::textBackgroundEnabled() const { return toolManager_.textBackgroundEnabled(); }
void AnnotationCanvas::setTextBackgroundColor(const QColor& color) { toolManager_.setTextBackgroundColor(color); }
QColor AnnotationCanvas::textBackgroundColor() const { return toolManager_.textBackgroundColor(); }
void AnnotationCanvas::updateTextBounds(int index) { toolManager_.updateTextBounds(index); }
int AnnotationCanvas::fontSize() const { return toolManager_.fontSize(); }
void AnnotationCanvas::setFontSize(int size, bool persist) { toolManager_.setFontSize(size, persist); }
int AnnotationCanvas::selectedIndex() const { return toolManager_.selectedIndex(); }
QString AnnotationCanvas::fontFamily() const { return toolManager_.fontFamily(); }
void AnnotationCanvas::setFontFamily(const QString& family) { toolManager_.setFontFamily(family); }
bool AnnotationCanvas::bold() const { return toolManager_.bold(); }
void AnnotationCanvas::setBold(bool b) { toolManager_.setBold(b); }
bool AnnotationCanvas::italic() const { return toolManager_.italic(); }
void AnnotationCanvas::setItalic(bool i) { toolManager_.setItalic(i); }
bool AnnotationCanvas::underline() const { return toolManager_.underline(); }
void AnnotationCanvas::setUnderline(bool u) { toolManager_.setUnderline(u); }
int AnnotationCanvas::textAlignment() const { return toolManager_.textAlignment(); }
void AnnotationCanvas::setTextAlignment(int align) { toolManager_.setTextAlignment(align); }
void AnnotationCanvas::syncTextPropertiesUI() { toolManager_.syncTextPropertiesUI(); }
double AnnotationCanvas::cropAspectRatio() const { return toolManager_.cropAspectRatio(); }
void AnnotationCanvas::setCropAspectRatio(double ratio) { toolManager_.setCropAspectRatio(ratio); }
double AnnotationCanvas::zoomFactor() const { return toolManager_.zoomFactor(); }
QSize AnnotationCanvas::imageSize() const { return image_.size(); }
QColor AnnotationCanvas::color() const { return toolManager_.color(); }
int AnnotationCanvas::strokeWidth() const { return toolManager_.strokeWidth(); }
const QVector<QColor>& AnnotationCanvas::recentColors() const { return toolManager_.recentColors(); }
void AnnotationCanvas::addRecentColor(const QColor& color) { toolManager_.addRecentColor(color); }
void AnnotationCanvas::undo() { toolManager_.undo(); }
void AnnotationCanvas::pushUndo() { toolManager_.pushUndo(); }
void AnnotationCanvas::redo() { toolManager_.redo(); }
int AnnotationCanvas::strokeAlpha() const { return toolManager_.strokeAlpha(); }
void AnnotationCanvas::setStrokeAlpha(int alpha) { toolManager_.setStrokeAlpha(alpha); }
ArrowStyle AnnotationCanvas::arrowStyle() const { return toolManager_.arrowStyle(); }
void AnnotationCanvas::setArrowStyle(ArrowStyle style) { toolManager_.setArrowStyle(style); }
int AnnotationCanvas::cornerRadius() const { return toolManager_.cornerRadius(); }
void AnnotationCanvas::setCornerRadius(int radius) { toolManager_.setCornerRadius(radius); }
bool AnnotationCanvas::gridEnabled() const { return toolManager_.gridEnabled(); }
void AnnotationCanvas::setGridEnabled(bool enabled) { toolManager_.setGridEnabled(enabled); }
int AnnotationCanvas::undoCount() const { return toolManager_.undoCount(); }
int AnnotationCanvas::redoCount() const { return toolManager_.redoCount(); }
void AnnotationCanvas::selectAnnotation(int index) { toolManager_.selectAnnotation(index); }
void AnnotationCanvas::deleteAnnotation(int index) { toolManager_.deleteAnnotation(index); }
void AnnotationCanvas::duplicateAnnotation(int index) { toolManager_.duplicateAnnotation(index); }
void AnnotationCanvas::swapAnnotations(int i, int j) { toolManager_.swapAnnotations(i, j); }
void AnnotationCanvas::setAnnotationVisible(int index, bool visible) { toolManager_.setAnnotationVisible(index, visible); }

void AnnotationCanvas::updateBrushCursor()
{
    int diameter;
    auto tool = toolManager_.currentTool();
    int sw = toolManager_.strokeWidth();
    switch (tool) {
    case AnnotationTool::Pen:     diameter = sw; break;
    case AnnotationTool::Mosaic:  diameter = sw * 4; break;
    case AnnotationTool::Eraser:  diameter = sw * 2; break;
    default: setCursor(Qt::CrossCursor); return;
    }
    int margin = 4;
    int size = qMax(diameter + margin * 2, 24);
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(255, 255, 255, 160), 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(size / 2.0, size / 2.0), diameter / 2.0, diameter / 2.0);
    p.setPen(QPen(QColor(0, 0, 0, 80), 1));
    p.drawEllipse(QPointF(size / 2.0, size / 2.0), diameter / 2.0 + 1, diameter / 2.0 + 1);
    if (tool == AnnotationTool::Pen || tool == AnnotationTool::Mosaic) {
        p.setPen(QPen(QColor(255, 255, 255, 120), 1));
        p.drawLine(QPointF(size / 2.0 - 6, size / 2.0), QPointF(size / 2.0 + 6, size / 2.0));
        p.drawLine(QPointF(size / 2.0, size / 2.0 - 6), QPointF(size / 2.0, size / 2.0 + 6));
    }
    p.end();
    setCursor(QCursor(pm, size / 2, size / 2));
}

void AnnotationCanvas::zoomFit()
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
            toolManager_.setZoomFactor(fit);
            updateWindowTitle();
            update();
            emit zoomChanged(fit);
        }
    }
}

// --- Event overrides delegated to EventHandler ---

void AnnotationCanvas::dragEnterEvent(QDragEnterEvent* event) { eventHandler_.dragEnterEvent(event); }
void AnnotationCanvas::dropEvent(QDropEvent* event) { eventHandler_.dropEvent(event); }
void AnnotationCanvas::mouseDoubleClickEvent(QMouseEvent* event) { eventHandler_.mouseDoubleClickEvent(event); }
void AnnotationCanvas::mousePressEvent(QMouseEvent* event) { eventHandler_.mousePressEvent(event); }
void AnnotationCanvas::mouseMoveEvent(QMouseEvent* event) { eventHandler_.mouseMoveEvent(event); }
void AnnotationCanvas::mouseReleaseEvent(QMouseEvent* event) { eventHandler_.mouseReleaseEvent(event); }
void AnnotationCanvas::keyPressEvent(QKeyEvent* event) { eventHandler_.keyPressEvent(event); }
void AnnotationCanvas::contextMenuEvent(QContextMenuEvent* event) { eventHandler_.contextMenuEvent(event); }
void AnnotationCanvas::wheelEvent(QWheelEvent* event) { eventHandler_.wheelEvent(event); }
void AnnotationCanvas::inputMethodEvent(QInputMethodEvent* event) { eventHandler_.inputMethodEvent(event); }
QVariant AnnotationCanvas::inputMethodQuery(Qt::InputMethodQuery query) const { return eventHandler_.inputMethodQuery(query); }

// --- Paint ---

void AnnotationCanvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), kCanvasBg);
    if (!image_.isNull()) {
        double zf = toolManager_.zoomFactor();
        // Use backing cache for static content
        if (backingCacheDirty_ || backingZoom_ != zf ||
            backingCache_.size() != size()) {
            rebuildBackingCache();
        }
        painter.drawPixmap(QPoint(0, 0), backingCache_);

        // Dynamic overlay: drawn fresh every frame
        painter.save();
        painter.scale(zf, zf);
        int sel = toolManager_.selectedIndex();
        if (sel >= 0 && sel < toolManager_.annotationCount()) {
            painter.setPen(QPen(kAccentColor, 1, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(toolManager_.annotationAt(sel).bounds.adjusted(-3, -3, 3, 3));
            painter.setPen(Qt::NoPen);
            painter.setBrush(kAccentColor);
            const auto r = toolManager_.annotationAt(sel).bounds;
            const QPoint corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
            for (const auto& c : corners) {
                painter.drawRect(QRect(c.x() - 4, c.y() - 4, 8, 8));
            }
            const QPoint midpoints[] = {
                QPoint(r.center().x(), r.top()), QPoint(r.right(), r.center().y()),
                QPoint(r.center().x(), r.bottom()), QPoint(r.left(), r.center().y())};
            painter.setBrush(kWhite);
            for (const auto& m : midpoints) {
                painter.drawRect(QRect(m.x() - 3, m.y() - 3, 6, 6));
            }
        }
        if (toolManager_.drawing()) {
            renderer_.drawDraft(painter, image_, toolManager_.draft(), toolManager_.fontSize());
        }
        painter.restore();
        if (toolManager_.gridEnabled()) {
            renderer_.drawGridOverlay(painter, rect(), zf);
        }
        renderer_.drawTextEditCursor(painter, toolManager_.annotations(),
            toolManager_.editingTextIndex(), toolManager_.cursorPos(),
            toolManager_.preeditString(), toolManager_.fontSize(), zf);
    }
    renderer_.drawDraftSizeLabel(painter, toolManager_.current(),
        toolManager_.draft(), toolManager_.drawing(), toolManager_.zoomFactor());
}

} // namespace snappaste
