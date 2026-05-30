#pragma once

#include "domain/editor/Annotation.h"

#include <QImage>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QVector>

class QPainter;

namespace snappaste {

class AnnotationRenderer {
public:
    void drawAnnotations(QPainter& painter, const QImage& sourceImage,
                         const QVector<Annotation>& annotations,
                         int selectedIndex, int editingTextIndex,
                         const QString& preeditString,
                         int fontSize, double zoomFactor,
                         bool includeSelectionChrome);

    void drawDraft(QPainter& painter, const QImage& sourceImage,
                   const Annotation& draft, int fontSize);

    void drawCheckerboard(QPainter& painter, const QImage& sourceImage);

    void drawGridOverlay(QPainter& painter, const QRect& imageRect, double zoomFactor);

    void drawTextEditCursor(QPainter& painter, const QVector<Annotation>& annotations,
                            int editingTextIndex, const QString& preeditString,
                            int fontSize, double zoomFactor);

    void drawDraftSizeLabel(QPainter& painter, const QPoint& currentPos,
                            const Annotation& draft, bool drawing, double zoomFactor);

    void invalidateCache();
    const QImage& cacheImage() const;

    QImage renderToImage(const QImage& sourceImage,
                         const QVector<Annotation>& annotations,
                         int fontSize) const;

    static bool hitTestAnnotation(const Annotation& annotation, const QPoint& pos);

private:
    static void drawRectAnnotation(QPainter* painter, const Annotation& annotation);
    static void drawEllipseAnnotation(QPainter* painter, const Annotation& annotation);
    static void drawArrowAnnotation(QPainter* painter, const Annotation& annotation);
    static void drawLineAnnotation(QPainter* painter, const Annotation& annotation);
    static void drawPenAnnotation(QPainter* painter, const Annotation& annotation);
    static void drawTextAnnotation(QPainter* painter, const Annotation& annotation, int fontSize);
    static void drawMosaicAnnotation(QPainter* painter, const QImage& sourceImage, const Annotation& annotation);
    static void drawHighlightAnnotation(QPainter* painter, const Annotation& annotation);
    static void drawNumberedAnnotation(QPainter* painter, const Annotation& annotation);
    static void drawAnnotation(QPainter* painter, const QImage& sourceImage, const Annotation& annotation, int fontSize);

    mutable QImage annotationCache_;
    mutable bool cacheValid_ = false;
};

} // namespace snappaste
