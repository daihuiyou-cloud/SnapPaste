#pragma once

#include "domain/settings/ISettingsRepository.h"

namespace nanosnap {

class JsonSettingsRepository final : public ISettingsRepository {
public:
    Result<AppSettings> load() override;
    Result<void> save(const AppSettings& settings) override;

private:
    AppSettings defaultSettings() const;
};

} // namespace nanosnap
