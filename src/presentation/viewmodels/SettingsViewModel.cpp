#include "presentation/viewmodels/SettingsViewModel.h"

#include <QCoreApplication>

namespace snappaste {

SettingsViewModel::SettingsViewModel(ISettingsRepository& repository, EventHub& eventHub, QObject* parent)
    : QObject(parent)
    , repository_(repository)
    , eventHub_(eventHub)
{
}

AppSettings SettingsViewModel::settings() const
{
    return settings_;
}

void SettingsViewModel::load()
{
    const auto result = repository_.load();
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    settings_ = result.value();
    emit loaded(settings_);
}

void SettingsViewModel::save(const AppSettings& settings)
{
    auto newSettings = settings;
    newSettings.imageFormat = newSettings.imageFormat.toLower();

    const auto result = saveWithValidation(newSettings);
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    settings_ = std::move(newSettings);
    emit saved();
    emit eventHub_.settingsChanged();
}

void SettingsViewModel::restoreDefaults()
{
    settings_ = repository_.defaultSettings();
    const auto result = saveWithValidation(settings_);
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }
    emit loaded(settings_);
    emit eventHub_.settingsChanged();
}

ThemeMode SettingsViewModel::themeFromIndex(int index)
{
    switch (index) {
    case 1:
        return ThemeMode::Light;
    case 2:
        return ThemeMode::Dark;
    case 0:
    default:
        return ThemeMode::System;
    }
}

Result<void> SettingsViewModel::saveWithValidation(const AppSettings& settings)
{
    if (settings.saveDirectory.trimmed().isEmpty()) {
        return Result<void>::failure(QCoreApplication::translate("AppErrors", "Save directory cannot be empty."));
    }
    if (settings.imageFormat != "png" && settings.imageFormat != "jpg") {
        return Result<void>::failure(QCoreApplication::translate("AppErrors", "Unsupported image format."));
    }
    return repository_.save(settings);
}

} // namespace snappaste
