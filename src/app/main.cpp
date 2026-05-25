#include "app/Application.h"
#include "platform/windows/dpi/HighDpiManager.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    nanosnap::HighDpiManager::configureBeforeApplication();

    QApplication qtApplication(argc, argv);
    QCoreApplication::setOrganizationName("NanoSnap");
    QCoreApplication::setApplicationName("NanoSnap");
    QCoreApplication::setApplicationVersion("0.1.0");

    nanosnap::Application application(qtApplication);
    return application.run();
}
