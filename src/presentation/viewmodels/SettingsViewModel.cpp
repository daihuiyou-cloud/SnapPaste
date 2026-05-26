#include "presentation/viewmodels/SettingsViewModel.h"

namespace snappaste {

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

void SettingsViewModel::save(QString saveDirectory, QString imageFormat, int themeIndex,
                             Hotkey captureHotkey, Hotkey pasteHotkey, Hotkey hidePinsHotkey,
                             QString ocrLanguage, bool autoSaveOnCapture,
                             Hotkey repeatCaptureHotkey)
{
    AppSettings newSettings;
    newSettings.saveDirectory = std::move(saveDirectory);
    newSettings.imageFormat = std::move(imageFormat).toLower();
    newSettings.themeMode = themeFromIndex(themeIndex);
    newSettings.captureHotkey = std::move(captureHotkey);
    newSettings.pasteHotkey = std::move(pasteHotkey);
    newSettings.hidePinsHotkey = std::move(hidePinsHotkey);
    newSettings.ocrLanguage = std::move(ocrLanguage);
    newSettings.autoSaveOnCapture = autoSaveOnCapture;
    newSettings.repeatCaptureHotkey = std::move(repeatCaptureHotkey);

    const auto result = service_.save(newSettings);
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
    settings_ = service_.defaultSettings();
    const auto result = service_.save(settings_);
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

} // namespace snappaste
