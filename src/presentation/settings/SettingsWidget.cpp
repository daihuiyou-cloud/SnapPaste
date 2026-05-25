#include "presentation/settings/SettingsWidget.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace snappaste {

SettingsWidget::SettingsWidget(SettingsViewModel& viewModel, QWidget* parent)
    : QWidget(parent)
    , viewModel_(viewModel)
    , saveDirectoryEdit_(new QLineEdit(this))
    , imageFormatCombo_(new QComboBox(this))
    , themeCombo_(new QComboBox(this))
{
    auto* browseButton = new QPushButton("Browse", this);
    auto* saveButton = new QPushButton("Save Settings", this);
    auto* captureHotkeyLabel = new QLabel(this);
    auto* pasteHotkeyLabel = new QLabel(this);
    auto* hidePinsHotkeyLabel = new QLabel(this);

    imageFormatCombo_->addItems({"png", "jpg"});
    themeCombo_->addItems({"System", "Light", "Dark"});

    auto* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(saveDirectoryEdit_);
    pathLayout->addWidget(browseButton);

    auto* form = new QFormLayout();
    form->addRow("Save directory", pathLayout);
    form->addRow("Image format", imageFormatCombo_);
    form->addRow("Theme", themeCombo_);
    form->addRow("Capture hotkey", captureHotkeyLabel);
    form->addRow("Paste hotkey", pasteHotkeyLabel);
    form->addRow("Hide pins hotkey", hidePinsHotkeyLabel);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(saveButton, 0, Qt::AlignLeft);
    layout->addStretch();
    setLayout(layout);

    connect(browseButton, &QPushButton::clicked, this, [this] {
        const auto directory = QFileDialog::getExistingDirectory(this, "Choose capture directory", saveDirectoryEdit_->text());
        if (!directory.isEmpty()) {
            saveDirectoryEdit_->setText(directory);
        }
    });
    connect(saveButton, &QPushButton::clicked, this, [this] {
        viewModel_.save(saveDirectoryEdit_->text(), imageFormatCombo_->currentText(), themeCombo_->currentIndex());
    });
    connect(&viewModel_, &SettingsViewModel::loaded, this,
            [this, captureHotkeyLabel, pasteHotkeyLabel, hidePinsHotkeyLabel](const AppSettings& settings) {
        saveDirectoryEdit_->setText(settings.saveDirectory);
        imageFormatCombo_->setCurrentText(settings.imageFormat);
        themeCombo_->setCurrentIndex(themeIndex(settings.themeMode));
        captureHotkeyLabel->setText(settings.captureHotkey.toDisplayString());
        pasteHotkeyLabel->setText(settings.pasteHotkey.toDisplayString());
        hidePinsHotkeyLabel->setText(settings.hidePinsHotkey.toDisplayString());
    });
    connect(&viewModel_, &SettingsViewModel::saved, this, [this] {
        QMessageBox::information(this, "SnapPaste", "Settings saved.");
    });
    connect(&viewModel_, &SettingsViewModel::errorOccurred, this, [this](const QString& message) {
        QMessageBox::warning(this, "SnapPaste", message);
    });

    viewModel_.load();
}

int SettingsWidget::themeIndex(ThemeMode mode) const
{
    switch (mode) {
    case ThemeMode::Light:
        return 1;
    case ThemeMode::Dark:
        return 2;
    case ThemeMode::System:
    default:
        return 0;
    }
}

} // namespace snappaste
