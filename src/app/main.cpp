#include "app/Application.h"
#include "platform/windows/dpi/HighDpiManager.h"
#include "presentation/toast/ToastNotifier.h"

#include <QApplication>
#include <QSharedMemory>
#include <QTimer>

int main(int argc, char* argv[])
{
    snappaste::HighDpiManager::configureBeforeApplication();

    QApplication qtApplication(argc, argv);
    QCoreApplication::setOrganizationName("SnapPaste");
    QCoreApplication::setApplicationName("SnapPaste");
    QCoreApplication::setApplicationVersion("0.1.0");

    QSharedMemory singleInstanceGuard("SnapPasteSingleInstance");
    if (!singleInstanceGuard.create(1)) {
        auto* notifier = new snappaste::ToastNotifier(&qtApplication);
        notifier->showMessage(QStringLiteral("\u7A0B\u5E8F\u5DF2\u8FD0\u884C"));
        QTimer::singleShot(2000, &qtApplication, &QApplication::quit);
        return qtApplication.exec();
    }

    snappaste::Application application(qtApplication);
    return application.run();
}
