#include "domain/settings/SettingsService.h"

namespace snappaste {

SettingsService::SettingsService(ISettingsRepository& repository)
    : repository_(repository)
{
}

Result<AppSettings> SettingsService::load()
{
    return repository_.load();
}

Result<void> SettingsService::save(const AppSettings& settings)
{
    if (settings.saveDirectory.trimmed().isEmpty()) {
        return Result<void>::failure("Save directory cannot be empty.");
    }
    if (settings.imageFormat != "png" && settings.imageFormat != "jpg") {
        return Result<void>::failure("Unsupported image format.");
    }

    return repository_.save(settings);
}

AppSettings SettingsService::defaultSettings()
{
    return repository_.defaultSettings();
}

} // namespace snappaste
