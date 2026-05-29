#pragma once

#include "presentation/viewmodels/SettingsViewModel.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLineEdit;

namespace snappaste {

class HotkeyInput;

class SettingsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWidget(SettingsViewModel& viewModel, QWidget* parent = nullptr);

private:
    int themeIndex(ThemeMode mode) const;

    SettingsViewModel& viewModel_;
    QLineEdit* saveDirectoryEdit_ = nullptr;
    QComboBox* imageFormatCombo_ = nullptr;
    QComboBox* themeCombo_ = nullptr;
    QComboBox* ocrLanguageCombo_ = nullptr;
    QComboBox* uiLanguageCombo_ = nullptr;
    QCheckBox* autoSaveCheckbox_ = nullptr;
    HotkeyInput* captureHotkeyInput_ = nullptr;
    HotkeyInput* pasteHotkeyInput_ = nullptr;
    HotkeyInput* hidePinsHotkeyInput_ = nullptr;
    HotkeyInput* repeatCaptureHotkeyInput_ = nullptr;
};

} // namespace snappaste
