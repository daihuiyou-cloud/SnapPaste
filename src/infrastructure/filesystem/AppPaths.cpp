#include "infrastructure/filesystem/AppPaths.h"

#include <QDir>
#include <QStandardPaths>

namespace snappaste {

QString AppPaths::dataDirectory()
{
    const auto overridePath = qEnvironmentVariable("SNAPPASTE_DATA_DIR");
    const auto path = overridePath.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        : overridePath;
    ensureDirectory(path);
    return path;
}

QString AppPaths::configFilePath()
{
    return QDir(dataDirectory()).filePath("settings.json");
}

QString AppPaths::databaseFilePath()
{
    return QDir(dataDirectory()).filePath("history.sqlite");
}

QString AppPaths::defaultCaptureDirectory()
{
    const auto pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const auto path = QDir(pictures).filePath("SnapPaste");
    ensureDirectory(path);
    return path;
}

QString AppPaths::thumbnailDirectory()
{
    const auto path = QDir(dataDirectory()).filePath("thumbs");
    ensureDirectory(path);
    return path;
}

bool AppPaths::ensureDirectory(const QString& path)
{
    if (path.isEmpty()) {
        return false;
    }
    QDir dir(path);
    return dir.exists() || dir.mkpath(".");
}

} // namespace snappaste
