#pragma once

#include "presentation/viewmodels/SettingsViewModel.h"

#include <QWidget>

class QComboBox;
class QLineEdit;

namespace nanosnap {

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
};

} // namespace nanosnap
