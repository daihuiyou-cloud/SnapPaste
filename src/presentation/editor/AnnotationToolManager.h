#pragma once

#include "domain/editor/Annotation.h"
#include "presentation/editor/ToolSettings.h"

#include <QColor>
#include <QCursor>
#include <QFont>
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

    ToolSettings& settings() { return settings_; }
    const ToolSettings& settings() const { return settings_; }

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
    std::function<void(QCursor)> onSetCursor;
    std::function<void()> onWindowTitleUpdate;
    std::function<void(const QSize&)> onResizeCanvas;
    std::function<void(int, int)> onScrollBy;
    std::function<void()> onImageHistoryRestored;

    // --- Image (owned by Canvas, referenced here) ---
    void setImage(const QImage& image, double zoomFactor);
    void syncImageState(QImage image, QImage baseImage, int brightness, int contrast);
    void markImageChanged() { imageChangedSinceLastUndo_ = true; }
    const QImage& image() const { return image_; }
    const QImage& baseImage() const { return baseImage_; }
    void setImageDirect(QImage img) { image_ = std::move(img); }
    void setBaseImage(QImage img) { baseImage_ = std::move(img); }
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
    void setPreeditString(QString s) { preeditString_ = std::move(s); }

    // --- Tool state (forwarders to settings_) ---
    AnnotationTool currentTool() const { return settings_.currentTool; }
    void setTool(AnnotationTool tool);

    QColor color() const { return settings_.currentColor; }
    void setColor(const QColor& color);

    QColor fillColor() const { return settings_.currentFillColor; }
    void setFillColor(const QColor& color);

    int strokeWidth() const { return settings_.currentStrokeWidth; }
    void setStrokeWidth(int width);

    int strokeAlpha() const { return settings_.strokeAlpha; }
    void setStrokeAlpha(int alpha);

    ArrowStyle arrowStyle() const { return settings_.arrowStyle; }
    void setArrowStyle(ArrowStyle style);

    int cornerRadius() const { return settings_.cornerRadius; }
    void setCornerRadius(int radius);

    int fontSize() const { return settings_.fontSize; }
    void setFontSize(int size, bool persist = true);

    QString fontFamily() const { return settings_.currentFontFamily; }
    void setFontFamily(const QString& family);

    bool bold() const { return settings_.bold; }
    void setBold(bool b);

    bool italic() const { return settings_.italic; }
    void setItalic(bool i);

    bool underline() const { return settings_.underline; }
    void setUnderline(bool u);

    int textAlignment() const { return settings_.textAlignment; }
    void setTextAlignment(int align);

    bool textOutlineEnabled() const { return settings_.textOutlineEnabled; }
    void setTextOutlineEnabled(bool enabled);

    bool filled() const { return settings_.filled; }
    void setFilled(bool filled);

    bool textBackgroundEnabled() const { return settings_.textBackgroundEnabled; }
    void setTextBackgroundEnabled(bool enabled);

    QColor textBackgroundColor() const { return settings_.textBackgroundColor; }
    void setTextBackgroundColor(const QColor& color);

    bool mosaicBlurred() const { return settings_.mosaicBlurred; }
    void setMosaicBlurred(bool blurred);

    bool pickingColor() const { return settings_.pickingColor; }
    void setPickingColor(bool picking);

    double cropAspectRatio() const { return settings_.cropAspectRatio; }
    void setCropAspectRatio(double ratio);

    bool gridEnabled() const { return settings_.gridEnabled; }
    void setGridEnabled(bool enabled);

    double zoomFactor() const { return settings_.zoomFactor; }
    void setZoomFactor(double z) { settings_.zoomFactor = z; }

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

    // --- Recent tools / colors ---
    const QVector<AnnotationTool>& recentTools() const { return settings_.recentTools; }
    const QVector<QColor>& recentColors() const { return settings_.customColors; }
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
    ToolSettings settings_;

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
        bool hasImage = false;
    };
    QVector<ImageSnapshot> imageHistory_;
    QVector<ImageSnapshot> redoImageHistory_;

    static constexpr int kMaxUndo = 20;

    bool imageChangedSinceLastUndo_ = false;

    // Selection
    int selectedIndex_ = -1;
    int editingTextIndex_ = -1;
    int cursorPos_ = 0;
    QString preeditString_;

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
};

} // namespace snappaste
