#include "infrastructure/config/JsonSettingsRepository.h"

#include "infrastructure/filesystem/AppPaths.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

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
        return Result<AppSettings>::failure("Failed to open settings file.");
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return Result<AppSettings>::failure("Settings file is invalid.");
    }

    const auto object = document.object();
    AppSettings settings = defaultSettingsInternal();
    bool shouldPersistMigratedSettings = !object.contains("settingsVersion");

    settings.saveDirectory = object.value("saveDirectory").toString(settings.saveDirectory);
    settings.imageFormat = object.value("imageFormat").toString(settings.imageFormat).toLower();
    settings.themeMode = themeFromString(object.value("themeMode").toString("system"));
    settings.ocrLanguage = object.value("ocrLanguage").toString();
    settings.autoSaveOnCapture = object.value("autoSaveOnCapture").toBool(false);

    const auto hotkey = object.value("captureHotkey").toObject();
    settings.captureHotkey.ctrl = hotkey.value("ctrl").toBool(settings.captureHotkey.ctrl);
    settings.captureHotkey.alt = hotkey.value("alt").toBool(settings.captureHotkey.alt);
    settings.captureHotkey.shift = hotkey.value("shift").toBool(settings.captureHotkey.shift);
    settings.captureHotkey.key = hotkey.value("key").toInt(settings.captureHotkey.key);
    if (!object.contains("settingsVersion") && isLegacyCaptureDefault(settings.captureHotkey)) {
        settings.captureHotkey = defaultSettingsInternal().captureHotkey;
    }

    const auto pasteHotkey = object.value("pasteHotkey").toObject();
    settings.pasteHotkey.ctrl = pasteHotkey.value("ctrl").toBool(settings.pasteHotkey.ctrl);
    settings.pasteHotkey.alt = pasteHotkey.value("alt").toBool(settings.pasteHotkey.alt);
    settings.pasteHotkey.shift = pasteHotkey.value("shift").toBool(settings.pasteHotkey.shift);
    settings.pasteHotkey.key = pasteHotkey.value("key").toInt(settings.pasteHotkey.key);

    const auto hidePinsHotkey = object.value("hidePinsHotkey").toObject();
    settings.hidePinsHotkey.ctrl = hidePinsHotkey.value("ctrl").toBool(settings.hidePinsHotkey.ctrl);
    settings.hidePinsHotkey.alt = hidePinsHotkey.value("alt").toBool(settings.hidePinsHotkey.alt);
    settings.hidePinsHotkey.shift = hidePinsHotkey.value("shift").toBool(settings.hidePinsHotkey.shift);
    settings.hidePinsHotkey.key = hidePinsHotkey.value("key").toInt(settings.hidePinsHotkey.key);

    const auto repeatCaptureHotkey = object.value("repeatCaptureHotkey").toObject();
    settings.repeatCaptureHotkey.ctrl = repeatCaptureHotkey.value("ctrl").toBool(settings.repeatCaptureHotkey.ctrl);
    settings.repeatCaptureHotkey.alt = repeatCaptureHotkey.value("alt").toBool(settings.repeatCaptureHotkey.alt);
    settings.repeatCaptureHotkey.shift = repeatCaptureHotkey.value("shift").toBool(settings.repeatCaptureHotkey.shift);
    settings.repeatCaptureHotkey.key = repeatCaptureHotkey.value("key").toInt(settings.repeatCaptureHotkey.key);

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
    const auto tmpPath = path + ".tmp";

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
    object["autoSaveOnCapture"] = settings.autoSaveOnCapture;
    object["captureHotkey"] = hotkeyToJson(settings.captureHotkey);
    object["pasteHotkey"] = hotkeyToJson(settings.pasteHotkey);
    object["hidePinsHotkey"] = hotkeyToJson(settings.hidePinsHotkey);
    object["repeatCaptureHotkey"] = hotkeyToJson(settings.repeatCaptureHotkey);

    {
        QFile file(tmpPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return Result<void>::failure("Failed to write settings file.");
        }
        const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Indented);
        if (file.write(bytes) != bytes.size()) {
            return Result<void>::failure("Failed to write settings file: " + file.errorString());
        }
        if (!file.flush()) {
            return Result<void>::failure("Failed to flush settings file: " + file.errorString());
        }
    }

    QFile::remove(path);
    if (!QFile::rename(tmpPath, path)) {
        return Result<void>::failure("Failed to atomically save settings file.");
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
