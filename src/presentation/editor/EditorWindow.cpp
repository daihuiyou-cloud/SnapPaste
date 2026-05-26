#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>

#include "presentation/editor/EditorWindow.h"

#include "presentation/icons/IconProvider.h"

#include <QInputDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QToolBar>
#include <QToolButton>

namespace snappaste {

namespace {

QIcon makeEllipseIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#bcbec6"), 2));
    p.drawEllipse(QRectF(3, 3, 14, 14));
    p.end();
    return QIcon(pix);
}

QIcon makeHighlightIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(QRectF(3, 6, 14, 8), QColor(255, 230, 0, 140));
    p.setPen(QPen(QColor("#bcbec6"), 1));
    p.drawRect(QRectF(3, 6, 14, 8));
    p.end();
    return QIcon(pix);
}

QIcon makeSelectIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#bcbec6"), 1.5));
    p.drawRect(QRectF(3, 3, 14, 14));
    p.drawRect(QRectF(5, 5, 10, 10));
    // corner handles
    constexpr QPointF handles[] = {{3,3},{17,3},{3,17},{17,17}};
    p.setBrush(QColor("#bcbec6"));
    for (auto& pt : handles) p.drawRect(QRectF(pt.x()-2, pt.y()-2, 4, 4));
    p.end();
    return QIcon(pix);
}

QIcon makeEraserIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#ff3b30"), 2));
    p.drawLine(4, 4, 16, 16);
    p.drawLine(16, 4, 4, 16);
    p.end();
    return QIcon(pix);
}

QIcon makeNumberedIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#bcbec6"), 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(2, 2, 16, 16));
    p.setPen(QColor("#bcbec6"));
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.drawText(QRectF(2, 2, 16, 16), Qt::AlignCenter, "1");
    p.end();
    return QIcon(pix);
}

QIcon makeEyedropperIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#bcbec6"), 2, Qt::SolidLine, Qt::RoundCap));
    p.drawEllipse(QPointF(6, 16), 3, 3);
    p.drawLine(7, 15, 15, 4);
    p.setBrush(QColor("#bcbec6"));
    p.drawRect(QRectF(12, 1, 6, 5));
    p.end();
    return QIcon(pix);
}

QIcon makeColorIcon(const QColor& color)
{
    QPixmap pix(16, 16);
    pix.fill(color);
    QPainter p(&pix);
    p.setPen(QPen(QColor(255, 255, 255, 48), 1));
    p.drawRect(QRectF(0.5, 0.5, 15, 15));
    p.end();
    return QIcon(pix);
}

static void blurHorizontal(const QImage& src, QImage& dst, int radius)
{
    const int w = src.width(), h = src.height();
    for (int y = 0; y < h; ++y) {
        const auto* in = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        auto* out = reinterpret_cast<QRgb*>(dst.scanLine(y));
        int a = 0, r = 0, g = 0, b = 0, cnt = 0;
        for (int x = 0; x <= radius && x < w; ++x) {
            auto px = in[x]; a += qAlpha(px); r += qRed(px); g += qGreen(px); b += qBlue(px); ++cnt;
        }
        for (int x = 0; x < w; ++x) {
            if (cnt > 0) out[x] = qRgba(r / cnt, g / cnt, b / cnt, a / cnt);
            int left = x - radius;
            if (left >= 0) { auto px = in[left]; a -= qAlpha(px); r -= qRed(px); g -= qGreen(px); b -= qBlue(px); --cnt; }
            int right = x + radius + 1;
            if (right < w) { auto px = in[right]; a += qAlpha(px); r += qRed(px); g += qGreen(px); b += qBlue(px); ++cnt; }
        }
    }
}

static void blurVertical(const QImage& src, QImage& dst, int radius)
{
    const int w = src.width(), h = src.height();
    for (int x = 0; x < w; ++x) {
        int a = 0, r = 0, g = 0, b = 0, cnt = 0;
        for (int y = 0; y <= radius && y < h; ++y) {
            auto px = src.pixel(x, y);
            a += qAlpha(px); r += qRed(px); g += qGreen(px); b += qBlue(px); ++cnt;
        }
        for (int y = 0; y < h; ++y) {
            if (cnt > 0) dst.setPixel(x, y, qRgba(r / cnt, g / cnt, b / cnt, a / cnt));
            int top = y - radius;
            if (top >= 0) { auto px = src.pixel(x, top); a -= qAlpha(px); r -= qRed(px); g -= qGreen(px); b -= qBlue(px); --cnt; }
            int bottom = y + radius + 1;
            if (bottom < h) { auto px = src.pixel(x, bottom); a += qAlpha(px); r += qRed(px); g += qGreen(px); b += qBlue(px); ++cnt; }
        }
    }
}

static QImage blurImage(QImage source, int radius)
{
    if (radius <= 0 || source.isNull()) return source;
    source = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QImage tmp(source.size(), QImage::Format_ARGB32_Premultiplied);
    for (int i = 0; i < 3; ++i) {
        blurHorizontal(source, tmp, radius);
        blurVertical(tmp, source, radius);
    }
    return source;
}

} // namespace

class AnnotationCanvas final : public QWidget {
public:
    explicit AnnotationCanvas(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        setMinimumSize(640, 360);
    }

    void setImage(QImage image)
    {
        image_ = std::move(image);
        annotations_.clear();
        undoStack_.clear();
        redoStack_.clear();
        setMinimumSize(image_.size());
        resize(image_.size());
        update();
    }

    QImage renderedImage() const
    {
        if (image_.isNull()) {
            return {};
        }

        QImage output = image_.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&output);
        drawAnnotations(&painter, image_, false);
        return output;
    }

    void setTool(AnnotationTool tool)
    {
        currentTool_ = tool;
        if (tool != AnnotationTool::Select) {
            selectedIndex_ = -1;
            update();
        }
    }

    void setColor(const QColor& color)
    {
        currentColor_ = color;
    }

    void setStrokeWidth(int width)
    {
        currentStrokeWidth_ = std::clamp(width, 1, 12);
    }

    void setPickingColor(bool picking)
    {
        pickingColor_ = picking;
        setCursor(picking ? Qt::CrossCursor : Qt::ArrowCursor);
        update();
    }

    void setMosaicBlurred(bool blurred)
    {
        mosaicBlurred_ = blurred;
    }

    void setTextOutlineEnabled(bool enabled)
    {
        textOutlineEnabled_ = enabled;
    }

    void undo()
    {
        if (undoStack_.isEmpty()) {
            return;
        }
        redoStack_.push_back(annotations_);
        annotations_ = undoStack_.takeLast();
        update();
    }

    void redo()
    {
        if (redoStack_.isEmpty()) {
            return;
        }
        undoStack_.push_back(annotations_);
        annotations_ = redoStack_.takeLast();
        update();
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (image_.isNull()) {
            return;
        }
        if (currentTool_ != AnnotationTool::Text) {
            return;
        }

        const auto pos = event->pos();

        for (int i = annotations_.size() - 1; i >= 0; --i) {
            if (annotations_[i].tool == AnnotationTool::Text && annotations_[i].bounds.contains(pos)) {
                bool ok = false;
                const auto newText = QInputDialog::getMultiLineText(
                    static_cast<QWidget*>(parent()), "Edit Text", "Edit text:", annotations_[i].text, &ok);
                if (ok && !newText.isEmpty() && newText != annotations_[i].text) {
                    undoStack_.push_back(annotations_);
                    redoStack_.clear();
                    annotations_[i].text = newText;
                    update();
                }
                return;
            }
        }

        bool ok = false;
        const auto text = QInputDialog::getMultiLineText(
            static_cast<QWidget*>(parent()), "Text Input", "Enter text:", QString(), &ok);
        if (!ok || text.isEmpty()) {
            return;
        }

        const auto clickPos = event->pos();
        QFont font("Segoe UI", 14);
        QFontMetrics fm(font);
        const auto textRect = fm.boundingRect(QRect(0, 0, 4096, 4096), Qt::AlignLeft | Qt::AlignTop, text);
        QRect bounds(clickPos.x(), clickPos.y(), qMax(textRect.width() + 8, 20), qMax(textRect.height() + 8, 20));
        if (bounds.right() > width()) {
            bounds.moveRight(width() - 4);
        }

        undoStack_.push_back(annotations_);
        redoStack_.clear();
        Annotation ann;
        ann.tool = AnnotationTool::Text;
        ann.bounds = bounds;
        ann.text = text;
        ann.color = currentColor_;
        ann.strokeWidth = 2;
        ann.textOutline = textOutlineEnabled_;
        annotations_.push_back(std::move(ann));
        update();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (image_.isNull()) {
            return;
        }

        setFocus();

        if (event->button() != Qt::LeftButton) {
            return;
        }

        const auto pos = event->pos();

        if (pickingColor_) {
            pickingColor_ = false;
            setCursor(Qt::ArrowCursor);
            if (image_.rect().contains(pos)) {
                QImage composited = image_.copy();
                QPainter p(&composited);
                drawAnnotations(&p, image_, false);
                p.end();
                currentColor_ = QColor::fromRgba(composited.pixel(pos));
            }
            update();
            return;
        }

        if (currentTool_ == AnnotationTool::Select) {
            if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
                const auto r = annotations_[selectedIndex_].bounds;
                const QPoint corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
                for (int ci = 0; ci < 4; ++ci) {
                    if (QRect(corners[ci].x() - 8, corners[ci].y() - 8, 16, 16).contains(pos)) {
                        undoStack_.push_back(annotations_);
                        redoStack_.clear();
                        resizing_ = true;
                        resizeCorner_ = ci;
                        resizeStartBounds_ = r;
                        resizeStartPoints_ = annotations_[selectedIndex_].points;
                        return;
                    }
                }
            }
            for (int i = annotations_.size() - 1; i >= 0; --i) {
                if (hitTestAnnotation(annotations_.at(i), pos)) {
                    undoStack_.push_back(annotations_);
                    redoStack_.clear();
                    selectedIndex_ = i;
                    moving_ = true;
                    moveOffset_ = annotations_[i].bounds.topLeft() - pos;
                    update();
                    return;
                }
            }
            selectedIndex_ = -1;
            update();
            return;
        }

        if (currentTool_ == AnnotationTool::Eraser) {
            for (int i = annotations_.size() - 1; i >= 0; --i) {
                if (hitTestAnnotation(annotations_.at(i), pos)) {
                    undoStack_.push_back(annotations_);
                    redoStack_.clear();
                    annotations_.removeAt(i);
                    update();
                    return;
                }
            }
            return;
        }

        if (currentTool_ == AnnotationTool::Numbered) {
            undoStack_.push_back(annotations_);
            redoStack_.clear();
            Annotation ann;
            ann.tool = AnnotationTool::Numbered;
            ann.bounds = QRect(pos.x() - kDefaultNumberedSize / 2, pos.y() - kDefaultNumberedSize / 2,
                               kDefaultNumberedSize, kDefaultNumberedSize);
            ann.color = currentColor_;
            ann.number = nextNumber_++;
            annotations_.push_back(std::move(ann));
            update();
            return;
        }

        drawing_ = true;
        start_ = event->pos();
        current_ = start_;
        draft_ = Annotation{};
        draft_.tool = currentTool_;
        draft_.color = currentColor_;
        draft_.strokeWidth = currentStrokeWidth_;
        draft_.blurRadius = (currentTool_ == AnnotationTool::Mosaic && mosaicBlurred_) ? 6 : 0;
        draft_.bounds = QRect(start_, current_);
        draft_.points = {start_};
        update();
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (currentTool_ == AnnotationTool::Select && selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
            if (resizing_) {
                auto& a = annotations_[selectedIndex_];
                auto b = resizeStartBounds_;
                const auto p = event->pos();
                switch (resizeCorner_) {
                case 0: b.setTopLeft(p); break;   // TL
                case 1: b.setTopRight(p); break;   // TR
                case 2: b.setBottomLeft(p); break; // BL
                case 3: b.setBottomRight(p); break;// BR
                }
                a.bounds = b.normalized();
                if (a.tool == AnnotationTool::Pen) {
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
                const auto newPos = event->pos() + moveOffset_;
                const auto delta = newPos - annotations_.at(selectedIndex_).bounds.topLeft();
                annotations_[selectedIndex_].bounds.translate(delta);
                if (annotations_[selectedIndex_].tool == AnnotationTool::Pen) {
                    for (auto& pt : annotations_[selectedIndex_].points) {
                        pt += delta;
                    }
                }
                moveOffset_ = annotations_[selectedIndex_].bounds.topLeft() - event->pos();
                update();
                return;
            }
        }

        if (!drawing_) {
            return;
        }

        if (draft_.tool == AnnotationTool::Numbered) {
            return;
        }

        current_ = event->pos();
        draft_.bounds = QRect(start_, current_).normalized();
        if (draft_.tool == AnnotationTool::Pen) {
            draft_.points.push_back(current_);
        }
        update();
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
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
        current_ = event->pos();
        draft_.bounds = QRect(start_, current_).normalized();
        if (draft_.tool == AnnotationTool::Arrow) {
            draft_.points = {start_, current_};
        }
        if (draft_.bounds.width() > 2 || draft_.bounds.height() > 2 || draft_.tool == AnnotationTool::Pen) {
            undoStack_.push_back(annotations_);
            redoStack_.clear();
            annotations_.push_back(draft_);
        }
        update();
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
            && selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
            undoStack_.push_back(annotations_);
            redoStack_.clear();
            annotations_.removeAt(selectedIndex_);
            selectedIndex_ = -1;
            update();
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)

        QPainter painter(this);
        painter.fillRect(rect(), QColor("#101418"));
        if (!image_.isNull()) {
            painter.drawImage(QPoint(0, 0), image_);
        }

        drawAnnotations(&painter, image_, true);
        if (drawing_) {
            drawAnnotation(&painter, image_, draft_);
        }
    }

private:
    void drawAnnotations(QPainter* painter, const QImage& sourceImage, bool includeSelectionChrome) const
    {
        for (int i = 0; i < annotations_.size(); ++i) {
            drawAnnotation(painter, sourceImage, annotations_.at(i));
            if (includeSelectionChrome && i == selectedIndex_) {
                painter->setPen(QPen(QColor("#2fbf9f"), 1, Qt::DashLine));
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(annotations_.at(i).bounds.adjusted(-3, -3, 3, 3));
                // corner handles
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor("#2fbf9f"));
                const auto r = annotations_.at(i).bounds;
                const QPoint corners[] = {r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
                for (const auto& c : corners) {
                    painter->drawRect(QRect(c.x() - 4, c.y() - 4, 8, 8));
                }
            }
        }
    }

    static bool hitTestAnnotation(const Annotation& annotation, const QPoint& pos)
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

    static void drawAnnotation(QPainter* painter, const QImage& sourceImage, const Annotation& annotation)
    {
        painter->setRenderHint(QPainter::Antialiasing, true);

        switch (annotation.tool) {
        case AnnotationTool::Rectangle:
            painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->drawRect(annotation.bounds);
            break;
        case AnnotationTool::Ellipse:
            painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->drawEllipse(annotation.bounds);
            break;
        case AnnotationTool::Arrow: {
            painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            const auto from = annotation.points.size() >= 2 ? annotation.points.first() : annotation.bounds.topLeft();
            const auto to = annotation.points.size() >= 2 ? annotation.points.last() : annotation.bounds.bottomRight();
            painter->drawLine(from, to);
            constexpr double kArrowSize = 12.0;
            const auto angle = std::atan2(to.y() - from.y(), to.x() - from.x());
            const auto p1 = QPointF(to.x() - kArrowSize * std::cos(angle - M_PI / 6),
                                     to.y() - kArrowSize * std::sin(angle - M_PI / 6));
            const auto p2 = QPointF(to.x() - kArrowSize * std::cos(angle + M_PI / 6),
                                     to.y() - kArrowSize * std::sin(angle + M_PI / 6));
            painter->drawLine(to, p1);
            painter->drawLine(to, p2);
            break;
        }
        case AnnotationTool::Pen:
            painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            for (int i = 1; i < annotation.points.size(); ++i) {
                painter->drawLine(annotation.points.at(i - 1), annotation.points.at(i));
            }
            break;
        case AnnotationTool::Text: {
            QFont font("Segoe UI", 14);
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
            break;
        }
        case AnnotationTool::Mosaic: {
            const auto clipped = annotation.bounds.intersected(sourceImage.rect());
            if (clipped.isEmpty()) {
                break;
            }
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
            break;
        }
        case AnnotationTool::Highlight:
            painter->fillRect(annotation.bounds, QColor(annotation.color.red(), annotation.color.green(), annotation.color.blue(), 100));
            break;
        case AnnotationTool::Numbered: {
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
            break;
        }
        case AnnotationTool::Select:
        case AnnotationTool::Eraser:
            break;
        }
    }

    QImage image_;
    QVector<Annotation> annotations_;
    QVector<QVector<Annotation>> undoStack_;
    QVector<QVector<Annotation>> redoStack_;
    AnnotationTool currentTool_ = AnnotationTool::Rectangle;
    QColor currentColor_{"#ff3b30"};
    int currentStrokeWidth_ = 3;
    int selectedIndex_ = -1;
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
    int nextNumber_ = 1;
    bool textOutlineEnabled_ = true;
};

EditorWindow::EditorWindow(QWidget* parent)
    : QMainWindow(parent)
    , canvas_(new AnnotationCanvas(this))
{
    setWindowTitle("SnapPaste Editor");
    resize(980, 680);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(canvas_);
    scrollArea->setAlignment(Qt::AlignCenter);
    setCentralWidget(scrollArea);

    createToolbar();

    auto* undoAction = new QAction("Undo", this);
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, [this] { canvas_->undo(); });
    addAction(undoAction);

    auto* redoAction = new QAction("Redo", this);
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, [this] { canvas_->redo(); });
    addAction(redoAction);
}

void EditorWindow::setImage(const QImage& image)
{
    canvas_->setImage(image);
    show();
    raise();
    activateWindow();
}

void EditorWindow::createToolbar()
{
    auto* toolbar = addToolBar("Editor");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(20, 20));

    auto* rectangle = toolbar->addAction(IconProvider::icon(IconName::Rectangle), "Rectangle");
    auto* ellipse = toolbar->addAction(makeEllipseIcon(), "Ellipse");
    auto* arrow = toolbar->addAction(IconProvider::icon(IconName::Arrow), "Arrow");
    auto* pen = toolbar->addAction(IconProvider::icon(IconName::Pen), "Pen");
    auto* textB = toolbar->addAction(IconProvider::icon(IconName::Text), "Text");
    auto* highlight = toolbar->addAction(makeHighlightIcon(), "Highlight");
    auto* numbered = toolbar->addAction(makeNumberedIcon(), "Numbered");
    auto* outlineBtn = new QToolButton(toolbar);
    outlineBtn->setText("O");
    outlineBtn->setToolTip("Toggle text outline");
    outlineBtn->setFixedSize(24, 24);
    outlineBtn->setCheckable(true);
    outlineBtn->setChecked(true);
    outlineBtn->setStyleSheet(
        "QToolButton { font: bold 10px; color: #bcbec6; }"
        "QToolButton:checked { color: #2fbf9f; }");
    connect(outlineBtn, &QToolButton::clicked, this, [this](bool checked) {
        canvas_->setTextOutlineEnabled(checked);
    });
    toolbar->addWidget(outlineBtn);
    auto* mosaic = toolbar->addAction(IconProvider::icon(IconName::Mosaic), "Mosaic");
    toolbar->addSeparator();

    auto* eraser = toolbar->addAction(makeEraserIcon(), "Eraser");

    toolbar->addSeparator();
    auto* select = toolbar->addAction(makeSelectIcon(), "Select");

    toolbar->addSeparator();

    struct StrokePreset { QString label; int width; };
    const StrokePreset strokes[] = {
        {"S", 2}, {"M", 4}, {"L", 8}
    };
    for (const auto& s : strokes) {
        auto* btn = new QToolButton(toolbar);
        btn->setText(s.label);
        btn->setToolTip(QString("Stroke width: %1px").arg(s.width));
        btn->setFixedSize(24, 24);
        btn->setCheckable(true);
        btn->setStyleSheet(
            "QToolButton { font: bold 10px; color: #bcbec6; }"
            "QToolButton:checked { color: #2fbf9f; }");
        if (s.width == 4) {
            btn->setChecked(true);
        }
        connect(btn, &QToolButton::clicked, this, [this, s, toolbar, btn] {
            canvas_->setStrokeWidth(s.width);
            for (auto* child : toolbar->children()) {
                if (auto* tb = qobject_cast<QToolButton*>(child)) {
                    tb->setChecked(tb == btn);
                }
            }
        });
        toolbar->addWidget(btn);
    }

    toolbar->addSeparator();

    const QColor colors[] = {
        QColor("#ff3b30"), QColor("#ff9500"), QColor("#ffcc00"),
        QColor("#34c759"), QColor("#007aff"), QColor("#af52de"),
        QColor("#ffffff"), QColor("#000000")
    };
    for (const auto& color : colors) {
        auto* btn = new QToolButton(toolbar);
        btn->setIcon(makeColorIcon(color));
        btn->setToolTip(color.name(QColor::HexRgb).toUpper());
        btn->setFixedSize(24, 24);
        connect(btn, &QToolButton::clicked, this, [this, color] {
            canvas_->setColor(color);
        });
        toolbar->addWidget(btn);
    }

    auto* eyedropper = toolbar->addAction(makeEyedropperIcon(), "Eyedropper");
    eyedropper->setToolTip("Pick color from image");

    toolbar->addSeparator();
    auto* copy = toolbar->addAction(IconProvider::icon(IconName::Copy), "Copy");
    auto* save = toolbar->addAction(IconProvider::icon(IconName::Save), "Save");

    rectangle->setToolTip("Rectangle");
    ellipse->setToolTip("Ellipse");
    arrow->setToolTip("Arrow");
    pen->setToolTip("Pen");
    textB->setToolTip("Text (double-click canvas to enter text)");
    highlight->setToolTip("Highlight");
    numbered->setToolTip("Numbered (click canvas to place sequential numbers)");
    mosaic->setToolTip("Mosaic");
    eraser->setToolTip("Eraser");
    select->setToolTip("Select (click to select, drag corners to resize, drag body to move, Delete to remove)");
    copy->setToolTip("Copy");
    save->setToolTip("Save");

    connect(rectangle, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Rectangle); });
    connect(ellipse, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Ellipse); });
    connect(arrow, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Arrow); });
    connect(pen, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Pen); });
    connect(textB, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Text); });
    connect(highlight, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Highlight); });
    connect(numbered, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Numbered); });
    auto* mosaicBlurBtn = new QToolButton(toolbar);
    mosaicBlurBtn->setText("Blur");
    mosaicBlurBtn->setToolTip("Toggle mosaic blur mode");
    mosaicBlurBtn->setFixedSize(32, 24);
    mosaicBlurBtn->setCheckable(true);
    mosaicBlurBtn->setStyleSheet(
        "QToolButton { font: bold 9px; color: #bcbec6; }"
        "QToolButton:checked { color: #2fbf9f; }");
    connect(mosaicBlurBtn, &QToolButton::clicked, this, [this](bool checked) {
        canvas_->setMosaicBlurred(checked);
    });
    toolbar->addWidget(mosaicBlurBtn);

    connect(mosaic, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Mosaic); });
    connect(select, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Select); });
    connect(eraser, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Eraser); });
    connect(eyedropper, &QAction::triggered, this, [this] { canvas_->setPickingColor(true); });
    connect(copy, &QAction::triggered, this, [this] {
        emit imageEdited(canvas_->renderedImage());
        emit copyRequested();
    });
    connect(save, &QAction::triggered, this, [this] {
        emit imageEdited(canvas_->renderedImage());
        emit saveRequested();
    });
}

} // namespace snappaste
