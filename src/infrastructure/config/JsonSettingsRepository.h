#pragma once

#include "domain/settings/ISettingsRepository.h"
#include "infrastructure/filesystem/IAppPaths.h"

namespace snappaste {

class JsonSettingsRepository final : public ISettingsRepository {
public:
    explicit JsonSettingsRepository(IAppPaths& appPaths);

    Result<AppSettings> load() override;
    Result<void> save(const AppSettings& settings) override;
    AppSettings defaultSettings() override;

private:
    AppSettings defaultSettingsInternal() const;

    IAppPaths& appPaths_;
};

} // namespace snappaste
