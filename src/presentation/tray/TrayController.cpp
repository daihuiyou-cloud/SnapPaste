#include "presentation/tray/TrayController.h"

#include "presentation/icons/IconProvider.h"

#include <QAction>
#include <QMenu>
#include <QMessageBox>

namespace snappaste {

TrayController::TrayController(QObject* parent)
    : QObject(parent)
    , trayIcon_(this)
    , menu_(std::make_unique<QMenu>())
{
    auto* captureAction = menu_->addAction(IconProvider::icon(IconName::Capture), tr("Capture"));
    auto* openFileAction = menu_->addAction(tr("Open Image..."));
    auto* showAction = menu_->addAction(IconProvider::icon(IconName::App), tr("Open SnapPaste"));
    auto* hidePinsAction = menu_->addAction(tr("Hide Pins"));
    auto* showPinsAction = menu_->addAction(tr("Show Pins"));
    auto* closeAllPinsAction = menu_->addAction(tr("Close All Pins"));
    menu_->addSeparator();
    auto* quitAction = menu_->addAction(IconProvider::icon(IconName::Close), tr("Quit"));

    trayIcon_.setContextMenu(menu_.get());
    trayIcon_.setToolTip(QStringLiteral("SnapPaste"));
    trayIcon_.setIcon(IconProvider::icon(IconName::App));

    connect(captureAction, &QAction::triggered, this, &TrayController::captureRequested);
    connect(openFileAction, &QAction::triggered, this, &TrayController::openFileRequested);
    connect(showAction, &QAction::triggered, this, &TrayController::showWindowRequested);
    connect(hidePinsAction, &QAction::triggered, this, &TrayController::hidePinsRequested);
    connect(showPinsAction, &QAction::triggered, this, &TrayController::showPinsRequested);
    connect(closeAllPinsAction, &QAction::triggered, this, &TrayController::closeAllPinsRequested);
    connect(quitAction, &QAction::triggered, this, [this] {
        auto ret = QMessageBox::question(nullptr, tr("Quit SnapPaste"),
            tr("Are you sure you want to quit?\nPinned images will be lost."),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            emit quitRequested();
        }
    });
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
