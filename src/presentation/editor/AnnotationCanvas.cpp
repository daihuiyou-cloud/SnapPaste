#define _USE_MATH_DEFINES
#include "presentation/editor/AnnotationCanvas.h"

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
    currentFontFamily_ = settings.value("editor/fontFamily", QApplication::font().family()).toString();
    bold_ = settings.value("editor/bold", false).toBool();
    italic_ = settings.value("editor/italic", false).toBool();
    underline_ = settings.value("editor/underline", false).toBool();
    textAlignment_ = settings.value("editor/textAlignment", -1).toInt();
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
    if (image.format() != QImage::Format_ARGB32_Premultiplied)
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    baseImage_ = image;
    brightness_ = 0;
    contrast_ = 0;
    image_ = std::move(image);
    annotations_.clear();
    undoStack_.clear();
    redoStack_.clear();
    modified_ = false;
    renderer_.invalidateCache();
    nextNumber_ = 1;
    editingTextIndex_ = -1;
    preeditString_.clear();
    preCropImage_ = {};
    preAdjustImage_ = {};
    cropUndoOffset_ = {};
    auto dpr = image_.devicePixelRatio();
    QSize logicalSize(image_.size() / dpr);
    setMinimumSize(logicalSize);
    resize(logicalSize);
    updateWindowTitle();
    update();
}

void AnnotationCanvas::reapplyAdjustments()
{
    double contrastFactor = (contrast_ + 100.0) / 100.0;
    image_ = baseImage_.copy();
    int w = image_.width(), h = image_.height();
    for (int y = 0; y < h; ++y) {
        auto* line = reinterpret_cast<QRgb*>(image_.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = qBound(0, static_cast<int>((qRed(line[x]) - 128) * contrastFactor + 128 + brightness_), 255);
            int g = qBound(0, static_cast<int>((qGreen(line[x]) - 128) * contrastFactor + 128 + brightness_), 255);
            int b = qBound(0, static_cast<int>((qBlue(line[x]) - 128) * contrastFactor + 128 + brightness_), 255);
            line[x] = qRgba(r, g, b, qAlpha(line[x]));
        }
    }
    auto logicalSize = image_.size() / image_.devicePixelRatio();
    setMinimumSize(logicalSize);
    resize(logicalSize);
    updateWindowTitle();
    renderer_.invalidateCache();
    markModified();
    update();
}

void AnnotationCanvas::adjustImage(int brightness, int contrast)
{
    if (baseImage_.isNull()) return;
    brightness = qBound(-100, brightness, 100);
    contrast = qBound(-100, contrast, 100);
    if (brightness == brightness_ && contrast == contrast_) return;
    pushUndo();
    redoStack_.clear();
    preAdjustImage_ = image_;
    brightness_ = brightness;
    contrast_ = contrast;
    reapplyAdjustments();
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
    pushUndo();
    redoStack_.clear();
    preAdjustImage_ = {};
    baseImage_ = baseImage_.transformed(transform, Qt::SmoothTransformation);
    annotations_.clear();
    selectedIndex_ = -1;
    nextNumber_ = 1;
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
    pushUndo();
    redoStack_.clear();
    preAdjustImage_ = {};
    annotations_.clear();
    selectedIndex_ = -1;
    nextNumber_ = 1;
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

    preCropImage_ = image_.copy();
    preAdjustImage_ = {};
    pushUndo();
    redoStack_.clear();
    cropUndoOffset_ = -physicalCrop.topLeft();
    image_ = image_.copy(physicalCrop);
    renderer_.invalidateCache();
    annotations_.clear();
    annotations_.squeeze();
    selectedIndex_ = -1;
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

    baseImage_ = image_;
    brightness_ = 0;
    contrast_ = 0;
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
    renderer_.invalidateCache();
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
    return renderer_.renderToImage(image_, annotations_, fontSize_);
}

void AnnotationCanvas::updateBrushCursor()
{
    int diameter;
    switch (currentTool_) {
    case AnnotationTool::Pen:     diameter = currentStrokeWidth_; break;
    case AnnotationTool::Mosaic:  diameter = currentStrokeWidth_ * 4; break;
    case AnnotationTool::Eraser:  diameter = currentStrokeWidth_ * 2; break;
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
    if (currentTool_ == AnnotationTool::Pen || currentTool_ == AnnotationTool::Mosaic) {
        p.setPen(QPen(QColor(255, 255, 255, 120), 1));
        p.drawLine(QPointF(size / 2.0 - 6, size / 2.0), QPointF(size / 2.0 + 6, size / 2.0));
        p.drawLine(QPointF(size / 2.0, size / 2.0 - 6), QPointF(size / 2.0, size / 2.0 + 6));
    }
    p.end();
    setCursor(QCursor(pm, size / 2, size / 2));
}

void AnnotationCanvas::setTool(AnnotationTool tool)
{
    if (pickingColor_) {
        setPickingColor(false);
    }
    if (currentTool_ == tool) return;
    currentTool_ = tool;
    if (tool == AnnotationTool::Pen || tool == AnnotationTool::Eraser || tool == AnnotationTool::Mosaic) {
        updateBrushCursor();
    } else {
        switch (tool) {
        case AnnotationTool::Text: setCursor(Qt::IBeamCursor); break;
        default: setCursor(Qt::CrossCursor); break;
        }
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
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        annotations_[selectedIndex_].color = color;
        markModified();
    }
}

void AnnotationCanvas::setStrokeWidth(int width)
{
    currentStrokeWidth_ = std::clamp(width, 1, 12);
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        annotations_[selectedIndex_].strokeWidth = currentStrokeWidth_;
        markModified();
    }
    if (currentTool_ == AnnotationTool::Pen || currentTool_ == AnnotationTool::Eraser || currentTool_ == AnnotationTool::Mosaic) {
        updateBrushCursor();
    }
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
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
        && annotations_[selectedIndex_].tool == AnnotationTool::Mosaic) {
        annotations_[selectedIndex_].blurRadius = blurred ? currentStrokeWidth_ : 0;
        markModified();
    }
    update();
}

void AnnotationCanvas::setTextOutlineEnabled(bool enabled)
{
    textOutlineEnabled_ = enabled;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        auto t = annotations_[selectedIndex_].tool;
        if (t == AnnotationTool::Text || t == AnnotationTool::Numbered) {
            annotations_[selectedIndex_].textOutline = enabled;
            markModified();
        }
    }
    update();
}

void AnnotationCanvas::setFilled(bool filled)
{
    filled_ = filled;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        auto& a = annotations_[selectedIndex_];
        if (a.tool == AnnotationTool::Rectangle || a.tool == AnnotationTool::Ellipse
            || a.tool == AnnotationTool::Arrow) {
            a.filled = filled;
            markModified();
            if (onSelectionChanged_) onSelectionChanged_();
        }
    }
    update();
}

QColor AnnotationCanvas::fillColor() const { return currentFillColor_; }
void AnnotationCanvas::setFillColor(const QColor& color) {
    currentFillColor_ = color;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        annotations_[selectedIndex_].fillColor = color;
        markModified();
        if (onSelectionChanged_) onSelectionChanged_();
    }
    update();
}
void AnnotationCanvas::setTextBackgroundEnabled(bool enabled) {
    textBackgroundEnabled_ = enabled;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
        && annotations_[selectedIndex_].tool == AnnotationTool::Text) {
        annotations_[selectedIndex_].textBackground = enabled;
        markModified();
    }
    update();
}
bool AnnotationCanvas::textBackgroundEnabled() const { return textBackgroundEnabled_; }
void AnnotationCanvas::setTextBackgroundColor(const QColor& color) {
    textBackgroundColor_ = color;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
        && annotations_[selectedIndex_].tool == AnnotationTool::Text) {
        annotations_[selectedIndex_].textBackgroundColor = color;
        markModified();
    }
    update();
}
QColor AnnotationCanvas::textBackgroundColor() const { return textBackgroundColor_; }

void AnnotationCanvas::updateTextBounds(int index)
{
    if (index < 0 || index >= annotations_.size()) return;
    auto& a = annotations_[index];
    if (a.tool != AnnotationTool::Text) return;
    QFont font(a.fontFamily.isEmpty() ? QApplication::font().family() : a.fontFamily,
               a.textFontSize > 0 ? a.textFontSize : fontSize_);
    font.setBold(a.bold);
    font.setItalic(a.italic);
    font.setUnderline(a.underline);
    QFontMetrics fm(font);
    auto logicalW = image_.width() / image_.devicePixelRatio();
    int textWidth = fm.horizontalAdvance(a.text);
    int naturalW = qMax(textWidth + 8, 60);
    int newW = qMin(qMax(a.bounds.width(), naturalW), static_cast<int>(logicalW));
    int wrapWidth = qMax(newW - 8, 20);
    auto textRect = fm.boundingRect(QRect(0, 0, wrapWidth, 4096), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, a.text);
    int newH = qMax(textRect.height() + 8, fm.height() + 8);
    QRect newBounds(a.bounds.topLeft(), QSize(newW, newH));
    if (newBounds.right() > logicalW) {
        newBounds.moveRight(logicalW - 4);
    }
    if (newBounds.left() < 0) newBounds.moveLeft(0);
    a.bounds = newBounds;
}

int AnnotationCanvas::fontSize() const { return fontSize_; }
void AnnotationCanvas::setFontSize(int size, bool persist)
{
    size = qBound(8, size, 72);
    if (size != fontSize_) {
        fontSize_ = size;
        if (editingTextIndex_ >= 0) {
            annotations_[editingTextIndex_].textFontSize = size;
            updateTextBounds(editingTextIndex_);
        } else if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
                   && (annotations_[selectedIndex_].tool == AnnotationTool::Text
                       || annotations_[selectedIndex_].tool == AnnotationTool::Numbered)) {
            annotations_[selectedIndex_].textFontSize = size;
            updateTextBounds(selectedIndex_);
            markModified();
        }
        if (currentTool_ == AnnotationTool::Text || currentTool_ == AnnotationTool::Numbered) {
            markModified();
        }
        update();
        if (onFontSizeChanged_) onFontSizeChanged_(fontSize_);
        if (persist)
            QSettings().setValue("editor/fontSize", fontSize_);
    }
}
void AnnotationCanvas::setOnFontSizeChanged(std::function<void(int)> cb) { onFontSizeChanged_ = std::move(cb); }

QString AnnotationCanvas::fontFamily() const { return currentFontFamily_; }
void AnnotationCanvas::setFontFamily(const QString& family)
{
    if (family != currentFontFamily_) {
        currentFontFamily_ = family;
        if (editingTextIndex_ >= 0) {
            annotations_[editingTextIndex_].fontFamily = family;
            updateTextBounds(editingTextIndex_);
        } else if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
                   && annotations_[selectedIndex_].tool == AnnotationTool::Text) {
            annotations_[selectedIndex_].fontFamily = family;
            updateTextBounds(selectedIndex_);
            markModified();
        }
        update();
        if (onTextPropertiesChanged_) onTextPropertiesChanged_();
        QSettings().setValue("editor/fontFamily", family);
    }
}
bool AnnotationCanvas::bold() const { return bold_; }
void AnnotationCanvas::setBold(bool b)
{
    if (b != bold_) {
        bold_ = b;
        if (editingTextIndex_ >= 0) {
            annotations_[editingTextIndex_].bold = b;
            updateTextBounds(editingTextIndex_);
        } else if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
                   && annotations_[selectedIndex_].tool == AnnotationTool::Text) {
            annotations_[selectedIndex_].bold = b;
            updateTextBounds(selectedIndex_);
            markModified();
        }
        update();
        if (onTextPropertiesChanged_) onTextPropertiesChanged_();
        QSettings().setValue("editor/bold", b);
    }
}
bool AnnotationCanvas::italic() const { return italic_; }
void AnnotationCanvas::setItalic(bool i)
{
    if (i != italic_) {
        italic_ = i;
        if (editingTextIndex_ >= 0) {
            annotations_[editingTextIndex_].italic = i;
            updateTextBounds(editingTextIndex_);
        } else if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
                   && annotations_[selectedIndex_].tool == AnnotationTool::Text) {
            annotations_[selectedIndex_].italic = i;
            updateTextBounds(selectedIndex_);
            markModified();
        }
        update();
        if (onTextPropertiesChanged_) onTextPropertiesChanged_();
        QSettings().setValue("editor/italic", i);
    }
}
bool AnnotationCanvas::underline() const { return underline_; }
void AnnotationCanvas::setUnderline(bool u)
{
    if (u != underline_) {
        underline_ = u;
        if (editingTextIndex_ >= 0) {
            annotations_[editingTextIndex_].underline = u;
            updateTextBounds(editingTextIndex_);
        } else if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
                   && annotations_[selectedIndex_].tool == AnnotationTool::Text) {
            annotations_[selectedIndex_].underline = u;
            updateTextBounds(selectedIndex_);
            markModified();
        }
        update();
        if (onTextPropertiesChanged_) onTextPropertiesChanged_();
        QSettings().setValue("editor/underline", u);
    }
}
int AnnotationCanvas::textAlignment() const { return textAlignment_; }
void AnnotationCanvas::setTextAlignment(int align)
{
    if (align != textAlignment_) {
        textAlignment_ = align;
        if (editingTextIndex_ >= 0) {
            annotations_[editingTextIndex_].textAlignment = align;
            updateTextBounds(editingTextIndex_);
        } else if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
                   && annotations_[selectedIndex_].tool == AnnotationTool::Text) {
            annotations_[selectedIndex_].textAlignment = align;
            updateTextBounds(selectedIndex_);
            markModified();
        }
        update();
        if (onTextPropertiesChanged_) onTextPropertiesChanged_();
        QSettings().setValue("editor/textAlignment", align);
    }
}
void AnnotationCanvas::setOnTextPropertiesChanged(std::function<void()> cb) { onTextPropertiesChanged_ = std::move(cb); }

void AnnotationCanvas::syncTextPropertiesUI() { if (onTextPropertiesChanged_) onTextPropertiesChanged_(); }

void AnnotationCanvas::setOnSelectionChanged(std::function<void()> cb) { onSelectionChanged_ = std::move(cb); }

double AnnotationCanvas::cropAspectRatio() const { return cropAspectRatio_; }
void AnnotationCanvas::setCropAspectRatio(double ratio)
{
    cropAspectRatio_ = std::max(0.0, ratio);
    if (onCropAspectRatioChanged_) onCropAspectRatioChanged_(cropAspectRatio_);
    update();
}
void AnnotationCanvas::setOnCropAspectRatioChanged(std::function<void(double)> cb) { onCropAspectRatioChanged_ = std::move(cb); }

double AnnotationCanvas::zoomFactor() const { return zoomFactor_; }
QSize AnnotationCanvas::imageSize() const { return image_.size(); }
QColor AnnotationCanvas::color() const { return currentColor_; }
int AnnotationCanvas::strokeWidth() const { return currentStrokeWidth_; }
void AnnotationCanvas::setOnZoomChanged(std::function<void(double)> cb) { onZoomChanged_ = std::move(cb); }

int AnnotationCanvas::strokeAlpha() const { return strokeAlpha_; }
void AnnotationCanvas::setStrokeAlpha(int alpha)
{
    strokeAlpha_ = std::clamp(alpha, 0, 255);
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        annotations_[selectedIndex_].color.setAlpha(strokeAlpha_);
        markModified();
    }
    if (onStrokeAlphaChanged_) onStrokeAlphaChanged_(strokeAlpha_);
    update();
}
void AnnotationCanvas::setOnStrokeAlphaChanged(std::function<void(int)> cb) { onStrokeAlphaChanged_ = std::move(cb); }

ArrowStyle AnnotationCanvas::arrowStyle() const { return arrowStyle_; }
void AnnotationCanvas::setArrowStyle(ArrowStyle style)
{
    arrowStyle_ = style;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
        && annotations_[selectedIndex_].tool == AnnotationTool::Arrow) {
        annotations_[selectedIndex_].arrowStyle = style;
        markModified();
    }
    if (onArrowStyleChanged_) onArrowStyleChanged_(static_cast<int>(style));
    update();
}
void AnnotationCanvas::setOnArrowStyleChanged(std::function<void(int)> cb) { onArrowStyleChanged_ = std::move(cb); }

int AnnotationCanvas::cornerRadius() const { return cornerRadius_; }
void AnnotationCanvas::setCornerRadius(int radius)
{
    cornerRadius_ = std::clamp(radius, 0, 40);
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
        && annotations_[selectedIndex_].tool == AnnotationTool::Rectangle) {
        annotations_[selectedIndex_].cornerRadius = cornerRadius_;
        markModified();
    }
    if (onCornerRadiusChanged_) onCornerRadiusChanged_(cornerRadius_);
    update();
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
    editingTextIndex_ = -1;
    cursorPos_ = 0;
    preeditString_.clear();
    drawing_ = false;
    moving_ = false;
    resizing_ = false;

    bool restoringCrop = !preCropImage_.isNull() && !cropUndoOffset_.isNull();
    if (restoringCrop) {
        image_ = preCropImage_;
        preCropImage_ = {};
        cropUndoOffset_ = {};
        renderer_.invalidateCache();
        auto logicalSize = image_.size() / image_.devicePixelRatio();
        setMinimumSize(logicalSize);
        resize(logicalSize);
        updateWindowTitle();
    } else if (!preAdjustImage_.isNull()) {
        image_ = preAdjustImage_;
        preAdjustImage_ = {};
        brightness_ = 0;
        contrast_ = 0;
        renderer_.invalidateCache();
        auto logicalSize = image_.size() / image_.devicePixelRatio();
        setMinimumSize(logicalSize);
        resize(logicalSize);
        updateWindowTitle();
    }

    redoStack_.push_back(annotations_);
    annotations_ = undoStack_.takeLast();
    if (!cropUndoOffset_.isNull()) {
        for (auto& a : annotations_) {
            a.bounds.translate(cropUndoOffset_);
            for (auto& pt : a.points) pt += cropUndoOffset_;
        }
        cropUndoOffset_ = {};
    }
    int maxNumber = 0;
    for (const auto& a : annotations_) {
        if (a.number > maxNumber) maxNumber = a.number;
    }
    nextNumber_ = maxNumber + 1;
    selectedIndex_ = -1;
    markModified();
    if (onSelectionChanged_) onSelectionChanged_();
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
    editingTextIndex_ = -1;
    cursorPos_ = 0;
    preeditString_.clear();
    drawing_ = false;
    moving_ = false;
    resizing_ = false;
    pushUndo();
    annotations_ = redoStack_.takeLast();
    selectedIndex_ = -1;
    markModified();
    if (onSelectionChanged_) onSelectionChanged_();
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
            if (isModified()) {
                auto ret = QMessageBox::question(this, tr("Unsaved Changes"),
                    tr("Drop image and discard all annotations?"),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (ret != QMessageBox::Yes) {
                    return;
                }
            }
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
            pushUndo();
            redoStack_.clear();
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
                pushUndo();
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
    drawing_ = false;
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
        const auto dpr = image_.devicePixelRatio();
        QPoint physicalPos(static_cast<int>(pos.x() * dpr),
                           static_cast<int>(pos.y() * dpr));
        currentColor_ = QColor::fromRgba(image_.pixel(physicalPos));
    }
    if (onPickingColorChanged_) onPickingColorChanged_(false);
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
                pushUndo();
                redoStack_.clear();
                resizing_ = true;
                resizeCorner_ = ci;
                resizeStartBounds_ = r;
                resizeStartPoints_ = annotations_[selectedIndex_].points;
                return true;
            }
        }
        const QPoint midpoints[] = {
            QPoint(r.center().x(), r.top()), QPoint(r.right(), r.center().y()),
            QPoint(r.center().x(), r.bottom()), QPoint(r.left(), r.center().y())};
        for (int mi = 0; mi < 4; ++mi) {
            if (QRect(midpoints[mi].x() - 7, midpoints[mi].y() - 7, 14, 14).contains(pos)) {
                pushUndo();
                redoStack_.clear();
                resizing_ = true;
                resizeCorner_ = 4 + mi;
                resizeStartBounds_ = r;
                resizeStartPoints_ = annotations_[selectedIndex_].points;
                return true;
            }
        }
    }
    for (int i = annotations_.size() - 1; i >= 0; --i) {
        if (renderer_.hitTestAnnotation(annotations_.at(i), pos)) {
            pushUndo();
            redoStack_.clear();
            selectedIndex_ = i;
            moving_ = true;
            moveOffset_ = annotations_[i].bounds.topLeft() - pos;
            update();
            if (onSelectionChanged_) onSelectionChanged_();
            return true;
        }
    }
    selectedIndex_ = -1;
    update();
    if (onSelectionChanged_) onSelectionChanged_();
    return true;
}

bool AnnotationCanvas::handleEraserPress(const QPoint& pos)
{
    for (int i = annotations_.size() - 1; i >= 0; --i) {
        if (renderer_.hitTestAnnotation(annotations_.at(i), pos)) {
            pushUndo();
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
    ann.color.setAlpha(strokeAlpha_);
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
        cursorPos_ = 0;
        preeditString_.clear();
        update();
    }
    for (int i = annotations_.size() - 1; i >= 0; --i) {
        if (annotations_.at(i).tool == AnnotationTool::Text && renderer_.hitTestAnnotation(annotations_.at(i), pos)) {
            selectedIndex_ = i;
            editingTextIndex_ = i;
            update();
            return true;
        }
    }
    QFont font(currentFontFamily_.isEmpty() ? QApplication::font().family() : currentFontFamily_, fontSize_);
    font.setBold(bold_);
    font.setItalic(italic_);
    font.setUnderline(underline_);
    QFontMetrics fm(font);
    int defaultWidth = qMax(fm.horizontalAdvance(QStringLiteral("    ")) + 8, 60);
    QRect bounds(pos.x(), pos.y(), defaultWidth, fm.height() + 8);
    auto logicalW = image_.width() / image_.devicePixelRatio();
    if (bounds.right() > logicalW) {
        bounds.setRight(logicalW - 4);
        bounds.setWidth(qMin(bounds.width(), static_cast<int>(logicalW - bounds.left() - 4)));
    }
    pushUndo();
    redoStack_.clear();
    Annotation ann;
    ann.tool = AnnotationTool::Text;
    ann.bounds = bounds;
    ann.text = QString();
    ann.color = currentColor_;
    ann.color.setAlpha(strokeAlpha_);
    ann.strokeWidth = 2;
    ann.textFontSize = fontSize_;
    ann.textOutline = textOutlineEnabled_;
    ann.textBackground = textBackgroundEnabled_;
    ann.textBackgroundColor = textBackgroundColor_;
    ann.fontFamily = currentFontFamily_;
    ann.bold = bold_;
    ann.italic = italic_;
    ann.underline = underline_;
    ann.textAlignment = textAlignment_;
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
        if (renderer_.hitTestAnnotation(annotations_.at(i), pos)) {
            if (i != selectedIndex_) {
                selectedIndex_ = i;
                update();
                if (onSelectionChanged_) onSelectionChanged_();
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
    draft_.fillColor = currentFillColor_.isValid() ? currentFillColor_ : QColor();
    draft_.textBackground = textBackgroundEnabled_;
    draft_.textBackgroundColor = textBackgroundColor_;
    draft_.arrowStyle = arrowStyle_;
    draft_.cornerRadius = cornerRadius_;
    draft_.textFontSize = fontSize_;
    draft_.fontFamily = currentFontFamily_;
    draft_.bold = bold_;
    draft_.italic = italic_;
    draft_.underline = underline_;
    draft_.textAlignment = textAlignment_;
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
            && renderer_.hitTestAnnotation(annotations_[editingTextIndex_], pos));
        if (!clickedSelf) {
            if (editingTextIndex_ < annotations_.size()
                && annotations_[editingTextIndex_].tool == AnnotationTool::Text
                && annotations_[editingTextIndex_].text.isEmpty()
                && annotations_[editingTextIndex_].points.isEmpty()) {
                pushUndo();
                redoStack_.clear();
                annotations_.removeAt(editingTextIndex_);
                if (selectedIndex_ >= editingTextIndex_) selectedIndex_--;
                markModified();
                if (onSelectionChanged_) onSelectionChanged_();
            }
            editingTextIndex_ = -1;
            cursorPos_ = 0;
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
    if (!resizing_ && !moving_) {
        return;
    }
    renderer_.invalidateCache();
    if (resizing_) {
        auto& a = annotations_[selectedIndex_];
        auto b = resizeStartBounds_;
        const auto p = toImage(event->pos());
        if (resizeCorner_ < 4) {
            switch (resizeCorner_) {
            case 0: b.setTopLeft(p); break;
            case 1: b.setTopRight(p); break;
            case 2: b.setBottomLeft(p); break;
            case 3: b.setBottomRight(p); break;
            }
            a.bounds = b.normalized();
        } else {
            switch (resizeCorner_) {
            case 4: b.setTop(p.y()); break;
            case 5: b.setRight(p.x()); break;
            case 6: b.setBottom(p.y()); break;
            case 7: b.setLeft(p.x()); break;
            }
            a.bounds = b.normalized();
        }
        if (event->modifiers().testFlag(Qt::ShiftModifier) && resizeCorner_ < 4) {
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
        if (a.tool == AnnotationTool::Pen
            || a.tool == AnnotationTool::Arrow
            || a.tool == AnnotationTool::Line
            || a.tool == AnnotationTool::Mosaic) {
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
        if (annotations_[selectedIndex_].tool == AnnotationTool::Pen
            || annotations_[selectedIndex_].tool == AnnotationTool::Arrow
            || annotations_[selectedIndex_].tool == AnnotationTool::Line
            || annotations_[selectedIndex_].tool == AnnotationTool::Mosaic) {
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

    auto rawPos = toImage(event->pos());
    if (!(rawPos == start_) && cropAspectRatio_ > 0.0 && draft_.tool == AnnotationTool::Crop) {
        auto dx = rawPos.x() - start_.x();
        auto dy = rawPos.y() - start_.y();
        double adx = std::abs(dx);
        double ady = std::abs(dy);
        double nh = adx / cropAspectRatio_;
        if (nh > ady) {
            rawPos.setY(start_.y() + (dy >= 0 ? static_cast<int>(nh) : -static_cast<int>(nh)));
        } else {
            double nw = ady * cropAspectRatio_;
            rawPos.setX(start_.x() + (dx >= 0 ? static_cast<int>(nw) : -static_cast<int>(nw)));
        }
    } else if (!(rawPos == start_) && event->modifiers().testFlag(Qt::ShiftModifier)) {
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
    } else if (draft_.tool == AnnotationTool::Arrow || draft_.tool == AnnotationTool::Line) {
        draft_.points = {start_, current_};
        update();
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
        if (currentTool_ == AnnotationTool::Pen || currentTool_ == AnnotationTool::Eraser || currentTool_ == AnnotationTool::Mosaic) {
            updateBrushCursor();
        } else {
            switch (currentTool_) {
            case AnnotationTool::Text: setCursor(Qt::IBeamCursor); break;
            default: setCursor(Qt::CrossCursor); break;
            }
        }
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
        setTool(AnnotationTool::Select);
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
        if (onSelectionChanged_) onSelectionChanged_();
        if (currentTool_ != AnnotationTool::Select && currentTool_ != AnnotationTool::Text
            && currentTool_ != AnnotationTool::Numbered && currentTool_ != AnnotationTool::Mosaic
            && currentTool_ != AnnotationTool::Eraser) {
            setTool(AnnotationTool::Select);
        }
    } else {
        update();
    }
}

bool AnnotationCanvas::handleTextEditingKey(QKeyEvent* event)
{
    auto& textAnn = annotations_[editingTextIndex_];

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            editingTextIndex_ = -1;
            cursorPos_ = 0;
            preeditString_.clear();
            update();
            event->accept();
            return true;
        }
        textAnn.text.insert(cursorPos_, QChar::LineFeed);
        cursorPos_++;
        updateTextBounds(editingTextIndex_);
        markModified();
        event->accept();
        return true;
    }

    if (event->key() == Qt::Key_Escape) {
        if (textAnn.text.isEmpty()) {
            pushUndo();
            redoStack_.clear();
            annotations_.removeAt(editingTextIndex_);
            selectedIndex_ = -1;
        }
        editingTextIndex_ = -1;
        cursorPos_ = 0;
        preeditString_.clear();
        markModified();
        event->accept();
        return true;
    }

    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (event->key() == Qt::Key_C) {
            QApplication::clipboard()->setText(textAnn.text);
            event->accept();
            return true;
        }
        if (event->key() == Qt::Key_V) {
            QString clipText = QApplication::clipboard()->text();
            if (!clipText.isEmpty()) {
                textAnn.text.insert(cursorPos_, clipText);
                cursorPos_ += clipText.length();
                updateTextBounds(editingTextIndex_);
                markModified();
            }
            event->accept();
            return true;
        }
        if (event->key() == Qt::Key_A) {
            cursorPos_ = textAnn.text.length();
            update();
            event->accept();
            return true;
        }
    }

    if (event->key() == Qt::Key_Left) {
        if (cursorPos_ > 0) cursorPos_--;
        update();
        event->accept();
        return true;
    }
    if (event->key() == Qt::Key_Right) {
        if (cursorPos_ < textAnn.text.length()) cursorPos_++;
        update();
        event->accept();
        return true;
    }
    if (event->key() == Qt::Key_Home) {
        cursorPos_ = 0;
        update();
        event->accept();
        return true;
    }
    if (event->key() == Qt::Key_End) {
        cursorPos_ = textAnn.text.length();
        update();
        event->accept();
        return true;
    }

    if (event->key() == Qt::Key_Backspace) {
        if (cursorPos_ > 0) {
            cursorPos_--;
            textAnn.text.remove(cursorPos_, 1);
            updateTextBounds(editingTextIndex_);
            markModified();
        }
        event->accept();
        return true;
    }
    if (event->key() == Qt::Key_Delete) {
        if (cursorPos_ < textAnn.text.length()) {
            textAnn.text.remove(cursorPos_, 1);
            updateTextBounds(editingTextIndex_);
            markModified();
        }
        event->accept();
        return true;
    }

    QString text = event->text();
    if (!text.isEmpty() && text[0].isPrint()) {
        textAnn.text.insert(cursorPos_, text);
        cursorPos_ += text.length();
        updateTextBounds(editingTextIndex_);
        markModified();
        event->accept();
        return true;
    }

    return false;
}

void AnnotationCanvas::zoomFit()
{
    handleZoomFit();
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
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size())
        deleteAnnotation(selectedIndex_);
}

void AnnotationCanvas::handleDuplicateKey()
{
    editingTextIndex_ = -1;
    cursorPos_ = 0;
    preeditString_.clear();
    auto dup = annotations_.at(selectedIndex_);
    dup.bounds.translate(10, 10);
    for (auto& pt : dup.points) pt += QPoint(10, 10);
    pushUndo();
    redoStack_.clear();
    annotations_.push_back(std::move(dup));
    selectedIndex_ = annotations_.size() - 1;
    markModified();
    if (onSelectionChanged_) onSelectionChanged_();
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
    pushUndo();
    redoStack_.clear();
    int step = (QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier)) ? 10 : 1;
    QPoint delta(0, 0);
    if (key == Qt::Key_Up) delta.setY(-step);
    else if (key == Qt::Key_Down) delta.setY(step);
    else if (key == Qt::Key_Left) delta.setX(-step);
    else if (key == Qt::Key_Right) delta.setX(step);
    annotations_[selectedIndex_].bounds.translate(delta);
    if (annotations_[selectedIndex_].tool == AnnotationTool::Pen
        || annotations_[selectedIndex_].tool == AnnotationTool::Arrow
        || annotations_[selectedIndex_].tool == AnnotationTool::Line
        || annotations_[selectedIndex_].tool == AnnotationTool::Mosaic) {
        for (auto& pt : annotations_[selectedIndex_].points) pt += delta;
    }
    markModified();
}

void AnnotationCanvas::handleFontSizeChange(int delta)
{
    int newSize = fontSize_ + delta;
    if (newSize >= 8 && newSize <= 72) {
        fontSize_ = newSize;
        if (editingTextIndex_ >= 0) {
            annotations_[editingTextIndex_].textFontSize = fontSize_;
            updateTextBounds(editingTextIndex_);
        }
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
        if (handleTextEditingKey(event)) {
            return;
        }
    }

    if (event->key() == Qt::Key_Escape) {
        if (pickingColor_) {
            setPickingColor(false);
            event->accept();
            return;
        }
        if (drawing_) {
            drawing_ = false;
            update();
            event->accept();
            return;
        }
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
    drawing_ = false;
    if (pickingColor_) {
        setPickingColor(false);
    }
    if (editingTextIndex_ >= 0) {
        editingTextIndex_ = -1;
        cursorPos_ = 0;
        preeditString_.clear();
        update();
    }
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
        for (auto& pt : dup.points) pt += QPoint(10, 10);
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

void AnnotationCanvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), QColor("#101418"));
    if (!image_.isNull()) {
        painter.save();
        painter.scale(zoomFactor_, zoomFactor_);
        if (image_.hasAlphaChannel()) {
            renderer_.drawCheckerboard(painter, image_);
        }
        painter.drawImage(QPoint(0, 0), image_);
        renderer_.drawAnnotations(painter, image_, annotations_, fontSize_);
        if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
            painter.setPen(QPen(QColor("#2fbf9f"), 1, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(annotations_[selectedIndex_].bounds.adjusted(-3, -3, 3, 3));
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#2fbf9f"));
            const auto r = annotations_[selectedIndex_].bounds;
            const QPoint corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
            for (const auto& c : corners) {
                painter.drawRect(QRect(c.x() - 4, c.y() - 4, 8, 8));
            }
            const QPoint midpoints[] = {
                QPoint(r.center().x(), r.top()), QPoint(r.right(), r.center().y()),
                QPoint(r.center().x(), r.bottom()), QPoint(r.left(), r.center().y())};
            painter.setBrush(QColor("#ffffff"));
            for (const auto& m : midpoints) {
                painter.drawRect(QRect(m.x() - 3, m.y() - 3, 6, 6));
            }
        }
        if (drawing_) {
            renderer_.drawDraft(painter, image_, draft_, fontSize_);
        }
        painter.restore();
        if (gridEnabled_) {
            renderer_.drawGridOverlay(painter, rect(), zoomFactor_);
        }
        renderer_.drawTextEditCursor(painter, annotations_, editingTextIndex_,
            cursorPos_, preeditString_, fontSize_, zoomFactor_);
    }
    // Pixel info overlay near cursor
    if (!image_.isNull() && mousePixelColor_.isValid()) {
        auto dpr = image_.devicePixelRatio();
        QPoint px(static_cast<int>(mouseImagePos_.x() * dpr),
                  static_cast<int>(mouseImagePos_.y() * dpr));
        QRect imgRect(QPoint(0, 0), image_.size());
        if (imgRect.contains(px)) {
            QString info = tr("(%1, %2) %3")
                .arg(static_cast<int>(mouseImagePos_.x()))
                .arg(static_cast<int>(mouseImagePos_.y()))
                .arg(mousePixelColor_.name(QColor::HexRgb).toUpper());
            static QFont infoFont = []{ QFont f; f.setPixelSize(11); return f; }();
            painter.setFont(infoFont);
            auto textRect = painter.fontMetrics().boundingRect(info);
            int ox = static_cast<int>(mouseImagePos_.x() * zoomFactor_) + 14;
            int oy = static_cast<int>(mouseImagePos_.y() * zoomFactor_) - textRect.height() - 6;
            int overlayW = textRect.width() + 8;
            int overlayH = textRect.height() + 4;
            if (ox + overlayW > width()) ox = width() - overlayW - 10;
            if (oy < 2) oy = static_cast<int>(mouseImagePos_.y() * zoomFactor_) + 14;
            if (oy + overlayH > height()) oy = static_cast<int>(mouseImagePos_.y() * zoomFactor_) - overlayH - 6;
            QRect bgRect(ox, oy, overlayW, overlayH);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 180));
            painter.drawRoundedRect(bgRect, 3, 3);
            painter.setPen(Qt::white);
            painter.setBrush(Qt::NoBrush);
            painter.drawText(bgRect, Qt::AlignCenter, info);
            // Color swatch
            int swatchSize = 10;
            bool swatchRight = bgRect.right() + 4 + swatchSize <= width();
            QRect swatchRect(swatchRight ? bgRect.right() + 4 : bgRect.left() - swatchSize - 4,
                             bgRect.center().y() - swatchSize / 2, swatchSize, swatchSize);
            painter.setPen(Qt::NoPen);
            painter.setBrush(mousePixelColor_);
            painter.drawRoundedRect(swatchRect, 2, 2);
        }
    }
    renderer_.drawDraftSizeLabel(painter, current_, draft_, drawing_, zoomFactor_);
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
    QFont font(a.fontFamily.isEmpty() ? QApplication::font().family() : a.fontFamily,
                       a.textFontSize > 0 ? a.textFontSize : fontSize_);
            font.setBold(a.bold);
            font.setItalic(a.italic);
            font.setUnderline(a.underline);
            QFontMetrics fm(font);
            int textWidth = fm.horizontalAdvance(a.text + preeditString_);
            int cx = static_cast<int>((a.bounds.left() + 4 + textWidth) * zoomFactor_);
            int cy = static_cast<int>((a.bounds.top() + 4) * zoomFactor_);
            int ch = static_cast<int>(fm.height() * zoomFactor_);
            return QRect(mapToGlobal(QPoint(0, 0)) + QPoint(cx, cy), QSize(4, ch));
        }
        case Qt::ImEnabled:
            return true;
        case Qt::ImFont: {
            QFont f(a.fontFamily.isEmpty() ? QApplication::font().family() : a.fontFamily,
                    a.textFontSize > 0 ? a.textFontSize : fontSize_);
            f.setBold(a.bold);
            f.setItalic(a.italic);
            f.setUnderline(a.underline);
            return f;
        }
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

void AnnotationCanvas::selectAnnotation(int index)
{
    if (index < 0 || index >= annotations_.size()) return;
    selectedIndex_ = index;
    editingTextIndex_ = -1;
    cursorPos_ = 0;
    preeditString_.clear();
    update();
    if (onSelectionChanged_) onSelectionChanged_();
}

void AnnotationCanvas::deleteAnnotation(int index)
{
    if (index < 0 || index >= annotations_.size()) return;
    pushUndo();
    redoStack_.clear();
    annotations_.removeAt(index);
    if (selectedIndex_ == index) selectedIndex_ = -1;
    else if (selectedIndex_ > index) --selectedIndex_;
    markModified();
}

void AnnotationCanvas::duplicateAnnotation(int index)
{
    if (index < 0 || index >= annotations_.size()) return;
    editingTextIndex_ = -1;
    cursorPos_ = 0;
    preeditString_.clear();
    auto dup = annotations_.at(index);
    dup.bounds.translate(10, 10);
    for (auto& pt : dup.points) pt += QPoint(10, 10);
    pushUndo();
    redoStack_.clear();
    annotations_.push_back(std::move(dup));
    selectedIndex_ = annotations_.size() - 1;
    markModified();
}

void AnnotationCanvas::swapAnnotations(int i, int j)
{
    if (i < 0 || i >= annotations_.size()) return;
    if (j < 0 || j >= annotations_.size()) return;
    pushUndo();
    redoStack_.clear();
    qSwap(annotations_[i], annotations_[j]);
    if (selectedIndex_ == i) selectedIndex_ = j;
    else if (selectedIndex_ == j) selectedIndex_ = i;
    markModified();
}

void AnnotationCanvas::setAnnotationVisible(int index, bool visible)
{
    if (index < 0 || index >= annotations_.size()) return;
    if (annotations_[index].visible == visible) return;
    pushUndo();
    redoStack_.clear();
    annotations_[index].visible = visible;
    markModified();
}

} // namespace snappaste
