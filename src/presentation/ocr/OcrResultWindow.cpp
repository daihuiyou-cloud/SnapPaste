#include "presentation/ocr/OcrResultWindow.h"

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTimer>
#include <QTimerEvent>
#include <QVBoxLayout>
#include <QWindowStateChangeEvent>

namespace snappaste {

namespace {

constexpr int kFadeMargin = 12;

QPoint clampToScreen(const QPoint& pos, const QSize& size)
{
    auto screen = QGuiApplication::screenAt(pos);
    if (!screen) { screen = QGuiApplication::primaryScreen(); }
    if (!screen) return pos;
    const auto geo = screen->availableGeometry();
    int x = std::max(geo.left() + kFadeMargin,
                     std::min(pos.x(), geo.right() - size.width() - kFadeMargin));
    int y = std::max(geo.top() + kFadeMargin,
                     std::min(pos.y(), geo.bottom() - size.height() - kFadeMargin));
    return {x, y};
}

QString titleBtnStyle(const char* hoverColor)
{
    return QStringLiteral(
        "QPushButton { background: transparent; color: #8a9aa8; font-size: 12px;"
        " border: none; border-radius: 6px; font-weight: 600; }"
        "QPushButton:hover { background: %1; color: #ffffff; }"
        "QPushButton:pressed { background: %2; }")
        .arg(hoverColor)
        .arg(hoverColor);
}

} // namespace

OcrResultWindow::OcrResultWindow(QImage source, QVector<OcrBlockInfo> blocks,
                                 QString fullText, QWidget* parent)
    : QWidget(parent)
    , source_(std::move(source))
    , blocks_(std::move(blocks))
    , fullText_(std::move(fullText))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(kMinWidth, kMinHeight);
    resize(kDefaultWidth, kDefaultHeight);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // --- Title bar ---
    auto* titleBar = new QWidget(this);
    titleBar->setFixedHeight(kTitleBarHeight);
    titleBar->setCursor(Qt::ArrowCursor);
    titleBar->setStyleSheet(
        "background: #1c2128; border-top-left-radius: 10px;"
        " border-top-right-radius: 10px;");
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(14, 0, 8, 0);

    auto* titleLabel = new QLabel(tr("OCR Result"), titleBar);
    titleLabel->setStyleSheet("color: #edf1f5; font-size: 13px; font-weight: 600;"
                              " background: transparent;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    // Minimize
    minimizeBtn_ = new QPushButton(titleBar);
    minimizeBtn_->setText(QString(QChar(0x2014)));
    minimizeBtn_->setFixedSize(32, 28);
    minimizeBtn_->setFocusPolicy(Qt::NoFocus);
    minimizeBtn_->setToolTip(tr("Minimize"));
    minimizeBtn_->setCursor(Qt::ArrowCursor);
    minimizeBtn_->setStyleSheet(titleBtnStyle("#4f5b70"));
    connect(minimizeBtn_, &QPushButton::clicked, this, &QWidget::showMinimized);
    titleLayout->addWidget(minimizeBtn_);

    // Maximize / Restore
    maximizeBtn_ = new QPushButton(titleBar);
    maximizeBtn_->setText(QString(QChar(0x25A1)));
    maximizeBtn_->setFixedSize(32, 28);
    maximizeBtn_->setFocusPolicy(Qt::NoFocus);
    maximizeBtn_->setToolTip(tr("Maximize"));
    maximizeBtn_->setCursor(Qt::ArrowCursor);
    maximizeBtn_->setStyleSheet(titleBtnStyle("#4f5b70"));
    connect(maximizeBtn_, &QPushButton::clicked, this, [this] {
        if (maximized_) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    titleLayout->addWidget(maximizeBtn_);

    // Close
    auto* closeBtn = new QPushButton(QString(QChar(0x2715)), titleBar);
    closeBtn->setFixedSize(32, 28);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setToolTip(tr("Close (Esc)"));
    closeBtn->setCursor(Qt::ArrowCursor);
    closeBtn->setStyleSheet(titleBtnStyle("#e03a3a"));
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    titleLayout->addWidget(closeBtn);

    updateTitleButtons();

    root->addWidget(titleBar);

    // --- Horizontal split: image | text list ---
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->setChildrenCollapsible(false);
    splitter->setStyleSheet(
        "QSplitter::handle { background: #252c35; }");

    // Left pane: scrollable image
    auto* imagePanel = new QWidget(splitter);
    auto* imageLayout = new QVBoxLayout(imagePanel);
    imageLayout->setContentsMargins(0, 0, 0, 0);

    imageScrollArea_ = new QScrollArea(imagePanel);
    imageScrollArea_->setWidgetResizable(true);
    imageScrollArea_->setFrameShape(QFrame::NoFrame);
    imageScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    imageScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    imageScrollArea_->setStyleSheet(
        "QScrollArea { background: #1c2128; border: none; }"
        "QScrollBar:vertical { background: #252c35; width: 8px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #3a4455; border-radius: 4px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #4f5b70; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal { background: #252c35; height: 8px; margin: 0; }"
        "QScrollBar::handle:horizontal { background: #3a4455; border-radius: 4px; min-width: 30px; }"
        "QScrollBar::handle:horizontal:hover { background: #4f5b70; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }");

    imageLabel_ = new QLabel(imageScrollArea_);
    imageLabel_->setAlignment(Qt::AlignCenter);
    imageLabel_->setMouseTracking(true);
    imageScrollArea_->setWidget(imageLabel_);

    imageLayout->addWidget(imageScrollArea_);
    splitter->addWidget(imagePanel);

    // Right pane: scrollable text list
    auto* textPanel = new QWidget(splitter);
    textPanel->setStyleSheet("background: #161a20;");
    auto* textPanelLayout = new QVBoxLayout(textPanel);
    textPanelLayout->setContentsMargins(0, 0, 0, 0);
    textPanelLayout->setSpacing(0);

    auto* textHeader = new QLabel(tr("  TEXT BLOCKS"), textPanel);
    textHeader->setFixedHeight(30);
    textHeader->setStyleSheet(
        "color: #6a7a88; font-size: 10px; font-weight: 600; letter-spacing: 1px;"
        " background: #13171c; border-bottom: 1px solid #252c35;");
    textPanelLayout->addWidget(textHeader);

    textScrollArea_ = new QScrollArea(textPanel);
    textScrollArea_->setWidgetResizable(true);
    textScrollArea_->setFrameShape(QFrame::NoFrame);
    textScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    textScrollArea_->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: #1c2128; width: 8px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #3a4455; border-radius: 4px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #4f5b70; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

    textListContainer_ = new QWidget(textScrollArea_);
    textListContainer_->setStyleSheet("background: transparent;");
    auto* textListLayout = new QVBoxLayout(textListContainer_);
    textListLayout->setContentsMargins(0, 0, 0, 0);
    textListLayout->setSpacing(0);

    textRows_.reserve(blocks_.size());
    for (int i = 0; i < blocks_.size(); ++i) {
        auto* row = new QFrame(textListContainer_);
        row->setFixedHeight(42);
        row->setCursor(Qt::PointingHandCursor);
        row->installEventFilter(this);
        row->setProperty("blockIndex", i);

        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(14, 0, 14, 0);
        rowLayout->setSpacing(0);

        auto* textLabel = new QLabel(blocks_[i].text, row);
        textLabel->setStyleSheet("background: transparent; color: #c8d0d8;"
                                 " font-size: 12px;");
        textLabel->setCursor(Qt::PointingHandCursor);
        rowLayout->addWidget(textLabel, 1);

        textListLayout->addWidget(row);
        textRows_.append(row);
    }
    textListLayout->addStretch();

    textScrollArea_->setWidget(textListContainer_);
    textPanelLayout->addWidget(textScrollArea_, 1);
    splitter->addWidget(textPanel);

    splitter->setStretchFactor(0, 45);
    splitter->setStretchFactor(1, 55);
    splitter->setSizes({kDefaultWidth * 45 / 100, kDefaultWidth * 55 / 100});

    root->addWidget(splitter, 1);

    // --- Bottom toolbar ---
    auto* toolBar = new QWidget(this);
    toolBar->setStyleSheet(
        "background: #1c2128; border-bottom-left-radius: 10px;"
        " border-bottom-right-radius: 10px;");
    auto* toolLayout = new QHBoxLayout(toolBar);
    toolLayout->setContentsMargins(12, 8, 12, 10);

    auto btnBase = QStringLiteral(
        "QPushButton { background: %1; color: %2; border: none; border-radius: 5px;"
        " padding: 6px 16px; font-size: 12px; font-weight: 600; }"
        "QPushButton:hover { background: %3; }"
        "QPushButton:pressed { background: %4; }");

    copyBtn_ = new QPushButton(tr("Copy All"), toolBar);
    copyBtn_->setStyleSheet(
        btnBase.arg("#2fbf9f", "#ffffff", "#3ddbab", "#28a88c"));
    connect(copyBtn_, &QPushButton::clicked, this, &OcrResultWindow::performCopy);

    pasteBtn_ = new QPushButton(tr("Paste && Close"), toolBar);
    pasteBtn_->setStyleSheet(
        btnBase.arg("#364150", "#edf1f5", "#43536a", "#2d3b4e"));
    connect(pasteBtn_, &QPushButton::clicked, this, &OcrResultWindow::performPaste);

    selectionInfo_ = new QLabel(toolBar);
    selectionInfo_->setStyleSheet("color: #6a7a88; font-size: 11px; padding: 0 6px;");

    toolLayout->addWidget(copyBtn_);
    toolLayout->addWidget(pasteBtn_);
    toolLayout->addStretch();
    toolLayout->addWidget(selectionInfo_);

    root->addWidget(toolBar);

    updateTextRows();
    updateToolbar();

    // Defer rebuild so layout is ready
    QTimer::singleShot(0, this, &OcrResultWindow::rebuildCache);

    const auto cursor = QCursor::pos();
    move(clampToScreen(cursor + QPoint(12, -height() / 2),
                       {kDefaultWidth, kDefaultHeight}));

    show();
    activateWindow();
    raise();
}

void OcrResultWindow::rebuildCache()
{
    if (source_.isNull()) return;

    double scale = 1.0;
    int margin = 20;
    auto dpr = source_.devicePixelRatio();
    double logicalW = source_.width() / dpr;
    double logicalH = source_.height() / dpr;

    int availW = qMax(100, imageScrollArea_->viewport()->width() - margin * 2);
    int availH = qMax(100, imageScrollArea_->viewport()->height() - margin * 2);

    if (logicalW > availW || logicalH > availH) {
        scale = qMin(static_cast<double>(availW) / logicalW,
                     static_cast<double>(availH) / logicalH);
    }

    int destW = static_cast<int>(logicalW * scale);
    int destH = static_cast<int>(logicalH * scale);

    basePixmap_ = QPixmap(destW + margin * 2, destH + margin * 2);
    basePixmap_.fill(Qt::transparent);

    QPainter p(&basePixmap_);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(QRect(margin, margin, destW, destH), source_);

    scaledRects_.resize(blocks_.size());
    for (int i = 0; i < blocks_.size(); ++i) {
        const auto& b = blocks_[i];
        int x = margin + static_cast<int>(b.rect.x() * scale);
        int y = margin + static_cast<int>(b.rect.y() * scale);
        int w = static_cast<int>(b.rect.width() * scale);
        int h = static_cast<int>(b.rect.height() * scale);
        scaledRects_[i] = QRect(x - kBlockPadding, y - kBlockPadding,
                                w + kBlockPadding * 2, h + kBlockPadding * 2);
    }
    p.end();

    updateImageOverlay();
}

void OcrResultWindow::updateImageOverlay()
{
    if (source_.isNull() || basePixmap_.isNull()) return;

    QPixmap pm = basePixmap_;
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    for (int i = 0; i < scaledRects_.size(); ++i) {
        bool hover = (i == hoveredIndex_);
        bool sel = selectedIndices_.contains(i);

        int bw = (hover || sel) ? 2 : 1;
        QColor fill, border;

        if (hover) {
            fill = QColor(47, 191, 159, 100);
            border = QColor(47, 191, 159, 240);
        } else if (sel) {
            fill = QColor(47, 191, 159, 70);
            border = QColor(47, 191, 159, 180);
        } else {
            fill = QColor(47, 191, 159, 20);
            border = QColor(47, 191, 159, 90);
        }

        p.setBrush(fill);
        p.setPen(QPen(border, bw));
        p.drawRoundedRect(scaledRects_[i].adjusted(0, 0, -1, -1), 2, 2);
    }
    p.end();

    imageLabel_->setPixmap(pm);
    imageLabel_->setFixedSize(pm.size());
}

void OcrResultWindow::updateTextRow(int i)
{
    if (i < 0 || i >= textRows_.size()) return;
    auto* row = textRows_[i];
    bool sel = selectedIndices_.contains(i);
    bool hover = (i == hoveredIndex_);

    if (sel) {
        row->setStyleSheet(
            "QFrame { background: #1a2d2a; border: none; border-left: 3px solid #2fbf9f;"
            " border-bottom: 1px solid #1c2128; }");
    } else if (hover) {
        row->setStyleSheet(
            "QFrame { background: #252c38; border: none; border-left: 3px solid #3a5a5a;"
            " border-bottom: 1px solid #1c2128; }");
    } else {
        row->setStyleSheet(
            "QFrame { background: transparent; border: none; border-left: 3px solid transparent;"
            " border-bottom: 1px solid #1c2128; }");
    }
}

void OcrResultWindow::updateTextRows()
{
    for (int i = 0; i < textRows_.size(); ++i) {
        updateTextRow(i);
    }
}

void OcrResultWindow::updateToolbar()
{
    if (selectedIndices_.isEmpty()) {
        selectionInfo_->setText(tr("%1 text blocks")
                                    .arg(blocks_.size()));
        copyBtn_->setText(tr("Copy All"));
    } else {
        selectionInfo_->setText(tr("%1/%2 selected")
                                    .arg(selectedIndices_.size())
                                    .arg(blocks_.size()));
        copyBtn_->setText(tr("Copy (%1)").arg(selectedIndices_.size()));
    }
}

void OcrResultWindow::toggleIndex(int idx)
{
    if (selectedIndices_.contains(idx)) {
        selectedIndices_.remove(idx);
    } else {
        selectedIndices_.insert(idx);
    }
    updateTextRow(idx);
    updateImageOverlay();
    updateToolbar();
}

void OcrResultWindow::selectAll()
{
    for (int i = 0; i < blocks_.size(); ++i) {
        selectedIndices_.insert(i);
    }
    updateTextRows();
    updateImageOverlay();
    updateToolbar();
}

void OcrResultWindow::deselectAll()
{
    selectedIndices_.clear();
    updateTextRows();
    updateImageOverlay();
    updateToolbar();
}

void OcrResultWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(28, 33, 40, 245));
    p.setPen(QPen(QColor(70, 80, 93, 180), 1));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), kCornerRadius, kCornerRadius);
}

void OcrResultWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && event->y() <= kTitleBarHeight) {
        dragStart_ = event->globalPos() - frameGeometry().topLeft();
        dragMaximizeCheck_ = event->globalPos();
        dragging_ = true;
        return;
    }
    QWidget::mousePressEvent(event);
}

void OcrResultWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging_ && (event->buttons() & Qt::LeftButton)) {
        // If dragged beyond a threshold, treat as drag (not click-to-maximize)
        QPoint delta = event->globalPos() - dragMaximizeCheck_;
        if (delta.manhattanLength() > 4) {
            dragMaximizeCheck_ = QPoint(); // invalidate
        }
        move(clampToScreen(event->globalPos() - dragStart_, size()));
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void OcrResultWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && dragging_) {
        dragging_ = false;
        dragMaximizeCheck_ = QPoint();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void OcrResultWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && event->y() <= kTitleBarHeight) {
        if (maximized_) {
            showNormal();
        } else {
            showMaximized();
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void OcrResultWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        maximized_ = windowState().testFlag(Qt::WindowMaximized);
        updateTitleButtons();
    }
    QWidget::changeEvent(event);
}

bool OcrResultWindow::eventFilter(QObject* watched, QEvent* event)
{
    auto* frame = qobject_cast<QFrame*>(watched);
    if (!frame) return QWidget::eventFilter(watched, event);

    bool ok = false;
    int idx = frame->property("blockIndex").toInt(&ok);
    if (!ok || idx < 0 || idx >= textRows_.size()) {
        return QWidget::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
            toggleIndex(idx);
            return true;
        }
        break;
    case QEvent::Enter:
        hoveredIndex_ = idx;
        updateImageOverlay();
        updateTextRow(idx);
        return true;
    case QEvent::Leave:
        hoveredIndex_ = -1;
        updateImageOverlay();
        updateTextRow(idx);
        return true;
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

void OcrResultWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    } else if (event->key() == Qt::Key_C && event->modifiers().testFlag(Qt::ControlModifier)) {
        performCopy();
    } else if (event->key() == Qt::Key_A && event->modifiers().testFlag(Qt::ControlModifier)) {
        selectAll();
    } else if (event->key() == Qt::Key_D && event->modifiers().testFlag(Qt::ControlModifier)) {
        deselectAll();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void OcrResultWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    delayedRebuild();
}

void OcrResultWindow::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == rebuildTimerId_) {
        killTimer(rebuildTimerId_);
        rebuildTimerId_ = 0;
        rebuildCache();
        return;
    }
    QWidget::timerEvent(event);
}

void OcrResultWindow::delayedRebuild()
{
    if (rebuildTimerId_) {
        killTimer(rebuildTimerId_);
    }
    rebuildTimerId_ = startTimer(100);
}

void OcrResultWindow::updateTitleButtons()
{
    minimizeBtn_->setText(maximized_ ? QString(QChar(0x2212)) : QString(QChar(0x2014)));
    maximizeBtn_->setText(maximized_ ? QString(QChar(0x2717)) : QString(QChar(0x25A1)));
    maximizeBtn_->setToolTip(maximized_ ? tr("Restore") : tr("Maximize"));
}

QString OcrResultWindow::selectedText() const
{
    if (selectedIndices_.isEmpty()) {
        return fullText_;
    }
    QStringList parts;
    for (int i = 0; i < blocks_.size(); ++i) {
        if (selectedIndices_.contains(i)) {
            parts << blocks_[i].text;
        }
    }
    return parts.join('\n');
}

void OcrResultWindow::performCopy()
{
    QString text = selectedText();
    if (text.isEmpty()) return;
    { const QSignalBlocker blocker(QApplication::clipboard()); QApplication::clipboard()->setText(text); }

    copyBtn_->setProperty("originalText", copyBtn_->text());
    copyBtn_->setText(tr("Copied!"));
    QTimer::singleShot(1500, this, [this] {
        copyBtn_->setText(copyBtn_->property("originalText").toString());
    });
}

void OcrResultWindow::performPaste()
{
    performCopy();
    QTimer::singleShot(200, this, [this] {
        emit pasteRequested();
        close();
    });
}

} // namespace snappaste