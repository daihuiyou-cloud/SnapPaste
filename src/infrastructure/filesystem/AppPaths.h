#pragma once

#include "infrastructure/filesystem/IAppPaths.h"

namespace snappaste {

class AppPaths final : public IAppPaths {
public:
    QString dataDirectory() override;
    QString configFilePath() override;
    QString databaseFilePath() override;
    QString defaultCaptureDirectory() override;
    QString thumbnailDirectory() override;
    bool ensureDirectory(const QString& path) override;
};

} // namespace snappaste
