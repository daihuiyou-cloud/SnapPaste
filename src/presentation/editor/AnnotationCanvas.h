#pragma once

#include "domain/editor/Annotation.h"
#include "presentation/editor/AnnotationRenderer.h"
#include "presentation/editor/AnnotationToolManager.h"
#include "presentation/editor/AnnotationEventHandler.h"

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QVector>
#include <QScrollBar>
#include <QWidget>

#include <functional>

class QPainter;
class QPixmap;
class QScrollArea;

namespace snappaste {

class AnnotationCanvas final : public QWidget {
    Q_OBJECT

public:
    explicit AnnotationCanvas(QWidget* parent = nullptr);

    QPoint toImage(QPoint widgetPt) const;
    void setImage(QImage image);
    void adjustImage(int brightness, int contrast);
    void beginImageAdjust();
    void previewAdjustImage(int brightness, int contrast);
    const QImage& image() const { return image_; }
    void rotateImage(int degrees);
    void flipImage(bool horizontal, bool vertical);
    void applyCrop(QRect cropRect);
    void clearModified();
    bool isModified() const;
    void markModified();
    void zoomAt(double factor, QPoint center);
    void zoomFit();
    void updateWindowTitle();
    QImage renderedImage() const;

    // --- Delegated to ToolManager ---
    void setTool(AnnotationTool tool);
    void setColor(const QColor& color);
    void setStrokeWidth(int width);
    void updateBrushCursor();
    void setPickingColor(bool picking);
    void setMosaicBlurred(bool blurred);
    void setTextOutlineEnabled(bool enabled);
    void setFilled(bool filled);
    QColor fillColor() const;
    void setFillColor(const QColor& color);
    void setTextBackgroundEnabled(bool enabled);
    bool textBackgroundEnabled() const;
    void setTextBackgroundColor(const QColor& color);
    QColor textBackgroundColor() const;
    void updateTextBounds(int index);
    int fontSize() const;
    void setFontSize(int size, bool persist = true);
    int selectedIndex() const;
    const Annotation& annotationAt(int index) const { return toolManager_.annotationAt(index); }

    QString fontFamily() const;
    void setFontFamily(const QString& family);
    bool bold() const;
    void setBold(bool b);
    bool italic() const;
    void setItalic(bool i);
    bool underline() const;
    void setUnderline(bool u);
    int textAlignment() const;
    void setTextAlignment(int align);
    void syncTextPropertiesUI();

    double cropAspectRatio() const;
    void setCropAspectRatio(double ratio);

    double zoomFactor() const;
    QSize imageSize() const;
    const QImage& baseImage() const { return baseImage_; }
    int brightness() const { return brightness_; }
    int contrast() const { return contrast_; }
    QColor color() const;
    int strokeWidth() const;
    const QVector<QColor>& recentColors() const;
    void addRecentColor(const QColor& color);
    void undo();
    void pushUndo();
    void redo();

    int strokeAlpha() const;
    void setStrokeAlpha(int alpha);

    ArrowStyle arrowStyle() const;
    void setArrowStyle(ArrowStyle style);

    int cornerRadius() const;
    void setCornerRadius(int radius);

    bool gridEnabled() const;
    void setGridEnabled(bool enabled);
    bool filled() const { return toolManager_.filled(); }
    bool textOutlineEnabled() const { return toolManager_.textOutlineEnabled(); }
    bool mosaicBlurred() const { return toolManager_.mosaicBlurred(); }

    QPointF mouseImagePos() const;
    QColor mousePixelColor() const;

    const QVector<AnnotationTool>& recentTools() const;

    int undoCount() const;
    int redoCount() const;
    int annotationCount() const { return toolManager_.annotationCount(); }
    void selectAnnotation(int index);
    void deleteAnnotation(int index);
    void duplicateAnnotation(int index);
    void swapAnnotations(int i, int j);
    void setAnnotationVisible(int index, bool visible);

    AnnotationRenderer& renderer() { return renderer_; }
    AnnotationToolManager& toolManager() { return toolManager_; }

signals:
    void imageEdited(const QImage& image);
    void toolChanged(AnnotationTool tool);
    void fontSizeChanged(int size);
    void pickingColorChanged(bool picking);
    void zoomChanged(double factor);
    void strokeAlphaChanged(int alpha);
    void arrowStyleChanged(int style);
    void cornerRadiusChanged(int radius);
    void mouseInfoChanged(QPointF pos, QColor color);
    void textPropertiesChanged();
    void cropAspectRatioChanged(double ratio);
    void modified();
    void annotationsChanged();
    void selectionChanged();

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
    void reapplyAdjustments();
    void rebuildBackingCache();
    void wireCallbacks();

    AnnotationRenderer renderer_;
    AnnotationToolManager toolManager_;
    AnnotationEventHandler eventHandler_;

    QImage image_;
    QImage baseImage_;
    int brightness_ = 0;
    int contrast_ = 0;
    bool modified_ = false;

    QPixmap backingCache_;
    double backingZoom_ = 0.0;
    bool backingCacheDirty_ = true;
};

} // namespace snappaste
