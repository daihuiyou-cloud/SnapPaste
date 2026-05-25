#pragma once

#include <QObject>
#include <QSystemTrayIcon>

class QAction;
class QMenu;

namespace snappaste {

class TrayController final : public QObject {
    Q_OBJECT

public:
    explicit TrayController(QObject* parent = nullptr);

    void show();
    void showMessage(const QString& title, const QString& message);

signals:
    void captureRequested();
    void showWindowRequested();
    void hidePinsRequested();
    void showPinsRequested();
    void quitRequested();

private:
    QSystemTrayIcon trayIcon_;
    QMenu* menu_ = nullptr;
};

} // namespace snappaste
