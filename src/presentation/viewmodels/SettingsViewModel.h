#pragma once

#include "domain/settings/SettingsService.h"
#include "shared/events/EventHub.h"

#include <QObject>

namespace nanosnap {

class SettingsViewModel final : public QObject {
    Q_OBJECT

public:
    SettingsViewModel(SettingsService& service, EventHub& eventHub, QObject* parent = nullptr);

    AppSettings settings() const;

public slots:
    void load();
    void save(QString saveDirectory, QString imageFormat, int themeIndex);

signals:
    void loaded(const AppSettings& settings);
    void saved();
    void errorOccurred(const QString& message);

private:
    static ThemeMode themeFromIndex(int index);

    SettingsService& service_;
    EventHub& eventHub_;
    AppSettings settings_;
};

} // namespace nanosnap
