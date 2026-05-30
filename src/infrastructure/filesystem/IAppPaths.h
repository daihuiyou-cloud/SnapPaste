#pragma once

#include <QString>

namespace snappaste {

struct IAppPaths {
    virtual ~IAppPaths() = default;
    virtual QString dataDirectory() = 0;
    virtual QString configFilePath() = 0;
    virtual QString databaseFilePath() = 0;
    virtual QString defaultCaptureDirectory() = 0;
    virtual QString thumbnailDirectory() = 0;
    virtual bool ensureDirectory(const QString& path) = 0;
};

} // namespace snappaste
