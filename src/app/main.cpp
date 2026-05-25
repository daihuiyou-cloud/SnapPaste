#include "app/Application.h"
#include "platform/windows/dpi/HighDpiManager.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    snappaste::HighDpiManager::configureBeforeApplication();

    QApplication qtApplication(argc, argv);
    QCoreApplication::setOrganizationName("SnapPaste");
    QCoreApplication::setApplicationName("SnapPaste");
    QCoreApplication::setApplicationVersion("0.1.0");

    snappaste::Application application(qtApplication);
    return application.run();
}
