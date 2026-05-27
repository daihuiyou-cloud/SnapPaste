#include "infrastructure/image/LocalImageStorage.h"

#include "infrastructure/filesystem/AppPaths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QUuid>

namespace snappaste {

Result<StoredImage> LocalImageStorage::saveCapture(const QImage& image,
                                                   const QString& directory,
                                                   const QString& format)
{
    if (image.isNull()) {
        return Result<StoredImage>::failure("Image is empty.");
    }

    const auto normalizedFormat = format.toLower() == "jpg" ? QString("jpg") : QString("png");
    const auto baseName = nextBaseName();
    if (!AppPaths::ensureDirectory(directory)) {
        return Result<StoredImage>::failure("Failed to create capture directory.");
    }

    const auto capturePath = QDir(directory).filePath(baseName + "." + normalizedFormat);
    const auto thumbnailPath = QDir(AppPaths::thumbnailDirectory()).filePath(baseName + ".jpg");

    if (!image.save(capturePath, normalizedFormat.toUpper().toUtf8().constData())) {
        QFile::remove(capturePath);
        return Result<StoredImage>::failure("Failed to save capture image.");
    }

    const auto thumb = image.scaled(320, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (!thumb.save(thumbnailPath, "JPG", 82)) {
        QFile::remove(capturePath);
        return Result<StoredImage>::failure("Failed to save thumbnail image.");
    }

    return Result<StoredImage>::success(StoredImage{capturePath, thumbnailPath});
}

QString LocalImageStorage::nextBaseName() const
{
    return "SnapPaste_" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd_HHmmss_zzz")
           + "_" + QString::number(QRandomGenerator::global()->bounded(100000));
}

} // namespace snappaste
