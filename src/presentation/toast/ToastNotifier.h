#pragma once

#include <QObject>
#include <QPoint>

class QLabel;
class QPropertyAnimation;
class QTimer;
class QWidget;

namespace snappaste {

class ToastNotifier final : public QObject {
    Q_OBJECT

public:
    explicit ToastNotifier(QObject* parent = nullptr);
    ~ToastNotifier() override;

    void showMessage(const QString& message, const QPoint& referencePosition = QPoint());
    void hide();

private:
    void ensureToast();
    void positionToast(const QPoint& referencePosition);

    QWidget* toast_ = nullptr;
    QLabel* label_ = nullptr;
    QTimer* hideTimer_ = nullptr;
    QPropertyAnimation* fadeAnimation_ = nullptr;
};

} // namespace snappaste
