#pragma once

#include "domain/settings/ISettingsRepository.h"

namespace snappaste {

class JsonSettingsRepository final : public ISettingsRepository {
public:
    Result<AppSettings> load() override;
    Result<void> save(const AppSettings& settings) override;
    AppSettings defaultSettings() override;

private:
    AppSettings defaultSettingsInternal() const;
};

} // namespace snappaste
