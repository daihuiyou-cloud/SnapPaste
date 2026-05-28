#pragma once

#include "domain/editor/Annotation.h"

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
    const QVector<QColor>& recentColors() const;
    void addRecentColor(const QColor& color);
    void undo();
    void pushUndo();
    void redo();

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
    void drawAnnotations(QPainter* painter, const QImage& sourceImage, bool includeSelectionChrome) const;
    static bool hitTestAnnotation(const Annotation& annotation, const QPoint& pos);
    static void drawAnnotation(QPainter* painter, const QImage& sourceImage, const Annotation& annotation, int fontSize = 14);

    static constexpr int kMaxUndo = 50;

    QImage image_;
    mutable QImage annotationCache_;
    mutable bool cacheValid_ = false;
    QVector<Annotation> annotations_;
    QVector<QVector<Annotation>> undoStack_;
    QVector<QVector<Annotation>> redoStack_;
    AnnotationTool currentTool_ = AnnotationTool::Rectangle;
    QColor currentColor_{"#ff3b30"};
    int currentStrokeWidth_ = 3;
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
    std::function<void(int)> onFontSizeChanged_;
    std::function<void(bool)> onPickingColorChanged_;
};

} // namespace snappaste
