#pragma once

#include <QObject>
#include <QPoint>

#include <functional>
#include <memory>
#include <queue>

class QLabel;
class QPropertyAnimation;
class QTimer;
class QWidget;

namespace snappaste {

struct ToastMessage {
    QString text;
    std::function<void()> onClick;
};

class ToastNotifier final : public QObject {
    Q_OBJECT

public:
    explicit ToastNotifier(QObject* parent = nullptr);
    ~ToastNotifier() override;

    void showMessage(const QString& message, const QPoint& referencePosition = QPoint(),
                     std::function<void()> onClick = {});
    void hide();

signals:
    void clicked();

private:
    bool eventFilter(QObject* obj, QEvent* event) override;

    void ensureToast();
    void positionToast(const QPoint& referencePosition);
    void showNext();
    void fadeOutCurrent();

    std::unique_ptr<QWidget> toast_;
    QLabel* label_ = nullptr;
    QTimer* hideTimer_ = nullptr;
    QTimer* hoverTimer_ = nullptr;
    QPropertyAnimation* fadeAnimation_ = nullptr;
    QPropertyAnimation* slideAnimation_ = nullptr;
    std::function<void()> onClick_;
    bool hovered_ = false;

    std::queue<ToastMessage> pending_;
    bool showing_ = false;
};

} // namespace snappaste
