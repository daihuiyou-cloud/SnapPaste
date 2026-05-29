#include "app/Application.h"
#include "platform/windows/dpi/HighDpiManager.h"
#include "presentation/toast/ToastNotifier.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QSharedMemory>
#include <QStandardPaths>
#include <QTimer>
#include <QTranslator>

static QString readSavedLanguage()
{
    const auto overridePath = qEnvironmentVariable("SNAPPASTE_DATA_DIR");
    const auto dataDir = overridePath.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        : overridePath;
    const auto path = QDir(dataDir).filePath("settings.json");

    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return {};
    }

    return doc.object().value("language").toString();
}

static void installTranslators()
{
    const auto langTag = readSavedLanguage();

    auto* translator = new QTranslator(QCoreApplication::instance());
    QLocale locale = langTag.isEmpty() ? QLocale() : QLocale(langTag);
    if (translator->load(locale, QLatin1String("snappaste"), QLatin1String("_"),
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
