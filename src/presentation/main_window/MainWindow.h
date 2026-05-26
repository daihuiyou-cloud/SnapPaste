#pragma once

#include "presentation/history/HistoryWidget.h"
#include "presentation/settings/SettingsWidget.h"

#include <QMainWindow>

namespace snappaste {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(HistoryViewModel& historyViewModel,
               SettingsViewModel& settingsViewModel,
               QWidget* parent = nullptr);

signals:
    void captureRequested();
    void repinRequested(const QString& filePath);
};

} // namespace snappaste
