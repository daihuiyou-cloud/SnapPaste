#pragma once

#include <QDateTime>
#include <QString>

namespace snappaste {

struct CaptureRecord final {
    qint64 id = 0;
    QString filePath;
    QString thumbnailPath;
    int width = 0;
    int height = 0;
    QString format;
    QDateTime createdAt;
    QString sourceScreen;
    bool deleted = false;
};

} // namespace snappaste
