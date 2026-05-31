#pragma once

#include "domain/editor/Annotation.h"
#include "presentation/editor/AnnotationRenderer.h"

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QVector>
#include <QWidget>

#include <functional>

class QPainter;
class QScrollArea;

namespace snappaste {

class AnnotationCanvas final : public QWidget {
    Q_OBJECT

public:
    explicit AnnotationCanvas(QWidget* parent = nullptr);

    QPoint toImage(QPoint widgetPt) const;
    void setImage(QImage image);
    void applyCrop(QRect cropRect);
    void clearModified();
    bool isModified() const;
    void markModified();
    void zoomAt(double factor, QPoint center);
    void updateWindowTitle();
    QImage renderedImage() const;
    void setTool(AnnotationTool tool);
    void setColor(const QColor& color);
    void setStrokeWidth(int width);
    void setPickingColor(bool picking);
    void setOnPickingColorChanged(std::function<void(bool)> cb);
    void setMosaicBlurred(bool blurred);
    void setTextOutlineEnabled(bool enabled);
    void setFilled(bool filled);
    void updateTextBounds(int index);
    int fontSize() const;
    void setFontSize(int size);
    void setOnFontSizeChanged(std::function<void(int)> cb);
    double zoomFactor() const;
    QSize imageSize() const;
    QColor color() const;
    int strokeWidth() const;
    void setOnZoomChanged(std::function<void(double)> cb);
    const QVector<QColor>& recentColors() const;
    void addRecentColor(const QColor& color);
    void undo();
    void pushUndo();
    void redo();

    int strokeAlpha() const;
    void setStrokeAlpha(int alpha);
    void setOnStrokeAlphaChanged(std::function<void(int)> cb);

    ArrowStyle arrowStyle() const;
    void setArrowStyle(ArrowStyle style);
    void setOnArrowStyleChanged(std::function<void(int)> cb);

    int cornerRadius() const;
    void setCornerRadius(int radius);
    void setOnCornerRadiusChanged(std::function<void(int)> cb);

    bool gridEnabled() const;
    void setGridEnabled(bool enabled);

    QPointF mouseImagePos() const;
    QColor mousePixelColor() const;
    void setOnMouseInfoChanged(std::function<void(QPointF, QColor)> cb);
    void setOnModified(std::function<void()> cb);

    const QVector<AnnotationTool>& recentTools() const;

    int undoCount() const;
    int redoCount() const;

signals:
    void imageEdited(const QImage& image);
    void toolChanged(AnnotationTool tool);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void inputMethodEvent(QInputMethodEvent* event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

private:
    void handlePanningPress(QMouseEvent* event);
    bool handlePickingColorPress(QMouseEvent* event);
    bool handleSelectPress(const QPoint& pos);
    bool handleEraserPress(const QPoint& pos);
    bool handleNumberedPress(const QPoint& pos);
    bool handleTextPress(const QPoint& pos);
    bool handleExistingAnnotationPress(const QPoint& pos);
    void startDrawingAnnotation(const QPoint& pos);

    void updateMouseInfo(QMouseEvent* event);
    void updateMoveCursor(QMouseEvent* event);
    void handleMovePan(QMouseEvent* event);
    void handleMoveSelect(QMouseEvent* event);
    void updateDrawingStroke(QMouseEvent* event);

    bool handleTextEditingKey(QKeyEvent* event);
    void handleZoomFit();
    void handleAnnotationDeleteKey();
    void handleDuplicateKey();
    void handleLayerReorderKey(int direction);
    void handleNudgeKey(int key);
    void handleFontSizeChange(int delta);

    static constexpr int kMaxUndo = 50;

    AnnotationRenderer renderer_;
    QImage image_;
    QVector<Annotation> annotations_;
    QVector<QVector<Annotation>> undoStack_;
    QVector<QVector<Annotation>> redoStack_;
    AnnotationTool currentTool_ = AnnotationTool::Rectangle;
    QColor currentColor_{"#ff3b30"};
    int currentStrokeWidth_ = 4;
    int selectedIndex_ = -1;
    int editingTextIndex_ = -1;
    QString preeditString_;
    bool moving_ = false;
    bool resizing_ = false;
    int resizeCorner_ = 0;
    QRect resizeStartBounds_;
    QVector<QPoint> resizeStartPoints_;
    QPoint moveOffset_;
    Annotation draft_;
    QPoint start_;
    QPoint current_;
    bool drawing_ = false;
    bool pickingColor_ = false;
    bool mosaicBlurred_ = false;
    bool panning_ = false;
    QPoint panStart_;
    double zoomFactor_ = 1.0;
    bool modified_ = false;
    int nextNumber_ = 1;
    int fontSize_ = 14;
    bool filled_ = false;
    bool textOutlineEnabled_ = true;
    QVector<QColor> customColors_;
    int strokeAlpha_ = 255;
    ArrowStyle arrowStyle_ = ArrowStyle::DefaultArrow;
    int cornerRadius_ = 0;
    bool gridEnabled_ = false;
    QPointF mouseImagePos_;
    QColor mousePixelColor_;
    QVector<AnnotationTool> recentTools_;
    std::function<void(int)> onFontSizeChanged_;
    std::function<void(bool)> onPickingColorChanged_;
    std::function<void(double)> onZoomChanged_;
    std::function<void(int)> onStrokeAlphaChanged_;
    std::function<void(int)> onArrowStyleChanged_;
    std::function<void(int)> onCornerRadiusChanged_;
    std::function<void(QPointF, QColor)> onMouseInfoChanged_;
    std::function<void()> onModified_;
};

} // namespace snappaste
