#include "presentation/toast/ToastNotifier.h"

#include <QApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QLabel>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

namespace snappaste {

namespace {

constexpr int kToastMargin = 24;
constexpr int kToastDurationMs = 2200;
constexpr int kToastFadeMs = 120;

} // namespace

ToastNotifier::ToastNotifier(QObject* parent)
    : QObject(parent)
    , hideTimer_(new QTimer(this))
{
    hideTimer_->setSingleShot(true);
    connect(hideTimer_, &QTimer::timeout, this, [this] {
        if (toast_ == nullptr || fadeAnimation_ == nullptr) {
            return;
        }
        fadeAnimation_->stop();
        fadeAnimation_->setStartValue(toast_->windowOpacity());
        fadeAnimation_->setEndValue(0.0);
        fadeAnimation_->start();
    });
}

ToastNotifier::~ToastNotifier()
{
    if (fadeAnimation_ != nullptr) {
        fadeAnimation_->stop();
    }
    delete toast_;
}

void ToastNotifier::showMessage(const QString& message, const QPoint& referencePosition,
                                std::function<void()> onClick)
{
    ensureToast();
    label_->setText(message);
    positionToast(referencePosition.isNull() ? QCursor::pos() : referencePosition);
    onClick_ = std::move(onClick);

    fadeAnimation_->stop();
    toast_->setWindowOpacity(0.0);
    toast_->show();
    toast_->raise();
    fadeAnimation_->setStartValue(0.0);
    fadeAnimation_->setEndValue(1.0);
    fadeAnimation_->start();
    hideTimer_->start(kToastDurationMs);
}

void ToastNotifier::hide()
{
    hideTimer_->stop();
    if (fadeAnimation_ != nullptr) {
        fadeAnimation_->stop();
    }
    if (toast_ != nullptr) {
        toast_->hide();
        toast_->setWindowOpacity(0.0);
    }
}

bool ToastNotifier::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == toast_ && event->type() == QEvent::MouseButtonRelease) {
        hide();
        if (onClick_) {
            onClick_();
        }
        emit clicked();
        return true;
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

    label_ = new QLabel(toast_);
    label_->setObjectName("ToastLabel");
    label_->setMaximumWidth(420);
    label_->setWordWrap(true);
    label_->setStyleSheet(
        "QLabel#ToastLabel {"
        " background: rgba(20, 26, 33, 230);"
        " color: #f4fbff;"
        " border: 1px solid rgba(255, 255, 255, 36);"
        " border-radius: 6px;"
        " padding: 9px 14px;"
        " font: 11px 'Microsoft YaHei UI','Segoe UI';"
        "}");

    auto* layout = new QVBoxLayout(toast_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label_);
    toast_->setLayout(layout);

    fadeAnimation_ = new QPropertyAnimation(toast_, "windowOpacity", this);
    fadeAnimation_->setDuration(kToastFadeMs);
    connect(fadeAnimation_, &QPropertyAnimation::finished, this, [this] {
        if (toast_ != nullptr && toast_->windowOpacity() <= 0.0) {
            toast_->hide();
        }
    });
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
    label_->setMaximumWidth(std::min(420, std::max(160, bounds.width() - kToastMargin * 2)));
    toast_->adjustSize();
    const auto size = toast_->sizeHint();
    toast_->move(bounds.right() - size.width() - kToastMargin + 1,
                 bounds.bottom() - size.height() - kToastMargin + 1);
}

} // namespace snappaste
