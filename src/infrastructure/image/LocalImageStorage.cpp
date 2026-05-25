#include "infrastructure/image/LocalImageStorage.h"

#include "infrastructure/filesystem/AppPaths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QUuid>

namespace nanosnap {

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
    const auto suffix = QUuid::createUuid().toString(QUuid::Id128).left(8);
    return "NanoSnap_" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd_HHmmss_zzz") + "_" + suffix;
}

} // namespace nanosnap
