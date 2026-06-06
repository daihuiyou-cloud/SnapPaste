#include "presentation/editor/AnnotationRenderer.h"
#include "presentation/editor/ImageBlur.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QStaticText>
#include <QTextOption>

#include <algorithm>
#include <cmath>

namespace snappaste {

namespace {
    const QColor kCheckerLight("#555555");
    const QColor kCheckerDark("#333333");
    const QColor kGridColor(255, 255, 255, 22);

    const QPixmap& checkerTile()
    {
        static const QPixmap tile = [] {
            QPixmap t(16, 16);
            QPainter p(&t);
            p.fillRect(0, 0, 8, 8, kCheckerLight);
            p.fillRect(8, 0, 8, 8, kCheckerDark);
            p.fillRect(0, 8, 8, 8, kCheckerDark);
            p.fillRect(8, 8, 8, 8, kCheckerLight);
            p.end();
            return t;
        }();
        return tile;
    }
}

void AnnotationRenderer::drawCheckerboard(QPainter& painter, const QImage& sourceImage)
{
    auto dpr = sourceImage.devicePixelRatio();
    auto logicalW = sourceImage.width() / dpr;
    auto logicalH = sourceImage.height() / dpr;
    painter.drawTiledPixmap(QRect(0, 0, logicalW, logicalH), checkerTile());
}

void AnnotationRenderer::drawGridOverlay(QPainter& painter, const QRect& imageRect, double zoomFactor)
{
    auto w = imageRect.width();
    auto h = imageRect.height();
    int step = static_cast<int>(50 * zoomFactor);
    if (step < 8) step = 8;
    painter.save();
    painter.setClipRect(0, 0, w, h);
    painter.setPen(QPen(kGridColor, 1));
    for (int x = step; x < w; x += step)
        painter.drawLine(x, 0, x, h);
    for (int y = step; y < h; y += step)
        painter.drawLine(0, y, w, y);
    painter.restore();
}

void AnnotationRenderer::drawTextEditCursor(QPainter& painter,
    const QVector<Annotation>& annotations,
    int editingTextIndex, int cursorPos, const QString& preeditString,
    int fontSize, double zoomFactor)
{
    painter.save();
    painter.scale(zoomFactor, zoomFactor);
    if (editingTextIndex >= 0 && editingTextIndex < annotations.size()) {
        const auto& a = annotations[editingTextIndex];
        if (a.tool == AnnotationTool::Text) {
            QFont font(a.fontFamily.isEmpty() ? qApp->font().family() : a.fontFamily);
            font.setPixelSize(a.textFontSize > 0 ? a.textFontSize : fontSize);
            font.setBold(a.bold);
            font.setItalic(a.italic);
            font.setUnderline(a.underline);
            painter.setFont(font);
            int cursorPosClamped = qBound(0, cursorPos, a.text.length());
            QString textBeforeCursor = a.text.left(cursorPosClamped);
            int cx = a.bounds.left() + 4 + painter.fontMetrics().horizontalAdvance(textBeforeCursor);
            int cy = a.bounds.top() + 4;
            int ch = painter.fontMetrics().height();
            if (!preeditString.isEmpty()) {
                painter.setPen(QPen(a.color, 1));
                int preeditWidth = painter.fontMetrics().horizontalAdvance(preeditString);
                painter.drawText(cx, cy, preeditWidth + 4, ch,
                    Qt::AlignLeft | Qt::AlignTop, preeditString);
                painter.setPen(QPen(a.color, 1, Qt::DashLine));
                painter.drawLine(cx, cy + ch + 1, cx + preeditWidth, cy + ch + 1);
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
    if (!drawing || draft.tool == AnnotationTool::Pen || draft.tool == AnnotationTool::Numbered) {
        return;
    }
    auto dims = draft.bounds.size();
    QString label = QStringLiteral("%1 \u00D7 %2").arg(dims.width(), dims.height());
    painter.save();
    painter.setPen(Qt::NoPen);
    auto textRect = painter.fontMetrics().boundingRect(label);
    auto labelPos = QPoint(static_cast<int>(currentPos.x() * zoomFactor),
                           static_cast<int>(currentPos.y() * zoomFactor));
    labelPos += QPoint(12, -textRect.height() - 8);
    textRect = QRect(labelPos.x() - 4, labelPos.y() - 2,
                     textRect.width() + 8, textRect.height() + 4);
    painter.setBrush(QColor(0, 0, 0, 160));
    painter.drawRoundedRect(textRect, 3, 3);
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, label);
    painter.restore();
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

    if (annotationCache_.size() != sourceImage.size() ||
        annotationCache_.format() != QImage::Format_ARGB32_Premultiplied) {
        annotationCache_ = QImage(sourceImage.size(), QImage::Format_ARGB32_Premultiplied);
    }
    annotationCache_.setDevicePixelRatio(sourceImage.devicePixelRatio());
    annotationCache_.fill(Qt::transparent);
    QPainter cachePainter(&annotationCache_);
    cachePainter.setRenderHint(QPainter::Antialiasing, true);
    cachePainter.setRenderHint(QPainter::TextAntialiasing, true);

    for (int i = 0; i < annotations.size(); ++i) {
        if (!annotations.at(i).visible) continue;
        drawAnnotation(&cachePainter, sourceImage, annotations.at(i), fontSize);
    }
    cachePainter.end();
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
    mosaicCachedRadius_ = -1;
    mosaicCachedBounds_ = QRect();
    mosaicThumbCache_ = {};
    mosaicSourceKey_ = -1;
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
        if (!a.visible) continue;
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
    QColor fill = annotation.fillColor.isValid() && annotation.filled ? annotation.fillColor
                 : annotation.filled ? annotation.color : Qt::transparent;
    painter->setBrush(fill);
    if (annotation.cornerRadius > 0)
        painter->drawRoundedRect(annotation.bounds, annotation.cornerRadius, annotation.cornerRadius);
    else
        painter->drawRect(annotation.bounds);
}

void AnnotationRenderer::drawEllipseAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QColor fill = annotation.fillColor.isValid() && annotation.filled ? annotation.fillColor
                 : annotation.filled ? annotation.color : Qt::transparent;
    painter->setBrush(fill);
    painter->drawEllipse(annotation.bounds);
}

void AnnotationRenderer::drawArrowAnnotation(QPainter* painter, const Annotation& annotation)
{
    const auto from = annotation.points.size() >= 2 ? annotation.points.first() : annotation.bounds.topLeft();
    const auto to = annotation.points.size() >= 2 ? annotation.points.last() : annotation.bounds.bottomRight();
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(from, to);
    const double arrowSize = 6.0 + annotation.strokeWidth * 2.0;
    const auto angle = std::atan2(to.y() - from.y(), to.x() - from.x());
    QColor headFill = annotation.fillColor.isValid() && annotation.filled ? annotation.fillColor
                     : annotation.filled ? annotation.color : annotation.color;
    painter->setBrush(headFill);
    painter->setPen(Qt::NoPen);
    if (annotation.arrowStyle == ArrowStyle::CircleArrow) {
        auto cx = to.x() - arrowSize * 0.5 * std::cos(angle);
        auto cy = to.y() - arrowSize * 0.5 * std::sin(angle);
        painter->drawEllipse(QPointF(cx, cy), arrowSize * 0.5, arrowSize * 0.5);
    } else if (annotation.arrowStyle == ArrowStyle::SquareArrow) {
        auto cx = to.x() - arrowSize * 0.5 * std::cos(angle);
        auto cy = to.y() - arrowSize * 0.5 * std::sin(angle);
        painter->drawRect(QRectF(cx - arrowSize * 0.4, cy - arrowSize * 0.4,
                                 arrowSize * 0.8, arrowSize * 0.8));
    } else {
        const auto p1 = QPointF(to.x() - arrowSize * std::cos(angle - M_PI / 6),
                                 to.y() - arrowSize * std::sin(angle - M_PI / 6));
        const auto p2 = QPointF(to.x() - arrowSize * std::cos(angle + M_PI / 6),
                                 to.y() - arrowSize * std::sin(angle + M_PI / 6));
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
    if (annotation.points.size() < 2) return;
    painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QPainterPath path;
    path.moveTo(annotation.points.first());
    for (int i = 1; i < annotation.points.size(); ++i)
        path.lineTo(annotation.points.at(i));
    painter->drawPath(path);
}

void AnnotationRenderer::drawTextAnnotation(QPainter* painter, const Annotation& annotation, int fontSize) const
{
    QString family = annotation.fontFamily.isEmpty() ? qApp->font().family() : annotation.fontFamily;
    int size = annotation.textFontSize > 0 ? annotation.textFontSize : fontSize;
    if (textFontCache_.fontFamily != family || textFontCache_.fontSize != size ||
        textFontCache_.bold != annotation.bold || textFontCache_.italic != annotation.italic ||
        textFontCache_.underline != annotation.underline) {
        textFontCache_.fontFamily = family;
        textFontCache_.fontSize = size;
        textFontCache_.bold = annotation.bold;
        textFontCache_.italic = annotation.italic;
        textFontCache_.underline = annotation.underline;
        textFontCache_.font = QFont(family);
        textFontCache_.font.setPixelSize(size);
        textFontCache_.font.setBold(annotation.bold);
        textFontCache_.font.setItalic(annotation.italic);
        textFontCache_.font.setUnderline(annotation.underline);
    }
    painter->setFont(textFontCache_.font);
    int align = (annotation.textAlignment >= 0) ? annotation.textAlignment : (Qt::AlignLeft | Qt::AlignTop);
    const auto flags = align | Qt::TextWordWrap;

    if (annotation.textBackground && annotation.textBackgroundColor.isValid() && !annotation.text.isEmpty()) {
        auto textBounds = painter->fontMetrics().boundingRect(
            annotation.bounds, flags, annotation.text);
        textBounds = textBounds.intersected(annotation.bounds);
        if (!textBounds.isEmpty()) {
            painter->setBrush(annotation.textBackgroundColor);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(textBounds.adjusted(-2, -1, 2, 1), 3, 3);
        }
    }

    if (annotation.textOutline) {
        painter->save();
        painter->setPen(QColor(255, 255, 255, 200));
        QStaticText st(annotation.text);
        st.setTextFormat(Qt::PlainText);
        QTextOption textOpt(static_cast<Qt::Alignment>(align));
        textOpt.setWrapMode(QTextOption::WordWrap);
        st.setTextOption(textOpt);
        st.setPerformanceHint(QStaticText::AggressiveCaching);
        auto b = annotation.bounds;
        st.setTextWidth(b.width() - 2);
        painter->drawStaticText(b.topLeft() + QPoint(-1, -1), st);
        painter->drawStaticText(b.topLeft() + QPoint( 1, -1), st);
        painter->drawStaticText(b.topLeft() + QPoint(-1,  1), st);
        painter->drawStaticText(b.topLeft() + QPoint( 1,  1), st);
        painter->restore();
    }
    painter->setPen(QPen(annotation.color, 1));
    painter->drawText(annotation.bounds, flags, annotation.text);
}

void AnnotationRenderer::drawMosaicAnnotation(QPainter* painter, const QImage& sourceImage, const Annotation& annotation) const
{
    // --- Blurred mosaic ---
    if (annotation.blurRadius > 0) {
        QRect blurBounds;
        if (!annotation.points.isEmpty()) {
            const int blockSize = qMax(4, annotation.strokeWidth * 4);
            for (const auto& pt : annotation.points) {
                QRect r(pt.x() - blockSize / 2, pt.y() - blockSize / 2, blockSize, blockSize);
                blurBounds = blurBounds.isEmpty() ? r : blurBounds.united(r);
            }
        } else {
            blurBounds = annotation.bounds;
        }
        blurBounds = blurBounds.intersected(sourceImage.rect());
        if (blurBounds.isEmpty()) return;

        bool cacheStale = (mosaicCachedRadius_ != annotation.blurRadius ||
                           mosaicCachedBounds_ != blurBounds ||
                           mosaicSourceKey_ != sourceImage.cacheKey());
        if (cacheStale) {
            mosaicBlurCache_ = blurImageRegion(sourceImage, blurBounds, annotation.blurRadius);
            mosaicBlurCache_.setDevicePixelRatio(sourceImage.devicePixelRatio());
            mosaicCachedRadius_ = annotation.blurRadius;
            mosaicCachedBounds_ = blurBounds;
            mosaicSourceKey_ = sourceImage.cacheKey();
        }

        if (!annotation.points.isEmpty()) {
            const int blockSize = qMax(4, annotation.strokeWidth * 4);
            for (const auto& pt : annotation.points) {
                QRect clipped = QRect(pt.x() - blockSize / 2, pt.y() - blockSize / 2, blockSize, blockSize)
                    .intersected(sourceImage.rect());
                if (!clipped.isEmpty()) {
                    QPoint srcOffset(clipped.left() - blurBounds.left(), clipped.top() - blurBounds.top());
                    painter->drawImage(clipped.topLeft(), mosaicBlurCache_, QRect(srcOffset, clipped.size()));
                }
            }
        } else {
            painter->drawImage(blurBounds.topLeft(), mosaicBlurCache_);
        }
        return;
    }

    // --- Pixel-block mosaic ---
    // Pre-scale the entire source once, then blit from thumb per point.
    constexpr int kThumbFraction = 8;
    auto sourceKey = sourceImage.cacheKey();
    if (mosaicSourceKey_ != sourceKey || mosaicThumbCache_.isNull()) {
        mosaicThumbCache_ = sourceImage.scaled(
            qMax(1, sourceImage.width() / kThumbFraction),
            qMax(1, sourceImage.height() / kThumbFraction),
            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        mosaicThumbCache_.setDevicePixelRatio(sourceImage.devicePixelRatio());
        mosaicSourceKey_ = sourceKey;
    }

    auto blitThumb = [&](const QRect& clipped, int bw, int bh) {
        QRectF srcRect(
            clipped.left() / static_cast<double>(kThumbFraction),
            clipped.top() / static_cast<double>(kThumbFraction),
            bw, bh);
        painter->save();
        painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter->drawImage(QRectF(clipped.topLeft(), clipped.size()), mosaicThumbCache_, srcRect);
        painter->restore();
    };

    if (!annotation.points.isEmpty()) {
        const int blockSize = qMax(4, annotation.strokeWidth * 4);
        for (const auto& pt : annotation.points) {
            QRect clipped = QRect(pt.x() - blockSize / 2, pt.y() - blockSize / 2, blockSize, blockSize)
                .intersected(sourceImage.rect());
            if (clipped.isEmpty()) continue;
            int bw = qMax(1, clipped.width() / kThumbFraction);
            int bh = qMax(1, clipped.height() / kThumbFraction);
            blitThumb(clipped, bw, bh);
        }
    } else {
        auto clipped = annotation.bounds.intersected(sourceImage.rect());
        if (clipped.isEmpty()) return;
        int bw = qMax(1, clipped.width() / kThumbFraction);
        int bh = qMax(1, clipped.height() / kThumbFraction);
        blitThumb(clipped, bw, bh);
    }
}

void AnnotationRenderer::drawHighlightAnnotation(QPainter* painter, const Annotation& annotation)
{
    QColor c = annotation.color;
    c.setAlpha(c.alpha() / 2);
    painter->fillRect(annotation.bounds, c);
}

void AnnotationRenderer::drawNumberedAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    const auto center = annotation.bounds.center();
    const auto r = kDefaultNumberedSize / 2;
    painter->setPen(QPen(annotation.color, 2));
    painter->setBrush(annotation.color);
    painter->drawEllipse(center, r, r);
    QFont numFont;
    if (!annotation.fontFamily.isEmpty())
        numFont.setFamily(annotation.fontFamily);
    numFont.setPixelSize(r);
    numFont.setBold(annotation.bold);
    numFont.setItalic(annotation.italic);
    painter->setFont(numFont);
    painter->setPen(Qt::white);
    painter->drawText(QRect(center.x() - r, center.y() - r, kDefaultNumberedSize, kDefaultNumberedSize),
                      Qt::AlignCenter, QString::number(annotation.number));
}

void AnnotationRenderer::drawCropAnnotation(QPainter* painter, const Annotation& annotation)
{
    painter->setPen(QPen(QColor("#2fbf9f"), 2, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(annotation.bounds);
}

void AnnotationRenderer::drawAnnotation(QPainter* painter, const QImage& sourceImage,
    const Annotation& annotation, int fontSize) const
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
    case AnnotationTool::Crop:
        drawCropAnnotation(painter, annotation);
        break;
    case AnnotationTool::Select:
    case AnnotationTool::Eraser:
        break;
    }
}

} // namespace snappaste