#include "presentation/ocr/OcrResultWindow.h"

#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

namespace snappaste {

namespace {

constexpr int kFadeMargin = 12;

QPoint clampToScreen(const QPoint& pos, const QSize& size)
{
    const auto screen = QGuiApplication::primaryScreen();
    if (!screen) return pos;
    const auto geo = screen->availableGeometry();
    int x = std::max(geo.left() + kFadeMargin,
                     std::min(pos.x(), geo.right() - size.width() - kFadeMargin));
    int y = std::max(geo.top() + kFadeMargin,
                     std::min(pos.y(), geo.bottom() - size.height() - kFadeMargin));
    return {x, y};
}

} // namespace

OcrResultWindow::OcrResultWindow(const QImage& source, const QVector<OcrBlockInfo>& blocks,
                                 const QString& fullText, QWidget* parent)
    : QWidget(parent)
    , source_(source)
    , blocks_(blocks)
    , fullText_(fullText)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* titleBar = new QWidget(this);
    titleBar->setFixedHeight(kTitleBarHeight);
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(12, 0, 8, 0);

    auto* titleLabel = new QLabel(QStringLiteral("OCR Result"), titleBar);
    titleLabel->setStyleSheet("color: #edf1f5; font-size: 12px; font-weight: bold;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    auto* closeBtn = new QPushButton(QStringLiteral("\u00D7"), titleBar);
    closeBtn->setFixedSize(24, 24);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #8a9aa8; font-size: 16px;"
        " border: none; border-radius: 4px; }"
        "QPushButton:hover { background: #c43e3e; color: #ffffff; }");
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    titleLayout->addWidget(closeBtn);

    auto* buttonBar = new QWidget(this);
    auto* buttonLayout = new QHBoxLayout(buttonBar);
    buttonLayout->setContentsMargins(10, 6, 10, 8);

    auto* copyBtn = new QPushButton(QStringLiteral("Copy All"), buttonBar);
    copyBtn->setStyleSheet(
        "QPushButton { background: #2fbf9f; color: #ffffff; border: none; border-radius: 4px;"
        " padding: 5px 14px; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background: #3ddbab; }"
        "QPushButton:pressed { background: #28a88c; }");
    connect(copyBtn, &QPushButton::clicked, this, &OcrResultWindow::performCopy);

    auto* pasteBtn = new QPushButton(QStringLiteral("Paste && Close"), buttonBar);
    pasteBtn->setStyleSheet(
        "QPushButton { background: #364150; color: #edf1f5; border: 1px solid #52677e;"
        " border-radius: 4px; padding: 5px 14px; font-size: 12px; }"
        "QPushButton:hover { background: #43536a; }"
        "QPushButton:pressed { background: #2d3b4e; }");
    connect(pasteBtn, &QPushButton::clicked, this, &OcrResultWindow::performPaste);

    auto* escHint = new QPushButton(QStringLiteral("Esc"), buttonBar);
    escHint->setFixedHeight(26);
    escHint->setStyleSheet(
        "QPushButton { background: transparent; color: #6a7a88; border: 1px solid #3a414c;"
        " border-radius: 4px; padding: 4px 10px; font-size: 11px; }"
        "QPushButton:hover { color: #8a9aa8; border-color: #52677e; }");
    connect(escHint, &QPushButton::clicked, this, &QWidget::close);

    buttonLayout->addWidget(copyBtn);
    buttonLayout->addWidget(pasteBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(escHint);

    root->addWidget(titleBar);
    root->addStretch(1);
    root->addWidget(buttonBar);

    double aspect = static_cast<double>(source_.width()) / source_.height();
    int w = std::min(kMaxWidth, source_.width() + 24);
    int h = std::min(kMaxHeight, static_cast<int>(w / aspect) + kTitleBarHeight + 50);
    w = std::max(240, std::min(w, kMaxWidth));
    h = std::max(180, h);
    setFixedSize(w, h);

    rebuildCache();

    const auto cursor = QCursor::pos();
    move(clampToScreen(cursor + QPoint(12, -h / 2), {w, h}));

    setAttribute(Qt::WA_ShowWithoutActivating, false);
    show();
    activateWindow();
    raise();
}

void OcrResultWindow::rebuildCache()
{
    if (source_.isNull()) return;

    int imgW = width() - 12;
    int imgH = height() - kTitleBarHeight - 50;
    if (imgW < 1 || imgH < 1) return;

    double scale = std::min(static_cast<double>(imgW) / source_.width(),
                            static_cast<double>(imgH) / source_.height());

    int destW = static_cast<int>(source_.width() * scale);
    int destH = static_cast<int>(source_.height() * scale);
    int offsetX = (imgW - destW) / 2 + 6;
    int offsetY = (imgH - destH) / 2 + kTitleBarHeight;

    cachedPixmap_ = QPixmap(size());
    cachedPixmap_.fill(Qt::transparent);

    QPainter p(&cachedPixmap_);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(QRect(offsetX, offsetY, destW, destH), source_);

    scaledRects_.resize(blocks_.size());
    for (int i = 0; i < blocks_.size(); ++i) {
        const auto& b = blocks_[i];
        int x = offsetX + static_cast<int>(b.rect.x() * scale);
        int y = offsetY + static_cast<int>(b.rect.y() * scale);
        int w = static_cast<int>(b.rect.width() * scale);
        int h = static_cast<int>(b.rect.height() * scale);
        scaledRects_[i] = QRect(x - kBlockPadding, y - kBlockPadding,
                                w + kBlockPadding * 2, h + kBlockPadding * 2);
    }
    p.end();
    update();
}

void OcrResultWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.setBrush(QColor(28, 33, 40, 245));
    p.setPen(QPen(QColor(70, 80, 93, 180), 1));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), kCornerRadius, kCornerRadius);

    QRect clip(6, kTitleBarHeight, width() - 12, height() - kTitleBarHeight - 50);
    p.setClipRect(clip);

    if (!cachedPixmap_.isNull()) {
        p.drawPixmap(0, 0, cachedPixmap_);
    }

    for (int i = 0; i < scaledRects_.size(); ++i) {
        const bool hover = (i == hoveredIndex_);
        const bool sel = selectedIndices_.contains(i);

        QColor fill(47, 191, 159, hover || sel ? 55 : 30);
        QColor border(47, 191, 159, hover || sel ? 200 : 100);
        int bw = hover || sel ? 2 : 1;

        p.setBrush(fill);
        p.setPen(QPen(border, bw));
        p.drawRoundedRect(scaledRects_[i].adjusted(0, 0, -1, -1), 3, 3);

        if (sel) {
            p.setPen(QColor(47, 191, 159));
            p.setFont(QFont("Segoe UI", 10));
            p.drawText(scaledRects_[i].adjusted(4, 2, -4, -2),
                       Qt::AlignLeft | Qt::AlignVCenter,
                       blocks_[i].text);
        }
    }
}

int OcrResultWindow::hitTest(const QPoint& pos) const
{
    for (int i = 0; i < scaledRects_.size(); ++i) {
        if (scaledRects_[i].contains(pos)) return i;
    }
    return -1;
}

void OcrResultWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && event->y() <= kTitleBarHeight) {
        dragStart_ = event->globalPos() - frameGeometry().topLeft();
        dragging_ = true;
        return;
    }
    if (event->button() == Qt::LeftButton) {
        int idx = hitTest(event->pos());
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            if (idx >= 0) {
                if (selectedIndices_.contains(idx)) {
                    selectedIndices_.remove(idx);
                } else {
                    selectedIndices_.insert(idx);
                }
            }
        } else {
            selectedIndices_.clear();
            if (idx >= 0) {
                selectedIndices_.insert(idx);
            }
        }
        update();
        QApplication::clipboard()->setText(selectedText());
        return;
    }
    QWidget::mousePressEvent(event);
}

void OcrResultWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging_ && (event->buttons() & Qt::LeftButton)) {
        move(clampToScreen(event->globalPos() - dragStart_, size()));
        return;
    }
    int idx = hitTest(event->pos());
    if (idx != hoveredIndex_) {
        hoveredIndex_ = idx;
        if (idx >= 0) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void OcrResultWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void OcrResultWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    } else if (event->key() == Qt::Key_C && event->modifiers().testFlag(Qt::ControlModifier)) {
        performCopy();
    } else if (event->key() == Qt::Key_A && event->modifiers().testFlag(Qt::ControlModifier)) {
        selectedIndices_.clear();
        for (int i = 0; i < blocks_.size(); ++i) {
            selectedIndices_.insert(i);
        }
        update();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void OcrResultWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    rebuildCache();
}

QString OcrResultWindow::selectedText() const
{
    if (selectedIndices_.isEmpty()) {
        return fullText_;
    }
    QStringList parts;
    auto it = selectedIndices_.constBegin();
    while (it != selectedIndices_.constEnd()) {
        parts << blocks_[*it].text;
        ++it;
    }
    return parts.join('\n');
}

void OcrResultWindow::performCopy()
{
    QApplication::clipboard()->setText(selectedText());
    auto buttons = findChildren<QPushButton*>();
    for (auto* btn : buttons) {
        if (btn->text().contains(QStringLiteral("Copy"))) {
            btn->setText(QStringLiteral("Copied"));
            QTimer::singleShot(1500, this, [btn] { btn->setText(QStringLiteral("Copy All")); });
            break;
        }
    }
}

void OcrResultWindow::performPaste()
{
    performCopy();
    emit pasteRequested();
    close();
}

} // namespace snappaste