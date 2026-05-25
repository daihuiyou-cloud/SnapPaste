#pragma once

#include "domain/settings/ISettingsRepository.h"

namespace nanosnap {

class SettingsService final {
public:
    explicit SettingsService(ISettingsRepository& repository);

    Result<AppSettings> load();
    Result<void> save(const AppSettings& settings);

private:
    ISettingsRepository& repository_;
};

} // namespace nanosnap
