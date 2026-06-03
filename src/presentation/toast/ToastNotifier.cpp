#include "presentation/toast/ToastNotifier.h"

#include <QApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QPointer>
#include <QEasingCurve>
#include <QLabel>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <memory>

namespace snappaste {

namespace {

constexpr int kToastMargin = 24;
constexpr int kToastDurationMs = 2200;
constexpr int kToastFadeMs = 150;
constexpr int kToastSlideMs = 200;
constexpr int kToastMaxWidth = 420;
constexpr int kToastMinWidth = 160;

} // namespace

ToastNotifier::ToastNotifier(QObject* parent)
    : QObject(parent)
    , hideTimer_(new QTimer(this))
    , hoverTimer_(new QTimer(this))
{
    hideTimer_->setSingleShot(true);
    connect(hideTimer_, &QTimer::timeout, this, [this] {
        if (toast_ == nullptr) return;
        hovered_ = false;
        fadeOutCurrent();
    });

    hoverTimer_->setSingleShot(true);
    hoverTimer_->setInterval(800);
    connect(hoverTimer_, &QTimer::timeout, this, [this] {
        if (toast_ != nullptr && hovered_) {
            hideTimer_->stop();
        }
    });
}

ToastNotifier::~ToastNotifier()
{
    if (fadeAnimation_ != nullptr) {
        fadeAnimation_->stop();
    }
    if (slideAnimation_ != nullptr) {
        slideAnimation_->stop();
    }
    delete toast_;
}

void ToastNotifier::showMessage(const QString& message, const QPoint& /*referencePosition*/,
                                std::function<void()> onClick)
{
    pending_.push({message, std::move(onClick)});
    if (!showing_) {
        showNext();
    }
}

void ToastNotifier::showNext()
{
    if (pending_.empty()) {
        showing_ = false;
        return;
    }

    showing_ = true;
    auto msg = std::move(pending_.front());
    pending_.pop();

    ensureToast();
    label_->setText(msg.text);
    positionToast(QCursor::pos());
    onClick_ = std::move(msg.onClick);

    fadeAnimation_->stop();
    if (slideAnimation_ != nullptr) {
        slideAnimation_->stop();
    }

    toast_->setWindowOpacity(0.0);
    toast_->show();
    toast_->raise();

    const auto startPos = toast_->pos() + QPoint(0, 20);
    toast_->move(startPos);

    delete slideAnimation_;
    slideAnimation_ = new QPropertyAnimation(toast_, "pos", this);
    slideAnimation_->setDuration(kToastSlideMs);
    slideAnimation_->setStartValue(startPos);
    slideAnimation_->setEndValue(toast_->pos() - QPoint(0, 20));
    slideAnimation_->setEasingCurve(QEasingCurve::OutBack);
    slideAnimation_->start();

    fadeAnimation_->setStartValue(0.0);
    fadeAnimation_->setEndValue(1.0);
    fadeAnimation_->start();
    hideTimer_->start(kToastDurationMs);
    hovered_ = false;
}

void ToastNotifier::fadeOutCurrent()
{
    if (toast_ == nullptr || fadeAnimation_ == nullptr) {
        showing_ = false;
        showNext();
        return;
    }
    fadeAnimation_->stop();
    fadeAnimation_->setStartValue(toast_->windowOpacity());
    fadeAnimation_->setEndValue(0.0);
    QPointer<ToastNotifier> guard(this);
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(fadeAnimation_, &QPropertyAnimation::finished, this, [this, guard, conn] {
        disconnect(*conn);
        if (guard.isNull()) return;
        toast_->hide();
        toast_->setWindowOpacity(0.0);
        showing_ = false;
        showNext();
    });
    fadeAnimation_->start();
}

void ToastNotifier::hide()
{
    hideTimer_->stop();
    if (fadeAnimation_ != nullptr) {
        fadeAnimation_->stop();
    }
    if (slideAnimation_ != nullptr) {
        slideAnimation_->stop();
    }
    if (toast_ != nullptr) {
        toast_->hide();
        toast_->setWindowOpacity(0.0);
    }
    while (!pending_.empty()) pending_.pop();
    showing_ = false;
}

bool ToastNotifier::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != toast_) {
        return QObject::eventFilter(obj, event);
    }

    switch (event->type()) {
    case QEvent::MouseButtonRelease:
        hideTimer_->stop();
        fadeOutCurrent();
        if (onClick_) {
            onClick_();
        }
        emit clicked();
        return true;
    case QEvent::Enter:
        hovered_ = true;
        hoverTimer_->start();
        return true;
    case QEvent::Leave:
        hovered_ = false;
        if (toast_->windowOpacity() > 0.0) {
            hideTimer_->start(kToastDurationMs);
        }
        return true;
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

void ToastNotifier::ensureToast()
{
    if (toast_ != nullptr) {
        return;
    }

    toast_ = new QWidget();
    toast_->setWindowFlags(Qt::Tool
                           | Qt::FramelessWindowHint
                           | Qt::WindowStaysOnTopHint
                           | Qt::WindowDoesNotAcceptFocus);
    toast_->setAttribute(Qt::WA_TranslucentBackground);
    toast_->setAttribute(Qt::WA_ShowWithoutActivating);
    toast_->installEventFilter(this);
    toast_->setMouseTracking(true);

    label_ = new QLabel(toast_);
    label_->setObjectName("ToastLabel");
    label_->setMaximumWidth(kToastMaxWidth);
    label_->setMinimumWidth(kToastMinWidth);
    label_->setWordWrap(true);
    label_->setStyleSheet(
        "QLabel#ToastLabel {"
        " background: rgba(20, 26, 33, 235);"
        " color: #f4fbff;"
        " border: 1px solid rgba(255, 255, 255, 40);"
        " border-radius: 8px;"
        " padding: 10px 16px;"
        " font: 12px;"
        " font-family: 'Microsoft YaHei UI','Segoe UI';"
        "}");

    auto* layout = new QVBoxLayout(toast_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label_);
    toast_->setLayout(layout);

    fadeAnimation_ = new QPropertyAnimation(toast_, "windowOpacity", this);
    fadeAnimation_->setDuration(kToastFadeMs);
    fadeAnimation_->setEasingCurve(QEasingCurve::OutCubic);
}

void ToastNotifier::positionToast(const QPoint& referencePosition)
{
    auto* screen = QGuiApplication::screenAt(referencePosition);
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr || toast_ == nullptr) {
        return;
    }

    const auto bounds = screen->availableGeometry();
    const int maxWidth = std::min(kToastMaxWidth, std::max(kToastMinWidth, bounds.width() - kToastMargin * 2));
    label_->setMaximumWidth(maxWidth);
    label_->setMinimumWidth(std::min(kToastMinWidth, maxWidth));
    toast_->adjustSize();
    const auto size = toast_->sizeHint();

    const int x = bounds.right() - size.width() - kToastMargin + 1;
    const int y = bounds.bottom() - size.height() - kToastMargin + 1;
    toast_->move(x, y);
}

} // namespace snappaste