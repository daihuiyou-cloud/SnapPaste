#pragma once

#include "domain/settings/ISettingsRepository.h"

namespace snappaste {

class SettingsService final {
public:
    explicit SettingsService(ISettingsRepository& repository);

    Result<AppSettings> load();
    Result<void> save(const AppSettings& settings);
    AppSettings defaultSettings();

private:
    ISettingsRepository& repository_;
};

} // namespace snappaste
