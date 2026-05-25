#include "presentation/tray/TrayController.h"

#include "presentation/icons/IconProvider.h"

#include <QAction>
#include <QMenu>

namespace snappaste {

TrayController::TrayController(QObject* parent)
    : QObject(parent)
    , trayIcon_(this)
    , menu_(new QMenu())
{
    auto* captureAction = menu_->addAction(IconProvider::icon(IconName::Capture), "Capture");
    auto* showAction = menu_->addAction(IconProvider::icon(IconName::App), "Open SnapPaste");
    auto* hidePinsAction = menu_->addAction("Hide Pins");
    auto* showPinsAction = menu_->addAction("Show Pins");
    menu_->addSeparator();
    auto* quitAction = menu_->addAction(IconProvider::icon(IconName::Close), "Quit");

    trayIcon_.setContextMenu(menu_);
    trayIcon_.setToolTip("SnapPaste");
    trayIcon_.setIcon(IconProvider::icon(IconName::App));

    connect(captureAction, &QAction::triggered, this, &TrayController::captureRequested);
    connect(showAction, &QAction::triggered, this, &TrayController::showWindowRequested);
    connect(hidePinsAction, &QAction::triggered, this, &TrayController::hidePinsRequested);
    connect(showPinsAction, &QAction::triggered, this, &TrayController::showPinsRequested);
    connect(quitAction, &QAction::triggered, this, &TrayController::quitRequested);
    connect(&trayIcon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            emit showWindowRequested();
        }
    });
}

void TrayController::show()
{
    trayIcon_.show();
}

void TrayController::showMessage(const QString& title, const QString& message)
{
    trayIcon_.showMessage(title, message, QSystemTrayIcon::Information, 2500);
}

} // namespace snappaste
