#pragma once

#include "domain/settings/ISettingsRepository.h"
#include "shared/events/EventHub.h"
#include "shared/result/Result.h"
#include "shared/types/Hotkey.h"

#include <QObject>

namespace snappaste {

class SettingsViewModel final : public QObject {
    Q_OBJECT

public:
    SettingsViewModel(ISettingsRepository& repository, EventHub& eventHub, QObject* parent = nullptr);

    AppSettings settings() const;

public slots:
    void load();
    void save(const AppSettings& settings);
    void restoreDefaults();

signals:
    void loaded(const AppSettings& settings);
    void saved();
    void errorOccurred(const QString& message);

public:
    static ThemeMode themeFromIndex(int index);

private:
    Result<void> saveWithValidation(const AppSettings& settings);

    ISettingsRepository& repository_;
    EventHub& eventHub_;
    AppSettings settings_;
};

} // namespace snappaste
