#pragma once

#include "shared/result/Result.h"
#include "shared/types/AppSettings.h"

namespace snappaste {

class ISettingsRepository {
public:
    virtual ~ISettingsRepository() = default;

    virtual Result<AppSettings> load() = 0;
    virtual Result<void> save(const AppSettings& settings) = 0;
    virtual AppSettings defaultSettings() = 0;
};

} // namespace snappaste
