#pragma once

#include "shared/result/Result.h"

#include <QImage>
#include <QString>

namespace snappaste {

struct StoredImage final {
    QString filePath;
    QString thumbnailPath;
};

class IImageStorage {
public:
    virtual ~IImageStorage() = default;

    virtual Result<StoredImage> saveCapture(const QImage& image,
                                            const QString& directory,
                                            const QString& format) = 0;
};

} // namespace snappaste
