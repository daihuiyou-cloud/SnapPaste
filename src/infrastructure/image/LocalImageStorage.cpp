#include "infrastructure/image/LocalImageStorage.h"
#include <QCoreApplication>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QUuid>

namespace snappaste {

LocalImageStorage::LocalImageStorage(IAppPaths& appPaths)
    : appPaths_(appPaths)
{
}

Result<StoredImage> LocalImageStorage::saveCapture(const QImage& image,
                                                    const QString& directory,
                                                    const QString& format)
{
    if (image.isNull()) {
        return Result<StoredImage>::failure(QCoreApplication::translate("AppErrors", "Image is empty."));
    }

    const auto normalizedFormat = format.toLower() == "jpg" ? QString("jpg") : QString("png");
    const auto baseName = nextBaseName();
    if (!appPaths_.ensureDirectory(directory)) {
        return Result<StoredImage>::failure(QCoreApplication::translate("AppErrors", "Failed to create capture directory."));
    }

    const auto capturePath = QDir(directory).filePath(baseName + "." + normalizedFormat);
    const auto thumbnailPath = QDir(appPaths_.thumbnailDirectory()).filePath(baseName + ".jpg");

    if (!image.save(capturePath, normalizedFormat.toUpper().toUtf8().constData())) {
        QFile::remove(capturePath);
        return Result<StoredImage>::failure(QCoreApplication::translate("AppErrors", "Failed to save capture image."));
    }

    auto thumb = image;
    if (thumb.width() > 640 || thumb.height() > 400) {
        thumb = thumb.scaled(640, 400, Qt::KeepAspectRatio, Qt::FastTransformation);
    }
    thumb = thumb.scaled(320, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (!thumb.save(thumbnailPath, "JPG", 82)) {
        return Result<StoredImage>::success(StoredImage{capturePath, {}});
    }

    return Result<StoredImage>::success(StoredImage{capturePath, thumbnailPath});
}

QString LocalImageStorage::nextBaseName() const
{
    return "SnapPaste_" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd_HHmmss_zzz")
           + "_" + QString::number(QRandomGenerator::global()->bounded(100000));
}

} // namespace snappaste
