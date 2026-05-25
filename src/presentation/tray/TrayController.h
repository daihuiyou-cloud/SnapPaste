#pragma once

#include <QObject>
#include <QMenu>
#include <QSystemTrayIcon>

#include <memory>

class QAction;

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
    std::unique_ptr<QMenu> menu_;
};

} // namespace snappaste
