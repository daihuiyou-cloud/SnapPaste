#pragma once

#include "presentation/icons/IIconProvider.h"

#include <QObject>
#include <QMenu>
#include <QSystemTrayIcon>

#include <memory>

class QAction;

namespace snappaste {

class TrayController final : public QObject {
    Q_OBJECT

public:
    explicit TrayController(IIconProvider& iconProvider, QObject* parent = nullptr);

    void show();
    void showMessage(const QString& title, const QString& message);

private:
    IIconProvider& iconProvider_;

signals:
    void captureRequested();
    void openFileRequested();
    void showWindowRequested();
    void hidePinsRequested();
    void showPinsRequested();
    void closeAllPinsRequested();
    void quitRequested();

private:
    QSystemTrayIcon trayIcon_;
    std::unique_ptr<QMenu> menu_;
};

} // namespace snappaste
