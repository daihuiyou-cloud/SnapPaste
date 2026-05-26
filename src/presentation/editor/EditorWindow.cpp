#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>

#include "presentation/editor/EditorWindow.h"

#include "presentation/icons/IconProvider.h"

#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QMimeData>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QWheelEvent>

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

QIcon makeCropIcon()
{
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#bcbec6"), 2));
    p.drawLine(3, 10, 3, 3);
    p.drawLine(3, 3, 10, 3);
    p.drawLine(17, 10, 17, 17);
    p.drawLine(10, 17, 17, 17);
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
            auto px = reinterpret_cast<const QRgb*>(src.constScanLine(y))[x];
            a += qAlpha(px); r += qRed(px); g += qGreen(px); b += qBlue(px); ++cnt;
        }
        for (int y = 0; y < h; ++y) {
            if (cnt > 0) reinterpret_cast<QRgb*>(dst.scanLine(y))[x] = qRgba(r / cnt, g / cnt, b / cnt, a / cnt);
            int top = y - radius;
            if (top >= 0) { auto px = reinterpret_cast<const QRgb*>(src.constScanLine(top))[x]; a -= qAlpha(px); r -= qRed(px); g -= qGreen(px); b -= qBlue(px); --cnt; }
            int bottom = y + radius + 1;
            if (bottom < h) { auto px = reinterpret_cast<const QRgb*>(src.constScanLine(bottom))[x]; a += qAlpha(px); r += qRed(px); g += qGreen(px); b += qBlue(px); ++cnt; }
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
        setAcceptDrops(true);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_InputMethodEnabled, true);
        setMinimumSize(640, 360);
        QSettings settings;
        fontSize_ = settings.value("editor/fontSize", 14).toInt();
        const auto saved = settings.value("editor/recentColors").toList();
        for (const auto& v : saved) {
            QColor c(v.toString());
            if (c.isValid())
                customColors_.append(c);
        }
    }

protected:
    void dragEnterEvent(QDragEnterEvent* event) override
    {
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent* event) override
    {
        const auto urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            const auto path = urls.first().toLocalFile();
            QImage img(path);
            if (!img.isNull()) {
                setImage(img);
                event->acceptProposedAction();
            }
        }
    }

public:
    QPoint toImage(QPoint widgetPt) const
    {
        return QPoint(static_cast<int>(widgetPt.x() / zoomFactor_),
                      static_cast<int>(widgetPt.y() / zoomFactor_));
    }

    void setImage(QImage image)
    {
        zoomFactor_ = 1.0;
        image_ = std::move(image);
        annotations_.clear();
        undoStack_.clear();
        redoStack_.clear();
        modified_ = false;
        nextNumber_ = 1;
        editingTextIndex_ = -1;
        preeditString_.clear();
        setMinimumSize(image_.size());
        resize(image_.size());
        updateWindowTitle();
        update();
    }

    void applyCrop(QRect cropRect)
    {
        cropRect = cropRect.intersected(image_.rect());
        if (cropRect.width() < 5 || cropRect.height() < 5) return;

        image_ = image_.copy(cropRect);
        annotations_.clear();
        annotations_.squeeze();
        selectedIndex_ = -1;
        undoStack_.clear();
        redoStack_.clear();
        nextNumber_ = 1;
        modified_ = true;

        auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
        if (scrollArea) {
            auto vp = scrollArea->viewport()->size();
            double fit = qMin(static_cast<double>(vp.width()) / image_.width(),
                               static_cast<double>(vp.height()) / image_.height());
            if (fit > 1.0) fit = 1.0;
            zoomFactor_ = fit;
        }

        setMinimumSize(image_.size());
        resize(image_.size());
        updateWindowTitle();
        update();

        if (auto* ew = qobject_cast<EditorWindow*>(window())) {
            ew->setWindowModified(true);
            emit ew->imageEdited(image_);
        }

        setTool(AnnotationTool::Select);
    }

    void clearModified() { modified_ = false; updateWindowTitle(); }

    bool isModified() const { return modified_; }

    void markModified() { modified_ = true; updateWindowTitle(); update(); }

    void zoomAt(double factor, QPoint center)
    {
        const auto oldCenter = toImage(center);
        zoomFactor_ = std::max(0.1, std::min(5.0, factor));
        auto imgSize = image_.size();
        QSize newSize(static_cast<int>(imgSize.width() * zoomFactor_),
                      static_cast<int>(imgSize.height() * zoomFactor_));
        setMinimumSize(newSize);
        resize(newSize);
        const auto newWidgetCenter = QPoint(static_cast<int>(oldCenter.x() * zoomFactor_),
                                            static_cast<int>(oldCenter.y() * zoomFactor_));
        auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
        if (scrollArea) {
            auto vp = scrollArea->viewport();
            auto scrollDelta = newWidgetCenter - center;
            scrollArea->horizontalScrollBar()->setValue(
                scrollArea->horizontalScrollBar()->value() + scrollDelta.x());
            scrollArea->verticalScrollBar()->setValue(
                scrollArea->verticalScrollBar()->value() + scrollDelta.y());
        }
        updateWindowTitle();
        update();
    }

    void updateWindowTitle()
    {
        auto* w = window();
        if (w) {
            QString title;
            if (modified_) title += "* ";
            title += "SnapPaste Editor";
            if (!image_.isNull()) {
                title += QString(" - %1x%2").arg(image_.width()).arg(image_.height());
            }
            title += QString(" - %1%").arg(static_cast<int>(zoomFactor_ * 100));
            int annCount = annotations_.size();
            if (annCount > 0) {
                title += QString(" - %1 ann").arg(annCount);
            }
            w->setWindowTitle(title);
        }
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
        if (currentTool_ == tool) return;
        currentTool_ = tool;
        switch (tool) {
        case AnnotationTool::Pen: setCursor(Qt::CrossCursor); break;
        case AnnotationTool::Text: setCursor(Qt::IBeamCursor); break;
        case AnnotationTool::Eraser:
        case AnnotationTool::Mosaic: setCursor(Qt::PointingHandCursor); break;
        case AnnotationTool::Crop: setCursor(Qt::CrossCursor); break;
        default: setCursor(Qt::CrossCursor); break;
        }
        update();
        auto* ew = qobject_cast<EditorWindow*>(window());
        if (ew) ew->onToolChanged(tool);
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
        if (onPickingColorChanged_) onPickingColorChanged_(picking);
        update();
    }

    void setOnPickingColorChanged(std::function<void(bool)> cb) { onPickingColorChanged_ = std::move(cb); }

    void setMosaicBlurred(bool blurred)
    {
        mosaicBlurred_ = blurred;
    }

    void setTextOutlineEnabled(bool enabled)
    {
        textOutlineEnabled_ = enabled;
    }

    void setFilled(bool filled)
    {
        filled_ = filled;
    }

    void updateTextBounds(int index)
    {
        if (index < 0 || index >= annotations_.size()) return;
        auto& a = annotations_[index];
        if (a.tool != AnnotationTool::Text) return;
        QFont font("Microsoft YaHei UI", a.textFontSize > 0 ? a.textFontSize : fontSize_);
        QFontMetrics fm(font);
        const auto textRect = fm.boundingRect(QRect(0, 0, 4096, 4096), Qt::AlignLeft | Qt::AlignTop, a.text);
        QRect newBounds(a.bounds.topLeft(), QSize(qMax(textRect.width() + 8, 20), qMax(textRect.height() + 8, 20)));
        if (newBounds.right() > image_.width()) {
            newBounds.moveRight(image_.width() - 4);
        }
        a.bounds = newBounds;
    }

    int fontSize() const { return fontSize_; }
    void setOnFontSizeChanged(std::function<void(int)> cb) { onFontSizeChanged_ = std::move(cb); }

    const QVector<QColor>& recentColors() const { return customColors_; }

    void addRecentColor(const QColor& color)
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

    void undo()
    {
        if (undoStack_.isEmpty()) {
            return;
        }
        redoStack_.push_back(annotations_);
        annotations_ = undoStack_.takeLast();
        markModified();
    }

    void pushUndo()
    {
        undoStack_.push_back(annotations_);
        if (undoStack_.size() > kMaxUndo) {
            undoStack_.removeFirst();
        }
    }

    void redo()
    {
        if (redoStack_.isEmpty()) {
            return;
        }
        pushUndo();
        annotations_ = redoStack_.takeLast();
        markModified();
    }

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (image_.isNull()) {
            return;
        }
        if (currentTool_ == AnnotationTool::Select && selectedIndex_ >= 0) {
            if (annotations_[selectedIndex_].tool == AnnotationTool::Text) {
                bool ok = false;
                const auto newText = QInputDialog::getMultiLineText(
                    static_cast<QWidget*>(parent()), "Edit Text", "Edit text:",
                    annotations_[selectedIndex_].text, &ok);
                if (ok && !newText.isEmpty() && newText != annotations_[selectedIndex_].text) {
                    pushUndo();
                    redoStack_.clear();
                    annotations_[selectedIndex_].text = newText;
                    update();
                }
            } else {
                annotations_.removeAt(selectedIndex_);
                selectedIndex_ = -1;
                markModified();
                update();
            }
            event->accept();
            return;
        }
        if (currentTool_ != AnnotationTool::Text) {
            return;
        }

        const auto pos = toImage(event->pos());

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
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (image_.isNull()) {
            return;
        }

        setFocus();

        // finish text editing on any mouse click, except clicks on the editing annotation
        if (editingTextIndex_ >= 0 && event->button() == Qt::LeftButton) {
            const auto pos = toImage(event->pos());
            bool clickedSelf = (editingTextIndex_ < annotations_.size()
                && annotations_[editingTextIndex_].tool == AnnotationTool::Text
                && hitTestAnnotation(annotations_[editingTextIndex_], pos));
            if (!clickedSelf) {
                editingTextIndex_ = -1;
                preeditString_.clear();
                update();
            }
        }

        if (event->button() == Qt::MiddleButton) {
            panning_ = true;
            panStart_ = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }

        if (event->button() != Qt::LeftButton) {
            return;
        }

        const auto pos = toImage(event->pos());

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
                    markModified();
                    return;
                }
            }
            return;
        }

        if (currentTool_ == AnnotationTool::Numbered) {
            pushUndo();
            redoStack_.clear();
            Annotation ann;
            ann.tool = AnnotationTool::Numbered;
            ann.bounds = QRect(pos.x() - kDefaultNumberedSize / 2, pos.y() - kDefaultNumberedSize / 2,
                               kDefaultNumberedSize, kDefaultNumberedSize);
            ann.color = currentColor_;
            ann.number = nextNumber_++;
            annotations_.push_back(std::move(ann));
            selectedIndex_ = annotations_.size() - 1;
            markModified();
            return;
        }

        if (currentTool_ == AnnotationTool::Text) {
            // finish any ongoing text editing
            if (editingTextIndex_ >= 0) {
                editingTextIndex_ = -1;
                preeditString_.clear();
                update();
            }
            // check if clicking existing text to edit
            for (int i = annotations_.size() - 1; i >= 0; --i) {
                if (annotations_.at(i).tool == AnnotationTool::Text && hitTestAnnotation(annotations_.at(i), pos)) {
                    selectedIndex_ = i;
                    editingTextIndex_ = i;
                    update();
                    return;
                }
            }
            // create new empty text annotation at click position
            QFont font("Microsoft YaHei UI", fontSize_);
            QFontMetrics fm(font);
            QRect bounds(pos.x(), pos.y(), 28, fm.height() + 8);
            if (bounds.right() > image_.width()) {
                bounds.moveRight(image_.width() - 4);
            }
            pushUndo();
            redoStack_.clear();
            Annotation ann;
            ann.tool = AnnotationTool::Text;
            ann.bounds = bounds;
            ann.text = QString();
            ann.color = currentColor_;
            ann.strokeWidth = 2;
            ann.textFontSize = fontSize_;
            ann.textOutline = textOutlineEnabled_;
            annotations_.push_back(std::move(ann));
            selectedIndex_ = annotations_.size() - 1;
            editingTextIndex_ = selectedIndex_;
            markModified();
            setFocus();
            return;
        }

        if (currentTool_ != AnnotationTool::Select && currentTool_ != AnnotationTool::Eraser
            && currentTool_ != AnnotationTool::Numbered) {
            for (int i = annotations_.size() - 1; i >= 0; --i) {
                if (hitTestAnnotation(annotations_.at(i), pos)) {
                    if (i != selectedIndex_) {
                        selectedIndex_ = i;
                        update();
                    }
                    return;
                }
            }
        }

        drawing_ = true;
        start_ = toImage(event->pos());
        current_ = start_;
        draft_ = Annotation{};
        draft_.tool = currentTool_;
        draft_.color = currentColor_;
        draft_.strokeWidth = currentStrokeWidth_;
        draft_.blurRadius = (currentTool_ == AnnotationTool::Mosaic && mosaicBlurred_) ? currentStrokeWidth_ : 0;
        draft_.filled = filled_;
        draft_.textFontSize = fontSize_;
        draft_.bounds = QRect(start_, current_);
        draft_.points = {start_};
        update();
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (!drawing_) {
            if (currentTool_ == AnnotationTool::Pen)
                setCursor(Qt::CrossCursor);
            else if (currentTool_ == AnnotationTool::Text)
                setCursor(Qt::IBeamCursor);
            else if (currentTool_ == AnnotationTool::Eraser || currentTool_ == AnnotationTool::Mosaic)
                setCursor(Qt::PointingHandCursor);
            else
                setCursor(Qt::CrossCursor);
        }
        if (panning_) {
            auto delta = event->pos() - panStart_;
            auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
            if (scrollArea) {
                scrollArea->horizontalScrollBar()->setValue(
                    scrollArea->horizontalScrollBar()->value() - delta.x());
                scrollArea->verticalScrollBar()->setValue(
                    scrollArea->verticalScrollBar()->value() - delta.y());
            }
            panStart_ = event->pos();
            event->accept();
            return;
        }
        if (currentTool_ == AnnotationTool::Select && selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
            if (resizing_) {
                auto& a = annotations_[selectedIndex_];
                auto b = resizeStartBounds_;
                const auto p = toImage(event->pos());
                switch (resizeCorner_) {
                case 0: b.setTopLeft(p); break;   // TL
                case 1: b.setTopRight(p); break;   // TR
                case 2: b.setBottomLeft(p); break; // BL
                case 3: b.setBottomRight(p); break;// BR
                }
                a.bounds = b.normalized();
                if (event->modifiers().testFlag(Qt::ShiftModifier)) {
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
                const auto imagePos = toImage(event->pos());
                const auto newPos = imagePos + moveOffset_;
                const auto delta = newPos - annotations_.at(selectedIndex_).bounds.topLeft();
                annotations_[selectedIndex_].bounds.translate(delta);
                if (annotations_[selectedIndex_].tool == AnnotationTool::Pen) {
                    for (auto& pt : annotations_[selectedIndex_].points) {
                        pt += delta;
                    }
                }
                moveOffset_ = annotations_[selectedIndex_].bounds.topLeft() - imagePos;
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

        auto rawPos = event->pos();
        if (!(rawPos == start_) && event->modifiers().testFlag(Qt::ShiftModifier)) {
            if (draft_.tool == AnnotationTool::Rectangle || draft_.tool == AnnotationTool::Ellipse) {
                auto dx = rawPos.x() - start_.x();
                auto dy = rawPos.y() - start_.y();
                int side = std::max(std::abs(dx), std::abs(dy));
                rawPos.setX(start_.x() + (dx >= 0 ? side : -side));
                rawPos.setY(start_.y() + (dy >= 0 ? side : -side));
            } else if (draft_.tool == AnnotationTool::Arrow || draft_.tool == AnnotationTool::Line) {
                double angle = std::atan2(rawPos.y() - start_.y(), rawPos.x() - start_.x());
                double snapped = std::round(angle / (M_PI / 4)) * (M_PI / 4);
                double dist = std::sqrt(std::pow(rawPos.x() - start_.x(), 2) +
                                        std::pow(rawPos.y() - start_.y(), 2));
                rawPos.setX(start_.x() + static_cast<int>(dist * std::cos(snapped)));
                rawPos.setY(start_.y() + static_cast<int>(dist * std::sin(snapped)));
            } else if (draft_.tool == AnnotationTool::Pen) {
                auto dx = std::abs(rawPos.x() - start_.x());
                auto dy = std::abs(rawPos.y() - start_.y());
                if (dx >= dy) {
                    rawPos.setY(start_.y());
                } else {
                    rawPos.setX(start_.x());
                }
            }
        }
        auto oldCurrent = current_;
        current_ = rawPos;
        draft_.bounds = QRect(start_, current_).normalized();
        if (draft_.tool == AnnotationTool::Pen) {
            draft_.points.push_back(current_);
            const int margin = currentStrokeWidth_ + 4;
            update(QRect(oldCurrent, current_).normalized().adjusted(-margin, -margin, margin, margin));
        } else if (draft_.tool == AnnotationTool::Mosaic) {
            draft_.points.push_back(current_);
            const int margin = currentStrokeWidth_ * 4 + 4;
            update(QRect(oldCurrent, current_).normalized().adjusted(-margin, -margin, margin, margin));
        } else {
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (panning_) {
            panning_ = false;
            setCursor(Qt::ArrowCursor);
            event->accept();
            return;
        }
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
        current_ = toImage(event->pos());
        draft_.bounds = QRect(start_, current_).normalized();
        if (draft_.tool == AnnotationTool::Crop) {
            if (draft_.bounds.width() > 5 && draft_.bounds.height() > 5) {
                applyCrop(draft_.bounds);
            }
            update();
            return;
        }
        if (draft_.tool == AnnotationTool::Arrow) {
            draft_.points = {start_, current_};
        }
        if (draft_.bounds.width() > 2 || draft_.bounds.height() > 2 || draft_.tool == AnnotationTool::Pen || draft_.tool == AnnotationTool::Mosaic) {
            pushUndo();
            redoStack_.clear();
            annotations_.push_back(draft_);
            selectedIndex_ = annotations_.size() - 1;
            markModified();
            if (currentTool_ != AnnotationTool::Select && currentTool_ != AnnotationTool::Text
                && currentTool_ != AnnotationTool::Numbered && currentTool_ != AnnotationTool::Mosaic
                && currentTool_ != AnnotationTool::Eraser) {
                setTool(AnnotationTool::Select);
            }
        } else {
            update();
        }
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (editingTextIndex_ >= 0 && editingTextIndex_ < annotations_.size()
            && annotations_[editingTextIndex_].tool == AnnotationTool::Text) {
            auto& textAnn = annotations_[editingTextIndex_];
            if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
                editingTextIndex_ = -1;
                preeditString_.clear();
                update();
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_Escape) {
                if (textAnn.text.isEmpty()) {
                    pushUndo();
                    redoStack_.clear();
                    annotations_.removeAt(editingTextIndex_);
                    selectedIndex_ = -1;
                }
                editingTextIndex_ = -1;
                preeditString_.clear();
                markModified();
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_Backspace) {
                if (!textAnn.text.isEmpty()) {
                    textAnn.text.chop(1);
                    updateTextBounds(editingTextIndex_);
                    markModified();
                }
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_Delete) {
                event->accept();
                return;
            }
            QString text = event->text();
            if (!text.isEmpty() && text[0].isPrint()) {
                textAnn.text += text;
                updateTextBounds(editingTextIndex_);
                markModified();
                event->accept();
                return;
            }
            // fall through for non-printable keys like arrow keys, tab, etc.
        }

        if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
            && selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()
            && editingTextIndex_ < 0) {
            pushUndo();
            redoStack_.clear();
            annotations_.removeAt(selectedIndex_);
            selectedIndex_ = -1;
            markModified();
            event->accept();
            return;
        }
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            if (event->key() == Qt::Key_Equal || event->key() == Qt::Key_Plus) {
                zoomAt(zoomFactor_ * 1.15, QPoint(width() / 2, height() / 2));
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_Minus) {
                zoomAt(zoomFactor_ / 1.15, QPoint(width() / 2, height() / 2));
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_0) {
                zoomAt(1.0, QPoint(width() / 2, height() / 2));
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_9) {
                auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
                if (scrollArea && !image_.isNull()) {
                    auto vp = scrollArea->viewport()->size();
                    double fit = qMin(static_cast<double>(vp.width()) / image_.width(),
                                       static_cast<double>(vp.height()) / image_.height());
                    int newW = static_cast<int>(image_.width() * fit);
                    int newH = static_cast<int>(image_.height() * fit);
                    if (newW > 0 && newH > 0) {
                        QSize newSize(newW, newH);
                        setMinimumSize(newSize);
                        resize(newSize);
                        zoomFactor_ = fit;
                        updateWindowTitle();
                        update();
                    }
                }
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_D && selectedIndex_ >= 0) {
                auto dup = annotations_.at(selectedIndex_);
                dup.bounds.translate(10, 10);
                pushUndo();
                redoStack_.clear();
                annotations_.push_back(std::move(dup));
                selectedIndex_ = annotations_.size() - 1;
                markModified();
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_A && !annotations_.isEmpty()) {
                selectedIndex_ = annotations_.size() - 1;
                markModified();
                event->accept();
                return;
            }
            if (event->modifiers().testFlag(Qt::ShiftModifier)) {
                if ((event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
                    && selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
                    int dir = (event->key() == Qt::Key_Up) ? 1 : -1;
                    int swap = selectedIndex_ + dir;
                    if (swap >= 0 && swap < annotations_.size()) {
                        pushUndo();
                        redoStack_.clear();
                        qSwap(annotations_[selectedIndex_], annotations_[swap]);
                        selectedIndex_ = swap;
                        markModified();
                    }
                    event->accept();
                    return;
                }
            }
            QWidget::keyPressEvent(event);
            return;
        }
        switch (event->key()) {
        case Qt::Key_R: setTool(AnnotationTool::Rectangle); event->accept(); return;
        case Qt::Key_E: setTool(AnnotationTool::Ellipse); event->accept(); return;
        case Qt::Key_A: setTool(AnnotationTool::Arrow); event->accept(); return;
        case Qt::Key_L: setTool(AnnotationTool::Line); event->accept(); return;
        case Qt::Key_P: setTool(AnnotationTool::Pen); event->accept(); return;
        case Qt::Key_T: setTool(AnnotationTool::Text); event->accept(); return;
        case Qt::Key_H: setTool(AnnotationTool::Highlight); event->accept(); return;
        case Qt::Key_N: setTool(AnnotationTool::Numbered); event->accept(); return;
        case Qt::Key_M: setTool(AnnotationTool::Mosaic); event->accept(); return;
        case Qt::Key_V: setTool(AnnotationTool::Select); event->accept(); return;
        case Qt::Key_X: setTool(AnnotationTool::Eraser); event->accept(); return;
        case Qt::Key_C: setTool(AnnotationTool::Crop); event->accept(); return;
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
            if (selectedIndex_ >= 0 && selectedIndex_ < annotations_.size()) {
                int step = event->modifiers().testFlag(Qt::ShiftModifier) ? 10 : 1;
                QPoint delta(0, 0);
                if (event->key() == Qt::Key_Up) delta.setY(-step);
                else if (event->key() == Qt::Key_Down) delta.setY(step);
                else if (event->key() == Qt::Key_Left) delta.setX(-step);
                else if (event->key() == Qt::Key_Right) delta.setX(step);
                annotations_[selectedIndex_].bounds.translate(delta);
                if (annotations_[selectedIndex_].tool == AnnotationTool::Pen) {
                    for (auto& pt : annotations_[selectedIndex_].points) pt += delta;
                }
                markModified();
                event->accept();
                return;
            }
            break;
        case Qt::Key_BracketLeft:
            if (currentTool_ == AnnotationTool::Text && fontSize_ > 8) {
                fontSize_ -= 2; markModified(); update();
                if (onFontSizeChanged_) onFontSizeChanged_(fontSize_);
                QSettings().setValue("editor/fontSize", fontSize_);
                event->accept(); return;
            }
            break;
        case Qt::Key_BracketRight:
            if (currentTool_ == AnnotationTool::Text && fontSize_ < 72) {
                fontSize_ += 2; markModified(); update();
                if (onFontSizeChanged_) onFontSizeChanged_(fontSize_);
                QSettings().setValue("editor/fontSize", fontSize_);
                event->accept(); return;
            }
            break;
        case Qt::Key_F1:
        case Qt::Key_Slash:
            if (event->modifiers().testFlag(Qt::ShiftModifier) || event->key() == Qt::Key_F1) {
                QMessageBox::information(static_cast<QWidget*>(window()), "Keyboard Shortcuts",
                    "<b>Tools</b><br>"
                    "R - Rectangle<br>E - Ellipse<br>A - Arrow<br>L - Line<br>P - Pen<br>"
                    "T - Text<br>H - Highlight<br>N - Numbered<br>M - Mosaic<br>"
                    "V - Select<br>X - Eraser<br>C - Crop<br><br>"
                    "<b>Edit</b><br>"
                        "Ctrl+Z - Undo<br>Ctrl+Y - Redo<br>Ctrl+D - Duplicate annotation<br>"
                        "Ctrl+A - Select last annotation<br>"
                        "Delete/Backspace - Remove annotation<br>"
                        "Arrow keys - Nudge annotation (Shift+Arrow = 10px)<br>"
                        "Ctrl+Shift+Arrow Up/Down - Bring forward / Send backward<br>"
                        "Double-click - Delete annotation (Text tool: edit text)<br>"
                        "[ / ] - Decrease/Increase text font size<br><br>"
                    "<b>Zoom</b><br>"
                    "Ctrl+Scroll / Ctrl++ / Ctrl+- - Zoom<br>"
                    "Ctrl+0 - 100%<br>Ctrl+9 - Fit to window<br><br>"
                    "<b>File</b><br>"
                    "Ctrl+S - Save<br>Ctrl+Shift+S - Save As...<br>"
                    "F3 - Pin image<br>"
                    "Escape - Close editor");
                event->accept();
                return;
            }
            break;
        default: break;
        }
        QWidget::keyPressEvent(event);
    }

    void contextMenuEvent(QContextMenuEvent* event) override
    {
        QMenu menu;
        auto* copyImage = menu.addAction("Copy Image\tCtrl+Shift+C");
        auto* saveAs = menu.addAction("Save As...");
        QAction* deleteAnn = nullptr;
        QAction* duplicateAnn = nullptr;
        QAction* bringForward = nullptr;
        QAction* sendBackward = nullptr;
        if (selectedIndex_ >= 0) {
            menu.addSeparator();
            deleteAnn = menu.addAction("Delete Annotation\tDel");
            duplicateAnn = menu.addAction("Duplicate Annotation");
            if (selectedIndex_ < annotations_.size() - 1)
                bringForward = menu.addAction("Bring Forward\tCtrl+Shift+Up");
            if (selectedIndex_ > 0)
                sendBackward = menu.addAction("Send Backward\tCtrl+Shift+Down");
        }
        menu.addSeparator();
        auto* zoomIn = menu.addAction("Zoom In\tCtrl++");
        auto* zoomOut = menu.addAction("Zoom Out\tCtrl+-");
        auto* zoom100 = menu.addAction("Actual Size (100%)\tCtrl+0");
        auto* zoomFit = menu.addAction("Fit to Window\tCtrl+9");
        menu.addSeparator();
        auto* clearAll = menu.addAction("Clear All Annotations");
        auto* action = menu.exec(event->globalPos());
        if (action == deleteAnn && selectedIndex_ >= 0) {
            pushUndo();
            redoStack_.clear();
            annotations_.removeAt(selectedIndex_);
            selectedIndex_ = -1;
            markModified();
        } else if (action == duplicateAnn && selectedIndex_ >= 0) {
            pushUndo();
            redoStack_.clear();
            auto dup = annotations_.at(selectedIndex_);
            dup.bounds.translate(10, 10);
            annotations_.push_back(std::move(dup));
            selectedIndex_ = annotations_.size() - 1;
            markModified();
        } else if (action == bringForward && selectedIndex_ >= 0 && selectedIndex_ < annotations_.size() - 1) {
            pushUndo();
            redoStack_.clear();
            qSwap(annotations_[selectedIndex_], annotations_[selectedIndex_ + 1]);
            selectedIndex_++;
            markModified();
        } else if (action == sendBackward && selectedIndex_ > 0 && selectedIndex_ < annotations_.size()) {
            pushUndo();
            redoStack_.clear();
            qSwap(annotations_[selectedIndex_], annotations_[selectedIndex_ - 1]);
            selectedIndex_--;
            markModified();
        } else if (action == copyImage) {
            QApplication::clipboard()->setImage(renderedImage());
        } else if (action == saveAs) {
            auto path = QFileDialog::getSaveFileName(this, "Save As", QString(),
                "PNG (*.png);;JPEG (*.jpg *.jpeg)");
            if (!path.isEmpty()) {
                renderedImage().save(path);
            }
        } else if (action == zoomIn) {
            zoomAt(zoomFactor_ * 1.15, QPoint(width() / 2, height() / 2));
        } else if (action == zoomOut) {
            zoomAt(zoomFactor_ / 1.15, QPoint(width() / 2, height() / 2));
        } else if (action == zoom100) {
            zoomAt(1.0, QPoint(width() / 2, height() / 2));
        } else if (action == zoomFit) {
            auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget());
            if (scrollArea && !image_.isNull()) {
                auto vp = scrollArea->viewport()->size();
                double fit = qMin(static_cast<double>(vp.width()) / image_.width(),
                                   static_cast<double>(vp.height()) / image_.height());
                int newW = static_cast<int>(image_.width() * fit);
                int newH = static_cast<int>(image_.height() * fit);
                if (newW > 0 && newH > 0) {
                    QSize newSize(newW, newH);
                    setMinimumSize(newSize);
                    resize(newSize);
                    zoomFactor_ = fit;
                    updateWindowTitle();
                    update();
                }
            }
        } else if (action == clearAll) {
            if (!annotations_.isEmpty()) {
                auto ret = QMessageBox::question(this, "Clear All Annotations",
                    "Are you sure you want to clear all annotations?",
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (ret == QMessageBox::Yes) {
                    pushUndo();
                    redoStack_.clear();
                    annotations_.clear();
                    selectedIndex_ = -1;
                    markModified();
                }
            }
        }
    }

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)

        QPainter painter(this);
        painter.fillRect(rect(), QColor("#101418"));
        if (!image_.isNull()) {
            painter.save();
            painter.scale(zoomFactor_, zoomFactor_);
            if (image_.hasAlphaChannel()) {
                int tile = 8;
                for (int y = 0; y < image_.height(); y += tile) {
                    for (int x = 0; x < image_.width(); x += tile) {
                        bool light = ((x / tile) + (y / tile)) % 2 == 0;
                        painter.fillRect(x, y, tile, tile, light ? QColor("#cccccc") : QColor("#888888"));
                    }
                }
            }
            painter.drawImage(QPoint(0, 0), image_);
            drawAnnotations(&painter, image_, true);
            if (drawing_) {
                drawAnnotation(&painter, image_, draft_, fontSize_);
            }
            // text editing cursor + preedit
            if (editingTextIndex_ >= 0 && editingTextIndex_ < annotations_.size()) {
                const auto& a = annotations_[editingTextIndex_];
                if (a.tool == AnnotationTool::Text) {
                    QFont font("Microsoft YaHei UI", a.textFontSize > 0 ? a.textFontSize : fontSize_);
                    painter.setFont(font);
                    int textWidth = painter.fontMetrics().horizontalAdvance(a.text);
                    int cx = a.bounds.left() + 4 + textWidth;
                    int cy = a.bounds.top() + 4;
                    int ch = painter.fontMetrics().height();
                    // draw preedit string if any
                    if (!preeditString_.isEmpty()) {
                        painter.setPen(QPen(a.color, 1));
                        painter.drawText(cx, cy, painter.fontMetrics().horizontalAdvance(preeditString_) + 4, ch,
                            Qt::AlignLeft | Qt::AlignTop, preeditString_);
                        // underline preedit text
                        int preeditWidth = painter.fontMetrics().horizontalAdvance(preeditString_);
                        painter.setPen(QPen(a.color, 1, Qt::DashLine));
                        painter.drawLine(cx, cy + ch + 1, cx + preeditWidth, cy + ch + 1);
                        textWidth += preeditWidth;
                        cx += preeditWidth;
                    }
                    // cursor
                    painter.setPen(QPen(a.color, 1.5));
                    painter.drawLine(cx, cy, cx, cy + ch);
                }
            }
            painter.restore();
        }
        if (drawing_ && draft_.tool != AnnotationTool::Pen && draft_.tool != AnnotationTool::Numbered && draft_.tool != AnnotationTool::Crop) {
            auto dims = draft_.bounds.size();
            QString label = QString("%1 × %2").arg(dims.width()).arg(dims.height());
            painter.setPen(Qt::NoPen);
            auto textRect = painter.fontMetrics().boundingRect(label);
            auto labelPos = current_;
            labelPos = QPoint(static_cast<int>(labelPos.x() * zoomFactor_),
                              static_cast<int>(labelPos.y() * zoomFactor_));
            labelPos += QPoint(12, -textRect.height() - 8);
            textRect = QRect(labelPos.x() - 4, labelPos.y() - 2,
                             textRect.width() + 8, textRect.height() + 4);
            painter.setBrush(QColor(0, 0, 0, 160));
            painter.drawRoundedRect(textRect, 3, 3);
            painter.setPen(Qt::white);
            painter.drawText(textRect, Qt::AlignCenter, label);
        }
    }

    void wheelEvent(QWheelEvent* event) override
    {
        if (image_.isNull() || !event->modifiers().testFlag(Qt::ControlModifier)) {
            QWidget::wheelEvent(event);
            return;
        }
        auto delta = event->angleDelta().y();
        if (delta == 0) return;
        zoomAt(zoomFactor_ * (delta > 0 ? 1.15 : 0.85), event->pos());
        event->accept();
    }

    void inputMethodEvent(QInputMethodEvent* event) override
    {
        if (editingTextIndex_ < 0 || editingTextIndex_ >= annotations_.size()
            || annotations_[editingTextIndex_].tool != AnnotationTool::Text) {
            QWidget::inputMethodEvent(event);
            return;
        }
        auto& a = annotations_[editingTextIndex_];
        if (!event->commitString().isEmpty()) {
            a.text += event->commitString();
            preeditString_.clear();
            updateTextBounds(editingTextIndex_);
            markModified();
        }
        preeditString_ = event->preeditString();
        update();
        event->accept();
    }

    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override
    {
        if (editingTextIndex_ >= 0 && editingTextIndex_ < annotations_.size()
            && annotations_[editingTextIndex_].tool == AnnotationTool::Text) {
            const auto& a = annotations_[editingTextIndex_];
            const auto& img = image_;
            switch (query) {
            case Qt::ImCursorRectangle: {
                QFont font("Microsoft YaHei UI", a.textFontSize > 0 ? a.textFontSize : fontSize_);
                QFontMetrics fm(font);
                int textWidth = fm.horizontalAdvance(a.text + preeditString_);
                int cx = static_cast<int>((a.bounds.left() + 4 + textWidth) * zoomFactor_);
                int cy = static_cast<int>((a.bounds.top() + 4) * zoomFactor_);
                int ch = static_cast<int>(fm.height() * zoomFactor_);
                return QRect(mapToGlobal(QPoint(0, 0)) + QPoint(cx, cy), QSize(4, ch));
            }
            case Qt::ImEnabled:
                return true;
            case Qt::ImFont:
                return QFont("Microsoft YaHei UI", a.textFontSize > 0 ? a.textFontSize : fontSize_);
            case Qt::ImCursorPosition:
                return a.text.length() + preeditString_.length();
            case Qt::ImSurroundingText:
                return a.text + preeditString_;
            case Qt::ImCurrentSelection:
                return QString();
            case Qt::ImAnchorPosition:
                return a.text.length() + preeditString_.length();
            default:
                break;
            }
        }
        return QWidget::inputMethodQuery(query);
    }

private:
    void drawAnnotations(QPainter* painter, const QImage& sourceImage, bool includeSelectionChrome) const
    {
        for (int i = 0; i < annotations_.size(); ++i) {
            drawAnnotation(painter, sourceImage, annotations_.at(i), fontSize_);
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

    static void drawAnnotation(QPainter* painter, const QImage& sourceImage, const Annotation& annotation, int fontSize = 14)
    {
        painter->setRenderHint(QPainter::Antialiasing, true);

        switch (annotation.tool) {
        case AnnotationTool::Rectangle:
            painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(annotation.filled ? annotation.color : Qt::NoBrush);
            painter->drawRect(annotation.bounds);
            break;
        case AnnotationTool::Ellipse:
            painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(annotation.filled ? annotation.color : Qt::NoBrush);
            painter->drawEllipse(annotation.bounds);
            break;
        case AnnotationTool::Arrow: {
            const auto from = annotation.points.size() >= 2 ? annotation.points.first() : annotation.bounds.topLeft();
            const auto to = annotation.points.size() >= 2 ? annotation.points.last() : annotation.bounds.bottomRight();
            painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->drawLine(from, to);
            constexpr double kArrowSize = 12.0;
            const auto angle = std::atan2(to.y() - from.y(), to.x() - from.x());
            const auto p1 = QPointF(to.x() - kArrowSize * std::cos(angle - M_PI / 6),
                                     to.y() - kArrowSize * std::sin(angle - M_PI / 6));
            const auto p2 = QPointF(to.x() - kArrowSize * std::cos(angle + M_PI / 6),
                                     to.y() - kArrowSize * std::sin(angle + M_PI / 6));
            QPolygonF arrowHead;
            arrowHead << to << p1 << p2;
            painter->setBrush(annotation.color);
            painter->setPen(Qt::NoPen);
            painter->drawPolygon(arrowHead);
            break;
        }
        case AnnotationTool::Line: {
            const auto from = annotation.points.size() >= 2 ? annotation.points.first() : annotation.bounds.topLeft();
            const auto to = annotation.points.size() >= 2 ? annotation.points.last() : annotation.bounds.bottomRight();
            painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->drawLine(from, to);
            break;
        }
        case AnnotationTool::Pen:
            painter->setPen(QPen(annotation.color, annotation.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            for (int i = 1; i < annotation.points.size(); ++i) {
                painter->drawLine(annotation.points.at(i - 1), annotation.points.at(i));
            }
            break;
        case AnnotationTool::Text: {
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
            break;
        }
        case AnnotationTool::Mosaic: {
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
                if (clipped.isEmpty()) break;
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

    static constexpr int kMaxUndo = 50;

    QImage image_;
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

    auto* copyImageAction = new QAction("Copy Image", this);
    copyImageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C));
    connect(copyImageAction, &QAction::triggered, this, [this] {
        QApplication::clipboard()->setImage(canvas_->renderedImage());
        statusBar()->showMessage("Image copied to clipboard", 3000);
    });
    addAction(copyImageAction);

    auto* pasteAction = new QAction("Paste Image", this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, [this] {
        auto pix = QApplication::clipboard()->pixmap();
        if (!pix.isNull()) {
            canvas_->setImage(pix.toImage());
            statusBar()->showMessage("Image pasted from clipboard", 3000);
        }
    });
    addAction(pasteAction);

    auto* saveAsAction = new QAction("Save As", this);
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
    connect(saveAsAction, &QAction::triggered, this, [this] {
        QSettings settings;
        auto dir = settings.value("editor/lastSaveDir").toString();
        auto path = QFileDialog::getSaveFileName(this, "Save As", dir,
            "PNG (*.png);;JPEG (*.jpg *.jpeg)");
        if (!path.isEmpty()) {
            canvas_->renderedImage().save(path);
            QSettings().setValue("editor/lastSaveDir", QFileInfo(path).absolutePath());
        }
    });
    addAction(saveAsAction);
}

void EditorWindow::closeEvent(QCloseEvent* event)
{
    if (canvas_ && canvas_->isModified()) {
        auto ret = QMessageBox::question(this, "Unsaved Changes",
            "You have unsaved annotations. Save before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            auto img = canvas_->renderedImage();
            emit imageEdited(img);
            emit saveRequested();
            event->accept();
        } else if (ret == QMessageBox::Discard) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void EditorWindow::setImage(const QImage& image)
{
    canvas_->setImage(image);
    show();
    raise();
    activateWindow();
}

void EditorWindow::onToolChanged(AnnotationTool tool)
{
    if (updateToolActions_) {
        updateToolActions_(tool);
    }
}

void EditorWindow::createToolbar()
{
    auto* toolbar = new QToolBar("Editor", this);
    addToolBar(Qt::RightToolBarArea, toolbar);
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setStyleSheet(
        "QToolBar { spacing: 1px; padding: 6px 4px; }"
        "QToolButton { font: 9px 'Microsoft YaHei UI','Segoe UI',sans-serif;"
        "  padding: 3px 4px; text-align: left; }");

    const QString toggleBtnStyle =
        "QToolButton { font: bold 10px; color: #999; background: transparent;"
        "  border: none; border-radius: 4px; padding: 3px 5px; }"
        "QToolButton:hover { background: rgba(47,191,159,0.1); color: #2fbf9f; }"
        "QToolButton:checked { color: #fff; background: #2fbf9f; }"
        "QToolButton:hover:checked { background: #269d84; }";

    // ─────────────────────────────────────────────
    // Group 1: Drawing Tools
    // ─────────────────────────────────────────────
    auto* rectangle = toolbar->addAction(IconProvider::icon(IconName::Rectangle), "Rectangle");
    rectangle->setCheckable(true);
    auto* ellipse = toolbar->addAction(makeEllipseIcon(), "Ellipse");
    ellipse->setCheckable(true);
    auto* arrow = toolbar->addAction(IconProvider::icon(IconName::Arrow), "Arrow");
    arrow->setCheckable(true);
    auto* lineTool = toolbar->addAction(IconProvider::icon(IconName::Line), "Line");
    lineTool->setCheckable(true);
    auto* pen = toolbar->addAction(IconProvider::icon(IconName::Pen), "Pen");
    pen->setCheckable(true);
    auto* textAction = toolbar->addAction(IconProvider::icon(IconName::Text), "Text");
    textAction->setCheckable(true);
    auto* highlight = toolbar->addAction(makeHighlightIcon(), "Highlight");
    highlight->setCheckable(true);
    auto* numbered = toolbar->addAction(makeNumberedIcon(), "Numbered");
    numbered->setCheckable(true);
    auto* mosaic = toolbar->addAction(IconProvider::icon(IconName::Mosaic), "Mosaic");
    mosaic->setCheckable(true);
    auto* eraser = toolbar->addAction(makeEraserIcon(), "Eraser");
    eraser->setCheckable(true);
    auto* select = toolbar->addAction(makeSelectIcon(), "Select");
    select->setCheckable(true);
    auto* crop = toolbar->addAction(makeCropIcon(), "Crop");
    crop->setCheckable(true);

    toolbar->addSeparator();

    // ─────────────────────────────────────────────
    // Group 2: Properties — Outline, Fill, Blur,
    //           Stroke(S/M/L), Font, Color
    // ─────────────────────────────────────────────
    auto* outlineBtn = new QToolButton(toolbar);
    outlineBtn->setText("Outline");
    outlineBtn->setToolTip("Toggle text outline");
    outlineBtn->setFixedHeight(24);
    outlineBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    outlineBtn->setCheckable(true);
    outlineBtn->setChecked(true);
    outlineBtn->setStyleSheet(toggleBtnStyle);
    connect(outlineBtn, &QToolButton::clicked, this, [this](bool checked) {
        canvas_->setTextOutlineEnabled(checked);
    });
    toolbar->addWidget(outlineBtn);

    auto* fillBtn = new QToolButton(toolbar);
    fillBtn->setText("Fill");
    fillBtn->setToolTip("Toggle fill for shapes");
    fillBtn->setFixedHeight(24);
    fillBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    fillBtn->setCheckable(true);
    fillBtn->setStyleSheet(toggleBtnStyle);
    connect(fillBtn, &QToolButton::clicked, this, [this](bool checked) {
        canvas_->setFilled(checked);
    });
    toolbar->addWidget(fillBtn);

    auto* mosaicBlurBtn = new QToolButton(toolbar);
    mosaicBlurBtn->setText("Blur");
    mosaicBlurBtn->setToolTip("Toggle mosaic blur mode");
    mosaicBlurBtn->setFixedHeight(24);
    mosaicBlurBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mosaicBlurBtn->setCheckable(true);
    mosaicBlurBtn->setStyleSheet(toggleBtnStyle);
    connect(mosaicBlurBtn, &QToolButton::clicked, this, [this](bool checked) {
        canvas_->setMosaicBlurred(checked);
    });
    toolbar->addWidget(mosaicBlurBtn);

    // Stroke presets
    struct StrokePreset { QString label; int width; };
    const StrokePreset strokes[] = {{"S", 2}, {"M", 4}, {"L", 8}};
    auto* strokeGroup = new QButtonGroup(toolbar);
    strokeGroup->setExclusive(true);
    for (const auto& s : strokes) {
        auto* btn = new QToolButton(toolbar);
        btn->setText(s.label);
        btn->setToolTip(QString("Stroke: %1px").arg(s.width));
        btn->setFixedHeight(24);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setCheckable(true);
        btn->setStyleSheet(toggleBtnStyle);
        if (s.width == 4) btn->setChecked(true);
        strokeGroup->addButton(btn);
        connect(btn, &QToolButton::clicked, this, [this, s] {
            canvas_->setStrokeWidth(s.width);
        });
        toolbar->addWidget(btn);
    }

    // Font size label
    auto* fontSizeLabel = new QLabel(toolbar);
    fontSizeLabel->setText("14px");
    fontSizeLabel->setToolTip("Font size");
    fontSizeLabel->setStyleSheet("color: #bcbec6; font: 10px; padding: 0 4px; background: transparent;");
    toolbar->addWidget(fontSizeLabel);
    canvas_->setOnFontSizeChanged([fontSizeLabel](int size) {
        fontSizeLabel->setText(QString("%1px").arg(size));
    });

    // Color picker
    auto* colorBtn = new QToolButton(toolbar);
    colorBtn->setObjectName("colorWell");
    colorBtn->setFixedHeight(26);
    colorBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    colorBtn->setPopupMode(QToolButton::InstantPopup);
    colorBtn->setToolTip("Color");

    auto updateColorIcon = [colorBtn](const QColor& c) {
        QPixmap px(18, 18);
        px.fill(c);
        QPainter p(&px);
        p.setPen(QPen(QColor(255, 255, 255, 48), 1));
        p.drawRect(QRectF(0.5, 0.5, 17, 17));
        p.end();
        colorBtn->setIcon(QIcon(px));
    };
    updateColorIcon(QColor("#ff3b30"));

    const QColor fixedColors[] = {
        QColor("#ff3b30"), QColor("#ff9500"), QColor("#ffcc00"),
        QColor("#34c759"), QColor("#007aff"), QColor("#af52de"),
        QColor("#ffffff"), QColor("#000000")
    };

    auto* colorMenu = new QMenu(colorBtn);
    auto* eyeAction = new QAction(makeEyedropperIcon(), "Eyedropper", nullptr);
    eyeAction->setCheckable(true);
    connect(eyeAction, &QAction::triggered, this, [this, eyeAction] {
        canvas_->setPickingColor(eyeAction->isChecked());
    });
    canvas_->setOnPickingColorChanged([eyeAction](bool picking) {
        eyeAction->setChecked(picking);
    });
    colorMenu->addAction(eyeAction);

    connect(colorMenu, &QMenu::aboutToShow, this, [this, colorMenu, fixedColors, updateColorIcon, eyeAction]() {
        auto actions = colorMenu->actions();
        for (auto* action : actions) {
            if (action != eyeAction) {
                colorMenu->removeAction(action);
                delete action;
            }
        }
        auto recent = canvas_->recentColors();
        if (!recent.isEmpty()) {
            for (const auto& c : recent) {
                auto* a = new QAction(makeColorIcon(c), c.name(QColor::HexRgb).toUpper(), colorMenu);
                colorMenu->insertAction(eyeAction, a);
                connect(a, &QAction::triggered, this, [this, c, updateColorIcon] {
                    canvas_->setColor(c); updateColorIcon(c);
                });
            }
            colorMenu->insertSeparator(eyeAction);
        }
        for (const auto& c : fixedColors) {
            auto* a = new QAction(makeColorIcon(c), c.name(QColor::HexRgb).toUpper(), colorMenu);
            colorMenu->insertAction(eyeAction, a);
            connect(a, &QAction::triggered, this, [this, c, updateColorIcon] {
                canvas_->setColor(c); updateColorIcon(c);
            });
        }
        colorMenu->insertSeparator(eyeAction);
        auto* customAction = new QAction("Custom Color...", colorMenu);
        colorMenu->insertAction(eyeAction, customAction);
        connect(customAction, &QAction::triggered, this, [this, updateColorIcon] {
            auto color = QColorDialog::getColor(Qt::white, this, "Choose Color");
            if (color.isValid()) {
                canvas_->setColor(color);
                canvas_->addRecentColor(color);
                updateColorIcon(color);
            }
        });
        colorMenu->insertSeparator(eyeAction);
    });

    colorBtn->setMenu(colorMenu);
    toolbar->addWidget(colorBtn);

    // ── Spacer: push actions to bottom ──
    auto* spacer = new QWidget(toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer->setStyleSheet("background: transparent;");
    toolbar->addWidget(spacer);

    // ─────────────────────────────────────────────
    // Group 3: Actions (bottom)
    // ─────────────────────────────────────────────
    toolbar->addSeparator();
    auto* undo = toolbar->addAction(QIcon::fromTheme("edit-undo"), "Undo");
    auto* redo = toolbar->addAction(QIcon::fromTheme("edit-redo"), "Redo");
    auto* copy = toolbar->addAction(IconProvider::icon(IconName::Copy), "Copy");
    auto* pinBtn = toolbar->addAction(IconProvider::icon(IconName::Pin), "Pin");
    auto* save = toolbar->addAction(IconProvider::icon(IconName::Save), "Save");
    auto* saveAs = toolbar->addAction("Save As...");

    // ── ToolTips ──
    rectangle->setToolTip("Rectangle (R)");
    ellipse->setToolTip("Ellipse (E)");
    arrow->setToolTip("Arrow (A)");
    lineTool->setToolTip("Line (L)");
    pen->setToolTip("Pen (P)");
    textAction->setToolTip("Text (T)");
    highlight->setToolTip("Highlight (H)");
    numbered->setToolTip("Numbered (N)");
    mosaic->setToolTip("Mosaic (M)");
    eraser->setToolTip("Eraser (X)");
    select->setToolTip("Select (V)");
    crop->setToolTip("Crop (C)");
    undo->setToolTip("Undo (Ctrl+Z)");
    redo->setToolTip("Redo (Ctrl+Y)");
    copy->setToolTip("Copy");
    pinBtn->setToolTip("Pin (F3)");
    save->setToolTip("Save (Ctrl+S)");
    saveAs->setToolTip("Save As... (Ctrl+Shift+S)");

    // ── updateToolActions callback ──
    updateToolActions_ = [rectangle, ellipse, arrow, lineTool, pen, textAction, highlight, numbered, mosaic, select, eraser, crop](AnnotationTool tool) {
        QAction* lookup[] = {rectangle, ellipse, arrow, lineTool, pen, textAction, highlight, numbered, mosaic, select, eraser, crop};
        const AnnotationTool tools[] = {AnnotationTool::Rectangle, AnnotationTool::Ellipse, AnnotationTool::Arrow,
            AnnotationTool::Line, AnnotationTool::Pen, AnnotationTool::Text, AnnotationTool::Highlight, AnnotationTool::Numbered,
            AnnotationTool::Mosaic, AnnotationTool::Select, AnnotationTool::Eraser, AnnotationTool::Crop};
        for (auto* action : lookup) action->setChecked(false);
        for (int i = 0; i < 12; ++i) {
            if (tools[i] == tool) { lookup[i]->setChecked(true); break; }
        }
    };
    updateToolActions_(AnnotationTool::Rectangle);

    // ── Connections ──
    connect(undo, &QAction::triggered, this, [this] { canvas_->undo(); });
    connect(redo, &QAction::triggered, this, [this] { canvas_->redo(); });
    connect(rectangle, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Rectangle); });
    connect(ellipse, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Ellipse); });
    connect(arrow, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Arrow); });
    connect(lineTool, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Line); });
    connect(pen, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Pen); });
    connect(textAction, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Text); });
    connect(highlight, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Highlight); });
    connect(numbered, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Numbered); });
    connect(mosaic, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Mosaic); });
    connect(select, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Select); });
    connect(crop, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Crop); });
    connect(eraser, &QAction::triggered, this, [this] { canvas_->setTool(AnnotationTool::Eraser); });
    connect(copy, &QAction::triggered, this, [this] {
        emit imageEdited(canvas_->renderedImage());
        emit copyRequested();
        statusBar()->showMessage("Copied to clipboard", 3000);
    });
    connect(pinBtn, &QAction::triggered, this, [this] {
        auto img = canvas_->renderedImage();
        emit imageEdited(img);
        emit pinRequested(img);
        statusBar()->showMessage("Image pinned", 3000);
    });
    connect(save, &QAction::triggered, this, [this] {
        canvas_->clearModified();
        emit imageEdited(canvas_->renderedImage());
        emit saveRequested();
        statusBar()->showMessage("Saved", 3000);
    });
    connect(saveAs, &QAction::triggered, this, [this] {
        auto path = QFileDialog::getSaveFileName(this, "Save As", QString(),
            "PNG (*.png);;JPEG (*.jpg *.jpeg)");
        if (!path.isEmpty()) {
            canvas_->renderedImage().save(path);
            statusBar()->showMessage("Saved to " + path, 5000);
        }
    });
}

} // namespace snappaste
