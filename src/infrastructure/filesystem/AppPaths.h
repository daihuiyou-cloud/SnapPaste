#pragma once

#include <QString>

namespace nanosnap {

class AppPaths final {
public:
    static QString dataDirectory();
    static QString configFilePath();
    static QString databaseFilePath();
    static QString defaultCaptureDirectory();
    static QString thumbnailDirectory();

    static bool ensureDirectory(const QString& path);
};

} // namespace nanosnap
