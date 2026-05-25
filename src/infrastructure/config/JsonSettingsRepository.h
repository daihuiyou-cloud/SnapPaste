#pragma once

#include "domain/settings/ISettingsRepository.h"

namespace snappaste {

class JsonSettingsRepository final : public ISettingsRepository {
public:
    Result<AppSettings> load() override;
    Result<void> save(const AppSettings& settings) override;

private:
    AppSettings defaultSettings() const;
};

} // namespace snappaste
