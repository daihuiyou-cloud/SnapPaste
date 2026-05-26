#include "presentation/settings/SettingsWidget.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFocusEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QToolTip>
#include <QVBoxLayout>

namespace snappaste {

class HotkeyInput final : public QLabel {
public:
    explicit HotkeyInput(QWidget* parent = nullptr)
        : QLabel(parent)
    {
        setAlignment(Qt::AlignCenter);
        setMinimumWidth(140);
        setCursor(Qt::PointingHandCursor);
        updateStyle();
    }

    Hotkey hotkey() const { return hotkey_; }

    void setHotkey(const Hotkey& hk)
    {
        hotkey_ = hk;
        recording_ = false;
        setText(hotkey_.toDisplayString());
        updateStyle();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        recording_ = !recording_;
        setText(recording_ ? "Press shortcut..." : hotkey_.toDisplayString());
        if (recording_) {
            grabKeyboard();
            setFocus();
        } else {
            releaseKeyboard();
        }
        updateStyle();
        QLabel::mousePressEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (!recording_) {
            QLabel::keyPressEvent(event);
            return;
        }

        const int key = event->key();
        if (key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Shift || key == Qt::Key_Meta) {
            return;
        }

        if (key == Qt::Key_Escape) {
            cancelRecording();
            return;
        }

        hotkey_.ctrl = event->modifiers().testFlag(Qt::ControlModifier);
        hotkey_.alt = event->modifiers().testFlag(Qt::AltModifier);
        hotkey_.shift = event->modifiers().testFlag(Qt::ShiftModifier);

        const int nativeVk = event->nativeVirtualKey();
        hotkey_.key = nativeVk > 0 ? nativeVk : key;

        recording_ = false;
        releaseKeyboard();
        setText(hotkey_.toDisplayString());
        updateStyle();
    }

    void focusOutEvent(QFocusEvent* event) override
    {
        if (recording_) {
            cancelRecording();
        }
        QLabel::focusOutEvent(event);
    }

private:
    void cancelRecording()
    {
        recording_ = false;
        releaseKeyboard();
        setText(hotkey_.toDisplayString());
        updateStyle();
    }

    void updateStyle()
    {
        if (recording_) {
            setStyleSheet(
                "QLabel { background: #2fbf9f; color: #101418;"
                " padding: 4px 12px; border-radius: 4px; font: bold 12px; }");
        } else {
            setStyleSheet(
                "QLabel { background: #1e242a; color: #f4fbff;"
                " padding: 4px 12px; border-radius: 4px;"
                " border: 1px solid #3a4046; font: 12px; }");
        }
    }

    Hotkey hotkey_;
    bool recording_ = false;
};

SettingsWidget::SettingsWidget(SettingsViewModel& viewModel, QWidget* parent)
    : QWidget(parent)
    , viewModel_(viewModel)
    , saveDirectoryEdit_(new QLineEdit(this))
    , imageFormatCombo_(new QComboBox(this))
    , themeCombo_(new QComboBox(this))
    , captureHotkeyInput_(new HotkeyInput(this))
    , pasteHotkeyInput_(new HotkeyInput(this))
    , hidePinsHotkeyInput_(new HotkeyInput(this))
{
    auto* browseButton = new QPushButton("Browse", this);

    auto* saveButton = new QPushButton("Save Settings", this);
    auto* restoreButton = new QPushButton("Restore Defaults", this);

    imageFormatCombo_->addItems({"png", "jpg"});
    themeCombo_->addItems({"System", "Light", "Dark"});

    auto* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(saveDirectoryEdit_);
    pathLayout->addWidget(browseButton);

    auto* form = new QFormLayout();
    form->addRow("Save directory", pathLayout);
    form->addRow("Image format", imageFormatCombo_);
    form->addRow("Theme", themeCombo_);
    form->addRow("Capture hotkey", captureHotkeyInput_);
    form->addRow("Paste hotkey", pasteHotkeyInput_);
    form->addRow("Hide pins hotkey", hidePinsHotkeyInput_);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    auto* btnRow = new QHBoxLayout();
    btnRow->addWidget(saveButton);
    btnRow->addWidget(restoreButton);
    btnRow->addStretch();
    layout->addLayout(btnRow);
    layout->addStretch();
    setLayout(layout);

    connect(browseButton, &QPushButton::clicked, this, [this] {
        const auto directory = QFileDialog::getExistingDirectory(this, "Choose capture directory", saveDirectoryEdit_->text());
        if (!directory.isEmpty()) {
            saveDirectoryEdit_->setText(directory);
        }
    });
    connect(saveButton, &QPushButton::clicked, this, [this] {
        viewModel_.save(saveDirectoryEdit_->text(),
                        imageFormatCombo_->currentText(),
                        themeCombo_->currentIndex(),
                        captureHotkeyInput_->hotkey(),
                        pasteHotkeyInput_->hotkey(),
                        hidePinsHotkeyInput_->hotkey());
    });
    connect(restoreButton, &QPushButton::clicked, this, [this] {
        auto ret = QMessageBox::question(this, "Restore Defaults",
            "Reset all settings to their default values?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            viewModel_.restoreDefaults();
        }
    });
    connect(&viewModel_, &SettingsViewModel::loaded, this,
            [this](const AppSettings& settings) {
                saveDirectoryEdit_->setText(settings.saveDirectory);
                imageFormatCombo_->setCurrentText(settings.imageFormat);
                themeCombo_->setCurrentIndex(themeIndex(settings.themeMode));
                captureHotkeyInput_->setHotkey(settings.captureHotkey);
                pasteHotkeyInput_->setHotkey(settings.pasteHotkey);
                hidePinsHotkeyInput_->setHotkey(settings.hidePinsHotkey);
            });
    connect(&viewModel_, &SettingsViewModel::saved, this, [this, saveButton] {
        QToolTip::showText(saveButton->mapToGlobal(QPoint(saveButton->width() / 2, 0)),
                           "Settings saved", this);
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
