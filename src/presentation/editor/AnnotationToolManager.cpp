#include "presentation/editor/AnnotationToolManager.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QSettings>

namespace snappaste {

AnnotationToolManager::AnnotationToolManager()
{
    // Batch-load editor settings from QSettings (single registry read)
    struct EditorPrefs {
        int fontSize = 14;
        QString fontFamily;
        bool bold = false, italic = false, underline = false;
        int textAlignment = -1;
        QVector<QColor> recentColors;
    };
    static const EditorPrefs prefs = [] {
        QSettings s;
        EditorPrefs p;
        p.fontSize = s.value("editor/fontSize", 14).toInt();
        p.fontFamily = s.value("editor/fontFamily", QApplication::font().family()).toString();
        p.bold = s.value("editor/bold", false).toBool();
        p.italic = s.value("editor/italic", false).toBool();
        p.underline = s.value("editor/underline", false).toBool();
        p.textAlignment = s.value("editor/textAlignment", -1).toInt();
        const auto saved = s.value("editor/recentColors").toList();
        for (const auto& v : saved) {
            QColor c(v.toString());
            if (c.isValid())
                p.recentColors.append(c);
        }
        return p;
    }();

    fontSize_ = prefs.fontSize;
    currentFontFamily_ = prefs.fontFamily;
    bold_ = prefs.bold;
    italic_ = prefs.italic;
    underline_ = prefs.underline;
    textAlignment_ = prefs.textAlignment;
    customColors_ = prefs.recentColors;
}

void AnnotationToolManager::setImage(const QImage& /*image*/, double zoom)
{
    annotations_.clear();
    undoStack_.clear();
    redoStack_.clear();
    selectedIndex_ = -1;
    editingTextIndex_ = -1;
    cursorPos_ = 0;
    preeditString_.clear();
    nextNumber_ = 1;
    imageHistory_.clear();
    redoImageHistory_.clear();
    zoomFactor_ = zoom;
}

// --- Tool state setters ---

void AnnotationToolManager::setTool(AnnotationTool tool)
{
    if (pickingColor_) {
        setPickingColor(false);
    }
    if (currentTool_ == tool) return;
    currentTool_ = tool;
    recentTools_.removeAll(tool);
    recentTools_.prepend(tool);
    if (recentTools_.size() > 4) recentTools_.resize(4);
    if (onToolChanged) onToolChanged(tool);
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setColor(const QColor& color)
{
    currentColor_ = color;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        pushUndo();
        annotations_[selectedIndex_].color = color;
        if (onModified) onModified();
    }
}

void AnnotationToolManager::setFillColor(const QColor& color)
{
    currentFillColor_ = color;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        pushUndo();
        annotations_[selectedIndex_].fillColor = color;
        if (onModified) onModified();
        if (onSelectionChanged) onSelectionChanged();
    }
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setStrokeWidth(int width)
{
    currentStrokeWidth_ = std::clamp(width, 1, 12);
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        pushUndo();
        annotations_[selectedIndex_].strokeWidth = currentStrokeWidth_;
        if (onModified) onModified();
    }
}

void AnnotationToolManager::setStrokeAlpha(int alpha)
{
    strokeAlpha_ = std::clamp(alpha, 0, 255);
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        pushUndo();
        annotations_[selectedIndex_].color.setAlpha(strokeAlpha_);
        if (onModified) onModified();
    }
    if (onStrokeAlphaChanged) onStrokeAlphaChanged(strokeAlpha_);
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setArrowStyle(ArrowStyle style)
{
    arrowStyle_ = style;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
        && annotations_[selectedIndex_].tool == AnnotationTool::Arrow) {
        pushUndo();
        annotations_[selectedIndex_].arrowStyle = style;
        if (onModified) onModified();
    }
    if (onArrowStyleChanged) onArrowStyleChanged(static_cast<int>(style));
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setCornerRadius(int radius)
{
    cornerRadius_ = std::clamp(radius, 0, 40);
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
        && annotations_[selectedIndex_].tool == AnnotationTool::Rectangle) {
        pushUndo();
        annotations_[selectedIndex_].cornerRadius = cornerRadius_;
        if (onModified) onModified();
    }
    if (onCornerRadiusChanged) onCornerRadiusChanged(cornerRadius_);
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setFontSize(int size, bool persist)
{
    size = qBound(8, size, 72);
    if (size != fontSize_) {
        fontSize_ = size;
        if (editingTextIndex_ >= 0 && editingTextIndex_ < annotations_.size()) {
            pushUndo();
            annotations_[editingTextIndex_].textFontSize = size;
            updateTextBounds(editingTextIndex_);
            if (onModified) onModified();
        } else if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
                   && (annotations_[selectedIndex_].tool == AnnotationTool::Text
                       || annotations_[selectedIndex_].tool == AnnotationTool::Numbered)) {
            pushUndo();
            annotations_[selectedIndex_].textFontSize = size;
            updateTextBounds(selectedIndex_);
            if (onModified) onModified();
        }
        if (onUpdateRequired) onUpdateRequired();
        if (onFontSizeChanged) onFontSizeChanged(fontSize_);
        if (persist)
            QSettings().setValue("editor/fontSize", fontSize_);
    }
}

namespace {

using ApplyTextFn = std::function<void(Annotation&)>;

template<typename T>
void applyTextProperty(T& member, const T& value, AnnotationToolManager& mgr,
                       ApplyTextFn fn, const char* settingsKey)
{
    if (member == value) return;
    member = value;

    int editIdx = mgr.editingTextIndex();
    auto& anns = mgr.annotationsMut();

    if (editIdx >= 0) {
        fn(anns[editIdx]);
        mgr.updateTextBounds(editIdx);
        if (mgr.onModified) mgr.onModified();
    } else {
        int sel = mgr.selectedIndex();
        if (sel >= 0 && sel < static_cast<int>(anns.size())
            && (anns[sel].tool == AnnotationTool::Text
                || anns[sel].tool == AnnotationTool::Numbered)) {
            mgr.pushUndo();
            fn(anns[sel]);
            mgr.updateTextBounds(sel);
            if (mgr.onModified) mgr.onModified();
        }
    }

    if (mgr.onUpdateRequired) mgr.onUpdateRequired();
    if (mgr.onTextPropertiesChanged) mgr.onTextPropertiesChanged();
    QSettings().setValue(settingsKey, QVariant::fromValue(value));
}

} // namespace

void AnnotationToolManager::setFontFamily(const QString& family)
{
    applyTextProperty(currentFontFamily_, family, *this,
        [&family](Annotation& a) { a.fontFamily = family; }, "editor/fontFamily");
}

void AnnotationToolManager::setBold(bool b)
{
    applyTextProperty(bold_, b, *this,
        [b](Annotation& a) { a.bold = b; }, "editor/bold");
}

void AnnotationToolManager::setItalic(bool i)
{
    applyTextProperty(italic_, i, *this,
        [i](Annotation& a) { a.italic = i; }, "editor/italic");
}

void AnnotationToolManager::setUnderline(bool u)
{
    applyTextProperty(underline_, u, *this,
        [u](Annotation& a) { a.underline = u; }, "editor/underline");
}

void AnnotationToolManager::setTextAlignment(int align)
{
    applyTextProperty(textAlignment_, align, *this,
        [align](Annotation& a) { a.textAlignment = align; }, "editor/textAlignment");
}

void AnnotationToolManager::setTextOutlineEnabled(bool enabled)
{
    textOutlineEnabled_ = enabled;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        pushUndo();
        auto t = annotations_[selectedIndex_].tool;
        if (t == AnnotationTool::Text || t == AnnotationTool::Numbered) {
            annotations_[selectedIndex_].textOutline = enabled;
            if (onModified) onModified();
        }
    }
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setFilled(bool filled)
{
    filled_ = filled;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
        pushUndo();
        auto& a = annotations_[selectedIndex_];
        if (a.tool == AnnotationTool::Rectangle || a.tool == AnnotationTool::Ellipse
            || a.tool == AnnotationTool::Arrow) {
            a.filled = filled;
            if (onModified) onModified();
            if (onSelectionChanged) onSelectionChanged();
        }
    }
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setTextBackgroundEnabled(bool enabled)
{
    textBackgroundEnabled_ = enabled;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
        && annotations_[selectedIndex_].tool == AnnotationTool::Text) {
        pushUndo();
        annotations_[selectedIndex_].textBackground = enabled;
        if (onModified) onModified();
    }
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setTextBackgroundColor(const QColor& color)
{
    textBackgroundColor_ = color;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
        && annotations_[selectedIndex_].tool == AnnotationTool::Text) {
        pushUndo();
        annotations_[selectedIndex_].textBackgroundColor = color;
        if (onModified) onModified();
    }
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setMosaicBlurred(bool blurred)
{
    mosaicBlurred_ = blurred;
    if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
        && annotations_[selectedIndex_].tool == AnnotationTool::Mosaic) {
        pushUndo();
        annotations_[selectedIndex_].blurRadius = blurred ? currentStrokeWidth_ : 0;
        if (onModified) onModified();
    }
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setPickingColor(bool picking)
{
    pickingColor_ = picking;
    if (onPickingColorChanged) onPickingColorChanged(picking);
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setCropAspectRatio(double ratio)
{
    cropAspectRatio_ = std::max(0.0, ratio);
    if (onCropAspectRatioChanged) onCropAspectRatioChanged(cropAspectRatio_);
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::setGridEnabled(bool enabled)
{
    gridEnabled_ = enabled;
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::addRecentColor(const QColor& color)
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

// --- Annotation manipulation ---

void AnnotationToolManager::selectAnnotation(int index)
{
    if (index < 0 || index >= annotations_.size()) return;
    selectedIndex_ = index;
    editingTextIndex_ = -1;
    cursorPos_ = 0;
    preeditString_.clear();
    if (onUpdateRequired) onUpdateRequired();
    if (onSelectionChanged) onSelectionChanged();
}

void AnnotationToolManager::deleteAnnotation(int index)
{
    if (index < 0 || index >= annotations_.size()) return;
    pushUndo();
    annotations_.removeAt(index);
    if (selectedIndex_ == index) selectedIndex_ = -1;
    else if (selectedIndex_ > index) --selectedIndex_;
    if (onModified) onModified();
    if (onSelectionChanged) onSelectionChanged();
}

void AnnotationToolManager::duplicateAnnotation(int index)
{
    if (index < 0 || index >= annotations_.size()) return;
    editingTextIndex_ = -1;
    cursorPos_ = 0;
    preeditString_.clear();
    auto dup = annotations_.at(index);
    dup.bounds.translate(10, 10);
    for (auto& pt : dup.points) pt += QPoint(10, 10);
    pushUndo();
    annotations_.push_back(std::move(dup));
    selectedIndex_ = annotations_.size() - 1;
    if (onModified) onModified();
    if (onSelectionChanged) onSelectionChanged();
}

void AnnotationToolManager::swapAnnotations(int i, int j)
{
    if (i < 0 || i >= annotations_.size()) return;
    if (j < 0 || j >= annotations_.size()) return;
    pushUndo();
    qSwap(annotations_[i], annotations_[j]);
    if (selectedIndex_ == i) selectedIndex_ = j;
    else if (selectedIndex_ == j) selectedIndex_ = i;
    if (onModified) onModified();
    if (onSelectionChanged) onSelectionChanged();
}

void AnnotationToolManager::setAnnotationVisible(int index, bool visible)
{
    if (index < 0 || index >= annotations_.size()) return;
    if (annotations_[index].visible == visible) return;
    pushUndo();
    annotations_[index].visible = visible;
    if (onModified) onModified();
}

// --- Undo/Redo ---

void AnnotationToolManager::pushUndoSnapshot(bool clearRedo)
{
    undoStack_.push_back(annotations_);
    imageHistory_.push_back({image_, baseImage_, brightness_, contrast_, zoomFactor_});
    if (clearRedo) {
        redoStack_.clear();
        redoImageHistory_.clear();
    }
    if (undoStack_.size() > kMaxUndo) {
        undoStack_.removeFirst();
        imageHistory_.removeFirst();
    }
}

void AnnotationToolManager::pushUndo()
{
    pushUndoSnapshot(true);
}

void AnnotationToolManager::undo()
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

    redoStack_.push_back(annotations_);
    redoImageHistory_.push_back({image_, baseImage_, brightness_, contrast_, zoomFactor_});

    annotations_ = undoStack_.takeLast();

    auto snap = imageHistory_.takeLast();
    image_ = snap.image;
    baseImage_ = snap.baseImage;
    brightness_ = snap.brightness;
    contrast_ = snap.contrast;
    zoomFactor_ = snap.zoomFactor;

    if (onWindowTitleUpdate) onWindowTitleUpdate();

    int maxNumber = 0;
    for (const auto& a : annotations_) {
        if (a.number > maxNumber) maxNumber = a.number;
    }
    nextNumber_ = maxNumber + 1;
    selectedIndex_ = -1;
    if (onModified) onModified();
    if (onSelectionChanged) onSelectionChanged();
    if (onZoomChanged) onZoomChanged(zoomFactor_);
}

void AnnotationToolManager::redo()
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

    pushUndoSnapshot(false);

    annotations_ = redoStack_.takeLast();
    auto snap = redoImageHistory_.takeLast();
    image_ = snap.image;
    baseImage_ = snap.baseImage;
    brightness_ = snap.brightness;
    contrast_ = snap.contrast;
    zoomFactor_ = snap.zoomFactor;

    if (onWindowTitleUpdate) onWindowTitleUpdate();

    selectedIndex_ = -1;
    if (onModified) onModified();
    if (onSelectionChanged) onSelectionChanged();
    if (onZoomChanged) onZoomChanged(zoomFactor_);
}

// --- Drawing ---

void AnnotationToolManager::startDrawing(const QPoint& pos)
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
    if (onUpdateRequired) onUpdateRequired();
}

void AnnotationToolManager::updateDrawingStroke(const QPoint& rawPos)
{
    if (draft_.tool == AnnotationTool::Numbered) {
        return;
    }

    auto oldCurrent = current_;
    current_ = rawPos;
    draft_.bounds = QRect(start_, current_).normalized();
    if (draft_.tool == AnnotationTool::Pen) {
        draft_.points.push_back(current_);
    } else if (draft_.tool == AnnotationTool::Mosaic) {
        draft_.points.push_back(current_);
    } else if (draft_.tool == AnnotationTool::Arrow || draft_.tool == AnnotationTool::Line) {
        draft_.points = {start_, current_};
    }
}

void AnnotationToolManager::finishDrawing()
{
    drawing_ = false;
}

void AnnotationToolManager::clearAnnotations()
{
    annotations_.clear();
    selectedIndex_ = -1;
    editingTextIndex_ = -1;
    cursorPos_ = 0;
    preeditString_.clear();
    nextNumber_ = 1;
}

// --- Move/resize ---

void AnnotationToolManager::startMoving(const QPoint& offset)
{
    moving_ = true;
    moveOffset_ = offset;
}

void AnnotationToolManager::updateMove(const QPoint& imagePos)
{
    const auto newPos = imagePos + moveOffset_;
    const auto delta = newPos - annotations_[selectedIndex_].bounds.topLeft();
    annotations_[selectedIndex_].bounds.translate(delta);
    auto tool = annotations_[selectedIndex_].tool;
    if (tool == AnnotationTool::Pen || tool == AnnotationTool::Arrow
        || tool == AnnotationTool::Line || tool == AnnotationTool::Mosaic) {
        for (auto& pt : annotations_[selectedIndex_].points) {
            pt += delta;
        }
    }
    moveOffset_ = annotations_[selectedIndex_].bounds.topLeft() - imagePos;
}

void AnnotationToolManager::startResizing(int corner, const QRect& bounds, const QVector<QPoint>& points)
{
    resizing_ = true;
    resizeCorner_ = corner;
    resizeStartBounds_ = bounds;
    resizeStartPoints_ = points;
}

void AnnotationToolManager::updateResize(const QPoint& imagePos, bool shiftHeld)
{
    auto& a = annotations_[selectedIndex_];
    auto b = resizeStartBounds_;
    const auto p = imagePos;
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
    if (shiftHeld && resizeCorner_ < 4) {
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
    auto tool = a.tool;
    if (tool == AnnotationTool::Pen || tool == AnnotationTool::Arrow
        || tool == AnnotationTool::Line || tool == AnnotationTool::Mosaic) {
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
}

// --- Text ---

void AnnotationToolManager::updateTextBounds(int index)
{
    if (index < 0 || index >= annotations_.size()) return;
    auto& a = annotations_[index];
    if (a.tool != AnnotationTool::Text) return;
    QFont font(a.fontFamily.isEmpty() ? QApplication::font().family() : a.fontFamily);
    font.setPixelSize(a.textFontSize > 0 ? a.textFontSize : fontSize_);
    font.setBold(a.bold);
    font.setItalic(a.italic);
    font.setUnderline(a.underline);
    QFontMetrics fm(font);
    if (image_.isNull()) return;
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

void AnnotationToolManager::syncTextPropertiesUI()
{
    if (onTextPropertiesChanged) onTextPropertiesChanged();
}

} // namespace snappaste
