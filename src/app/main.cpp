#include "app/Application.h"
#include "platform/windows/dpi/HighDpiManager.h"
#include "presentation/toast/ToastNotifier.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QSharedMemory>
#include <QTimer>
#include <QTranslator>

static void installTranslators()
{
    auto* translator = new QTranslator(QCoreApplication::instance());
    if (translator->load(QLocale(), QLatin1String("snappaste"), QLatin1String("_"),
                         QLatin1String(":/translations"))) {
        QCoreApplication::installTranslator(translator);
    }
}

int main(int argc, char* argv[])
{
    snappaste::HighDpiManager::configureBeforeApplication();

    QApplication qtApplication(argc, argv);
    QCoreApplication::setOrganizationName("SnapPaste");
    QCoreApplication::setApplicationName("SnapPaste");
    QCoreApplication::setApplicationVersion("0.1.0");

    installTranslators();

    QSharedMemory singleInstanceGuard("SnapPasteSingleInstance");
    if (!singleInstanceGuard.create(1)) {
        auto* notifier = new snappaste::ToastNotifier(&qtApplication);
        notifier->showMessage(QObject::tr("Program already running"));
        QTimer::singleShot(2000, &qtApplication, &QApplication::quit);
        return qtApplication.exec();
    }

    snappaste::Application application(qtApplication);
    return application.run();
}
