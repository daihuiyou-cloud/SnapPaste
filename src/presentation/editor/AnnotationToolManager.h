#pragma once

#include "domain/editor/Annotation.h"

#include <QColor>
#include <QCursor>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QVector>

#include <functional>

namespace snappaste {

class AnnotationToolManager {
public:
    AnnotationToolManager();

    // --- Callbacks (wired by AnnotationCanvas) ---
    std::function<void()> onModified;
    std::function<void()> onSelectionChanged;
    std::function<void(AnnotationTool)> onToolChanged;
    std::function<void(int)> onFontSizeChanged;
    std::function<void(bool)> onPickingColorChanged;
    std::function<void(double)> onZoomChanged;
    std::function<void(int)> onStrokeAlphaChanged;
    std::function<void(int)> onArrowStyleChanged;
    std::function<void(int)> onCornerRadiusChanged;
    std::function<void()> onTextPropertiesChanged;
    std::function<void(double)> onCropAspectRatioChanged;
    std::function<void()> onImageEdited;
    std::function<void()> onUpdateRequired;
    std::function<void(QPointF, QColor)> onMouseInfoChanged;
    std::function<void(QCursor)> onSetCursor;
    std::function<void()> onWindowTitleUpdate;
    std::function<void(const QSize&)> onResizeCanvas;
    std::function<void(int, int)> onScrollBy;

    // --- Image (owned by Canvas, referenced here) ---
    void setImage(const QImage& image, double zoomFactor);
    const QImage& image() const { return image_; }
    const QImage& baseImage() const { return baseImage_; }
    void setImageDirect(const QImage& img) { image_ = img; }
    void setBaseImage(const QImage& img) { baseImage_ = img; }
    int brightness() const { return brightness_; }
    void setBrightness(int b) { brightness_ = b; }
    int contrast() const { return contrast_; }
    void setContrast(int c) { contrast_ = c; }

    // --- Annotations ---
    const QVector<Annotation>& annotations() const { return annotations_; }
    QVector<Annotation>& annotationsMut() { return annotations_; }
    int annotationCount() const { return annotations_.size(); }
    const Annotation& annotationAt(int index) const { return annotations_.at(index); }

    // --- Selection ---
    int selectedIndex() const { return (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) ? selectedIndex_ : -1; }
    void setSelectedIndex(int idx) { selectedIndex_ = idx; }
    int editingTextIndex() const { return editingTextIndex_; }
    void setEditingTextIndex(int idx) { editingTextIndex_ = idx; }
    int cursorPos() const { return cursorPos_; }
    void setCursorPos(int pos) { cursorPos_ = pos; }
    const QString& preeditString() const { return preeditString_; }
    void setPreeditString(const QString& s) { preeditString_ = s; }

    // --- Tool state ---
    AnnotationTool currentTool() const { return currentTool_; }
    void setTool(AnnotationTool tool);

    QColor color() const { return currentColor_; }
    void setColor(const QColor& color);

    QColor fillColor() const { return currentFillColor_; }
    void setFillColor(const QColor& color);

    int strokeWidth() const { return currentStrokeWidth_; }
    void setStrokeWidth(int width);

    int strokeAlpha() const { return strokeAlpha_; }
    void setStrokeAlpha(int alpha);

    ArrowStyle arrowStyle() const { return arrowStyle_; }
    void setArrowStyle(ArrowStyle style);

    int cornerRadius() const { return cornerRadius_; }
    void setCornerRadius(int radius);

    int fontSize() const { return fontSize_; }
    void setFontSize(int size, bool persist = true);

    QString fontFamily() const { return currentFontFamily_; }
    void setFontFamily(const QString& family);

    bool bold() const { return bold_; }
    void setBold(bool b);

    bool italic() const { return italic_; }
    void setItalic(bool i);

    bool underline() const { return underline_; }
    void setUnderline(bool u);

    int textAlignment() const { return textAlignment_; }
    void setTextAlignment(int align);

    bool textOutlineEnabled() const { return textOutlineEnabled_; }
    void setTextOutlineEnabled(bool enabled);

    bool filled() const { return filled_; }
    void setFilled(bool filled);

    bool textBackgroundEnabled() const { return textBackgroundEnabled_; }
    void setTextBackgroundEnabled(bool enabled);

    QColor textBackgroundColor() const { return textBackgroundColor_; }
    void setTextBackgroundColor(const QColor& color);

    bool mosaicBlurred() const { return mosaicBlurred_; }
    void setMosaicBlurred(bool blurred);

    bool pickingColor() const { return pickingColor_; }
    void setPickingColor(bool picking);

    double cropAspectRatio() const { return cropAspectRatio_; }
    void setCropAspectRatio(double ratio);

    bool gridEnabled() const { return gridEnabled_; }
    void setGridEnabled(bool enabled);

    double zoomFactor() const { return zoomFactor_; }
    void setZoomFactor(double z) { zoomFactor_ = z; }

    // --- Drawing state ---
    bool drawing() const { return drawing_; }
    void setDrawing(bool d) { drawing_ = d; }
    const Annotation& draft() const { return draft_; }
    Annotation& draftMut() { return draft_; }
    QPoint start() const { return start_; }
    void setStart(const QPoint& p) { start_ = p; }
    QPoint current() const { return current_; }
    void setCurrent(const QPoint& p) { current_ = p; }
    int nextNumber() const { return nextNumber_; }
    void setNextNumber(int n) { nextNumber_ = n; }
    void advanceNumber() { nextNumber_++; }

    // --- Move/resize state ---
    bool moving() const { return moving_; }
    void setMoving(bool m) { moving_ = m; }
    bool resizing() const { return resizing_; }
    void setResizing(bool r) { resizing_ = r; }
    int resizeCorner() const { return resizeCorner_; }
    void setResizeCorner(int c) { resizeCorner_ = c; }
    const QRect& resizeStartBounds() const { return resizeStartBounds_; }
    void setResizeStartBounds(const QRect& r) { resizeStartBounds_ = r; }
    const QVector<QPoint>& resizeStartPoints() const { return resizeStartPoints_; }
    void setResizeStartPoints(const QVector<QPoint>& pts) { resizeStartPoints_ = pts; }
    QPoint moveOffset() const { return moveOffset_; }
    void setMoveOffset(const QPoint& off) { moveOffset_ = off; }

    // --- Pan state ---
    bool panning() const { return panning_; }
    void setPanning(bool p) { panning_ = p; }
    QPoint panStart() const { return panStart_; }
    void setPanStart(const QPoint& p) { panStart_ = p; }

    // --- Mouse info ---
    QPointF mouseImagePos() const { return mouseImagePos_; }
    void setMouseImagePos(const QPointF& pos) { mouseImagePos_ = pos; }
    QColor mousePixelColor() const { return mousePixelColor_; }
    void setMousePixelColor(const QColor& c) { mousePixelColor_ = c; }

    // --- Recent tools / colors ---
    const QVector<AnnotationTool>& recentTools() const { return recentTools_; }
    const QVector<QColor>& recentColors() const { return customColors_; }
    void addRecentColor(const QColor& color);

    // --- Annotation manipulation ---
    void selectAnnotation(int index);
    void deleteAnnotation(int index);
    void duplicateAnnotation(int index);
    void swapAnnotations(int i, int j);
    void setAnnotationVisible(int index, bool visible);

    // --- Undo/Redo ---
    void pushUndo();
    void pushUndoSnapshot(bool clearRedo);
    void undo();
    void redo();
    int undoCount() const { return undoStack_.size(); }
    int redoCount() const { return redoStack_.size(); }

    // --- Start / update / finish drawing ---
    void startDrawing(const QPoint& pos);
    void updateDrawingStroke(const QPoint& rawPos);
    void finishDrawing();

    // --- Move/resize ---
    void startMoving(const QPoint& offset);
    void updateMove(const QPoint& imagePos);
    void startResizing(int corner, const QRect& bounds, const QVector<QPoint>& points);
    void updateResize(const QPoint& imagePos, bool shiftHeld);

    // --- Text ---
    void updateTextBounds(int index);
    void syncTextPropertiesUI();

    // --- Clear ---
    void clearAnnotations();

private:
    // Image
    QImage image_;
    QImage baseImage_;
    int brightness_ = 0;
    int contrast_ = 0;

    // Annotations
    QVector<Annotation> annotations_;
    QVector<QVector<Annotation>> undoStack_;
    QVector<QVector<Annotation>> redoStack_;

    struct ImageSnapshot {
        QImage image;
        QImage baseImage;
        int brightness = 0;
        int contrast = 0;
        double zoomFactor = 1.0;
    };
    QVector<ImageSnapshot> imageHistory_;
    QVector<ImageSnapshot> redoImageHistory_;

    static constexpr int kMaxUndo = 20;

    // Selection
    int selectedIndex_ = -1;
    int editingTextIndex_ = -1;
    int cursorPos_ = 0;
    QString preeditString_;

    // Tool state
    AnnotationTool currentTool_ = AnnotationTool::Rectangle;
    QColor currentColor_{"#ff3b30"};
    QColor currentFillColor_;
    int currentStrokeWidth_ = 4;
    int strokeAlpha_ = 255;
    ArrowStyle arrowStyle_ = ArrowStyle::DefaultArrow;
    int cornerRadius_ = 0;
    int fontSize_ = 14;
    QString currentFontFamily_;
    bool bold_ = false;
    bool italic_ = false;
    bool underline_ = false;
    int textAlignment_ = -1;
    bool textOutlineEnabled_ = true;
    bool filled_ = false;
    bool textBackgroundEnabled_ = false;
    QColor textBackgroundColor_{0, 0, 0, 80};
    bool mosaicBlurred_ = false;
    bool pickingColor_ = false;
    double cropAspectRatio_ = 0.0;
    bool gridEnabled_ = false;
    double zoomFactor_ = 1.0;

    // Drawing
    bool drawing_ = false;
    Annotation draft_;
    QPoint start_;
    QPoint current_;
    int nextNumber_ = 1;

    // Move/resize
    bool moving_ = false;
    bool resizing_ = false;
    int resizeCorner_ = 0;
    QRect resizeStartBounds_;
    QVector<QPoint> resizeStartPoints_;
    QPoint moveOffset_;

    // Pan
    bool panning_ = false;
    QPoint panStart_;

    // Mouse info
    QPointF mouseImagePos_;
    QColor mousePixelColor_;

    // Recent
    QVector<AnnotationTool> recentTools_;
    QVector<QColor> customColors_;
};

} // namespace snappaste
