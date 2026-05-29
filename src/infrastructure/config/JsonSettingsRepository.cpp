#include <QCoreApplication>
#include "infrastructure/config/JsonSettingsRepository.h"

#include "infrastructure/filesystem/AppPaths.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace snappaste {

namespace {

QString themeToString(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::Light:
        return "light";
    case ThemeMode::Dark:
        return "dark";
    case ThemeMode::System:
    default:
        return "system";
    }
}

ThemeMode themeFromString(const QString& value)
{
    if (value == "light") {
        return ThemeMode::Light;
    }
    if (value == "dark") {
        return ThemeMode::Dark;
    }
    return ThemeMode::System;
}

bool isLegacyCaptureDefault(const Hotkey& hotkey)
{
    return hotkey.ctrl && hotkey.alt && !hotkey.shift && hotkey.key == 'A';
}

Hotkey readHotkeyFromObject(const QJsonObject& root, const QString& key, const Hotkey& fallback)
{
    const auto obj = root.value(key).toObject();
    Hotkey hk;
    hk.ctrl = obj.value("ctrl").toBool(fallback.ctrl);
    hk.alt = obj.value("alt").toBool(fallback.alt);
    hk.shift = obj.value("shift").toBool(fallback.shift);
    hk.key = obj.value("key").toInt(fallback.key);
    return hk;
}

} // namespace

Result<AppSettings> JsonSettingsRepository::load()
{
    QFile file(AppPaths::configFilePath());
    if (!file.exists()) {
        auto settings = defaultSettingsInternal();
        const auto saveResult = save(settings);
        if (saveResult.isError()) {
            return Result<AppSettings>::failure(saveResult.error());
        }
        return Result<AppSettings>::success(settings);
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return Result<AppSettings>::failure(QCoreApplication::translate("AppErrors", "Failed to open settings file."));
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return Result<AppSettings>::failure(QCoreApplication::translate("AppErrors", "Settings file is invalid."));
    }

    const auto object = document.object();
    AppSettings settings = defaultSettingsInternal();
    bool shouldPersistMigratedSettings = !object.contains("settingsVersion");

    settings.saveDirectory = object.value("saveDirectory").toString(settings.saveDirectory);
    settings.imageFormat = object.value("imageFormat").toString(settings.imageFormat).toLower();
    settings.themeMode = themeFromString(object.value("themeMode").toString("system"));
    settings.ocrLanguage = object.value("ocrLanguage").toString();
    settings.language = object.value("language").toString();
    settings.autoSaveOnCapture = object.value("autoSaveOnCapture").toBool(false);

    settings.captureHotkey = readHotkeyFromObject(object, "captureHotkey", settings.captureHotkey);
    if (!object.contains("settingsVersion") && isLegacyCaptureDefault(settings.captureHotkey)) {
        settings.captureHotkey = defaultSettingsInternal().captureHotkey;
    }

    settings.pasteHotkey = readHotkeyFromObject(object, "pasteHotkey", settings.pasteHotkey);
    settings.hidePinsHotkey = readHotkeyFromObject(object, "hidePinsHotkey", settings.hidePinsHotkey);
    settings.repeatCaptureHotkey = readHotkeyFromObject(object, "repeatCaptureHotkey", settings.repeatCaptureHotkey);

    if (shouldPersistMigratedSettings) {
        const auto saveResult = save(settings);
        if (saveResult.isError()) {
            return Result<AppSettings>::failure(saveResult.error());
        }
    }

    return Result<AppSettings>::success(settings);
}

Result<void> JsonSettingsRepository::save(const AppSettings& settings)
{
    const auto path = AppPaths::configFilePath();

    const auto hotkeyToJson = [](const Hotkey& source) {
        QJsonObject hotkey;
        hotkey["ctrl"] = source.ctrl;
        hotkey["alt"] = source.alt;
        hotkey["shift"] = source.shift;
        hotkey["key"] = source.key;
        return hotkey;
    };

    QJsonObject object;
    object["settingsVersion"] = 2;
    object["saveDirectory"] = settings.saveDirectory;
    object["imageFormat"] = settings.imageFormat;
    object["themeMode"] = themeToString(settings.themeMode);
    object["ocrLanguage"] = settings.ocrLanguage;
    object["language"] = settings.language;
    object["autoSaveOnCapture"] = settings.autoSaveOnCapture;
    object["captureHotkey"] = hotkeyToJson(settings.captureHotkey);
    object["pasteHotkey"] = hotkeyToJson(settings.pasteHotkey);
    object["hidePinsHotkey"] = hotkeyToJson(settings.hidePinsHotkey);
    object["repeatCaptureHotkey"] = hotkeyToJson(settings.repeatCaptureHotkey);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return Result<void>::failure(QCoreApplication::translate("AppErrors", "Failed to write settings file."));
    }
    const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) {
        return Result<void>::failure(QCoreApplication::translate("AppErrors", "Failed to write settings file: ") + file.errorString());
    }
    if (!file.commit()) {
        return Result<void>::failure(QCoreApplication::translate("AppErrors", "Failed to atomically save settings file: ") + file.errorString());
    }
    return Result<void>::success();
}

AppSettings JsonSettingsRepository::defaultSettings()
{
    return defaultSettingsInternal();
}

AppSettings JsonSettingsRepository::defaultSettingsInternal() const
{
    AppSettings settings;
    settings.saveDirectory = AppPaths::defaultCaptureDirectory();
    settings.imageFormat = "png";
    settings.themeMode = ThemeMode::System;
    settings.captureHotkey = Hotkey{false, false, false, 0x70};
    settings.pasteHotkey = Hotkey{false, false, false, 0x72};
    settings.hidePinsHotkey = Hotkey{true, true, false, 'H'};
    settings.repeatCaptureHotkey = Hotkey{false, false, false, 0x73};
    return settings;
}

} // namespace snappaste
