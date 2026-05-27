#pragma once

#include <QMainWindow>

namespace snappaste {

class HistoryViewModel;
class SettingsViewModel;
class HistoryWidget;
class SettingsWidget;

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
