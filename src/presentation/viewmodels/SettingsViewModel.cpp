#include "presentation/viewmodels/SettingsViewModel.h"

namespace nanosnap {

SettingsViewModel::SettingsViewModel(SettingsService& service, EventHub& eventHub, QObject* parent)
    : QObject(parent)
    , service_(service)
    , eventHub_(eventHub)
{
}

AppSettings SettingsViewModel::settings() const
{
    return settings_;
}

void SettingsViewModel::load()
{
    const auto result = service_.load();
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    settings_ = result.value();
    emit loaded(settings_);
}

void SettingsViewModel::save(QString saveDirectory, QString imageFormat, int themeIndex)
{
    settings_.saveDirectory = std::move(saveDirectory);
    settings_.imageFormat = std::move(imageFormat).toLower();
    settings_.themeMode = themeFromIndex(themeIndex);

    const auto result = service_.save(settings_);
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    emit saved();
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

} // namespace nanosnap
