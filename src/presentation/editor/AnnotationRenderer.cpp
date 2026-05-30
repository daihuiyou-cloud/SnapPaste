#define _USE_MATH_DEFINES
#include "presentation/editor/AnnotationRenderer.h"
#include "presentation/editor/ImageBlur.h"

#include <QPainter>

#include <algorithm>
#include <cmath>

namespace snappaste {

void AnnotationRenderer::drawCheckerboard(QPainter& painter, const QImage& sourceImage)
{
    int tile = 8;
    auto dpr = sourceImage.devicePixelRatio();
    auto logicalH = sourceImage.height() / dpr;
    auto logicalW = sourceImage.width() / dpr;
    for (int y = 0; y < logicalH; y += tile) {
        for (int x = 0; x < logicalW; x += tile) {
            bool light = ((x / tile) + (y / tile)) % 2 == 0;
            painter.fillRect(x, y, tile, tile, light ? QColor("#cccccc") : QColor("#888888"));
        }
    }
}

void AnnotationRenderer::drawGridOverlay(QPainter& painter, const QRect& imageRect, double zoomFactor)
{
    Q_UNUSED(imageRect)
    auto w = imageRect.width();
    auto h = imageRect.height();
    int step = static_cast<int>(50 * zoomFactor);
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

void AnnotationRenderer::drawTextEditCursor(QPainter& painter,
    const QVector<Annotation>& annotations,
    int editingTextIndex, const QString& preeditString,
    int fontSize, double zoomFactor)
{
    painter.save();
    painter.scale(zoomFactor, zoomFactor);
    if (editingTextIndex >= 0 && editingTextIndex < annotations.size()) {
        const auto& a = annotations[editingTextIndex];
        if (a.tool == AnnotationTool::Text) {
            QFont font("Microsoft YaHei UI", a.textFontSize > 0 ? a.textFontSize : fontSize);
            painter.setFont(font);
            int textWidth = painter.fontMetrics().horizontalAdvance(a.text);
            int cx = a.bounds.left() + 4 + textWidth;
            int cy = a.bounds.top() + 4;
            int ch = painter.fontMetrics().height();
            if (!preeditString.isEmpty()) {
                painter.setPen(QPen(a.color, 1));
                painter.drawText(cx, cy, painter.fontMetrics().horizontalAdvance(preeditString) + 4, ch,
                    Qt::AlignLeft | Qt::AlignTop, preeditString);
                int preeditWidth = painter.fontMetrics().horizontalAdvance(preeditString);
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

void AnnotationRenderer::drawDraftSizeLabel(QPainter& painter,
    const QPoint& currentPos, const Annotation& draft,
    bool drawing, double zoomFactor)
{
    if (!drawing || draft.tool == AnnotationTool::Pen || draft.tool == AnnotationTool::Numbered || draft.tool == AnnotationTool::Crop) {
        return;
    }
    auto dims = draft.bounds.size();
    QString label = QStringLiteral("%1 \u00D7 %2").arg(dims.width()).arg(dims.height());
    painter.setPen(Qt::NoPen);
    auto textRect = painter.fontMetrics().boundingRect(label);
    auto labelPos = currentPos;
    labelPos = QPoint(static_cast<int>(labelPos.x() * zoomFactor),
                      static_cast<int>(labelPos.y() * zoomFactor));
    labelPos += QPoint(12, -textRect.height() - 8);
    textRect = QRect(labelPos.x() - 4, labelPos.y() - 2,
                     textRect.width() + 8, textRect.height() + 4);
    painter.setBrush(QColor(0, 0, 0, 160));
    painter.drawRoundedRect(textRect, 3, 3);
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, label);
}

void AnnotationRenderer::drawAnnotations(QPainter& painter,
    const QImage& sourceImage,
    const QVector<Annotation>& annotations,
    int fontSize)
{
    if (cacheValid_) {
        painter.drawImage(QPoint(0, 0), annotationCache_);
        return;
    }

    QImage cache(sourceImage.size(), QImage::Format_ARGB32_Premultiplied);
    cache.setDevicePixelRatio(sourceImage.devicePixelRatio());
    cache.fill(Qt::transparent);
    QPainter cachePainter(&cache);
    cachePainter.setRenderHint(QPainter::Antialiasing, true);

    for (int i = 0; i < annotations.size(); ++i) {
        drawAnnotation(&cachePainter, sourceImage, annotations.at(i), fontSize);
    }
    cachePainter.end();

    annotationCache_ = cache;
    cacheValid_ = true;
    painter.drawImage(QPoint(0, 0), annotationCache_);
}

void AnnotationRenderer::drawDraft(QPainter& painter,
    const QImage& sourceImage, const Annotation& draft, int fontSize)
{
    drawAnnotation(&painter, sourceImage, draft, fontSize);
}

void AnnotationRenderer::invalidateCache()
{
    cacheValid_ = false;
}

const QImage& AnnotationRenderer::cacheImage() const
{
    return annotationCache_;
}

QImage AnnotationRenderer::renderToImage(const QImage& sourceImage,
    const QVector<Annotation>& annotations, int fontSize) const
{
    QImage result = sourceImage.copy();
    QPainter painter(&result);
    for (const auto& a : annotations) {
        drawAnnotation(&painter, sourceImage, a, fontSize);
    }
    painter.end();
    return result;
}

bool AnnotationRenderer::hitTestAnnotation(const Annotation& annotation, const QPoint& pos)
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

void AnnotationRenderer::drawRectAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(annotation.filled ? annotation.color : Qt::NoBrush);
    if (annotation.cornerRadius > 0)
        painter->drawRoundedRect(annotation.bounds, annotation.cornerRadius, annotation.cornerRadius);
    else
        painter->drawRect(annotation.bounds);
}

void AnnotationRenderer::drawEllipseAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(annotation.filled ? annotation.color : Qt::NoBrush);
    painter->drawEllipse(annotation.bounds);
}

void AnnotationRenderer::drawArrowAnnotation(QPainter* painter, const Annotation& annotation)
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

void AnnotationRenderer::drawLineAnnotation(QPainter* painter, const Annotation& annotation)
{
    const auto from = annotation.points.size() >= 2 ? annotation.points.first() : annotation.bounds.topLeft();
    const auto to = annotation.points.size() >= 2 ? annotation.points.last() : annotation.bounds.bottomRight();
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(from, to);
}

void AnnotationRenderer::drawPenAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for (int i = 1; i < annotation.points.size(); ++i) {
        painter->drawLine(annotation.points.at(i - 1), annotation.points.at(i));
    }
}

void AnnotationRenderer::drawTextAnnotation(QPainter* painter, const Annotation& annotation, int fontSize)
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

void AnnotationRenderer::drawMosaicAnnotation(QPainter* painter, const QImage& sourceImage, const Annotation& annotation)
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

void AnnotationRenderer::drawHighlightAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->fillRect(annotation.bounds, QColor(annotation.color.red(), annotation.color.green(), annotation.color.blue(), 100));
}

void AnnotationRenderer::drawNumberedAnnotation(QPainter* painter, const Annotation& annotation)
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

void AnnotationRenderer::drawAnnotation(QPainter* painter, const QImage& sourceImage,
    const Annotation& annotation, int fontSize)
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
