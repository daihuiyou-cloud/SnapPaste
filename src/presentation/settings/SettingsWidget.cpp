#include "presentation/settings/SettingsWidget.h"

#include <QCheckBox>
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
#include <QTimer>
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
        timer_.setSingleShot(true);
        timer_.setInterval(10000);
        connect(&timer_, &QTimer::timeout, this, [this] { cancelRecording(); });
    }

    Hotkey hotkey() const { return hotkey_; }

    void setHotkey(const Hotkey& hk)
    {
        hotkey_ = hk;
        recording_ = false;
        timer_.stop();
        setText(hotkey_.toDisplayString());
        updateStyle();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        recording_ = !recording_;
        setText(recording_ ? "Press shortcut..." : hotkey_.toDisplayString());
        if (recording_) {
            timer_.start();
            if (!grabKeyboard()) {
                recording_ = false;
                timer_.stop();
                setText(hotkey_.toDisplayString());
                updateStyle();
                return;
            }
            setFocus();
        } else {
            timer_.stop();
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
        timer_.stop();
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
        timer_.stop();
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

    QTimer timer_;
    Hotkey hotkey_;
    bool recording_ = false;
};

SettingsWidget::SettingsWidget(SettingsViewModel& viewModel, QWidget* parent)
    : QWidget(parent)
    , viewModel_(viewModel)
    , saveDirectoryEdit_(new QLineEdit(this))
    , imageFormatCombo_(new QComboBox(this))
    , themeCombo_(new QComboBox(this))
    , ocrLanguageCombo_(new QComboBox(this))
    , autoSaveCheckbox_(new QCheckBox("Auto-save on capture", this))
    , captureHotkeyInput_(new HotkeyInput(this))
    , pasteHotkeyInput_(new HotkeyInput(this))
    , hidePinsHotkeyInput_(new HotkeyInput(this))
    , repeatCaptureHotkeyInput_(new HotkeyInput(this))
{
    auto* browseButton = new QPushButton("Browse", this);

    auto* saveButton = new QPushButton("Save Settings", this);
    auto* restoreButton = new QPushButton("Restore Defaults", this);

    imageFormatCombo_->addItems({"png", "jpg"});
    themeCombo_->addItems({"System", "Light", "Dark"});

    struct LangEntry { const char* label; const char* tag; };
    const LangEntry kLanguages[] = {
        {"Auto (System Default)", ""},
        {"English", "en"},
        {"Chinese (Simplified)", "zh-Hans"},
        {"Chinese (Traditional)", "zh-Hant"},
        {"Japanese", "ja"},
        {"Korean", "ko"},
        {"French", "fr"},
        {"German", "de"},
        {"Spanish", "es"},
        {"Italian", "it"},
        {"Portuguese (Brazil)", "pt-BR"},
        {"Russian", "ru"},
        {"Arabic", "ar"},
        {"Dutch", "nl"},
        {"Polish", "pl"},
        {"Swedish", "sv"},
        {"Turkish", "tr"},
        {"Czech", "cs"},
        {"Danish", "da"},
        {"Finnish", "fi"},
        {"Greek", "el"},
        {"Hungarian", "hu"},
        {"Norwegian", "nb"},
        {"Thai", "th"},
    };
    for (const auto& lang : kLanguages) {
        ocrLanguageCombo_->addItem(QLatin1String(lang.label), QLatin1String(lang.tag));
    }

    auto* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(saveDirectoryEdit_);
    pathLayout->addWidget(browseButton);

    auto* form = new QFormLayout();
    form->addRow("Save directory", pathLayout);
    form->addRow("Image format", imageFormatCombo_);
    form->addRow("Theme", themeCombo_);
    form->addRow("OCR language", ocrLanguageCombo_);
    form->addRow("", autoSaveCheckbox_);
    form->addRow("Capture hotkey", captureHotkeyInput_);
    form->addRow("Paste hotkey", pasteHotkeyInput_);
    form->addRow("Hide pins hotkey", hidePinsHotkeyInput_);
    form->addRow("Repeat capture hotkey", repeatCaptureHotkeyInput_);

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
        if (saveDirectoryEdit_->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validation", "Save directory cannot be empty.");
            saveDirectoryEdit_->setFocus();
            return;
        }

        const Hotkey hotkeys[] = {
            captureHotkeyInput_->hotkey(),
            pasteHotkeyInput_->hotkey(),
            hidePinsHotkeyInput_->hotkey(),
            repeatCaptureHotkeyInput_->hotkey()
        };
        const char* labels[] = {"Capture", "Paste", "Hide Pins", "Repeat Capture"};
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) {
                if (hotkeys[i].key > 0 && hotkeys[j].key > 0
                    && hotkeys[i].key == hotkeys[j].key
                    && hotkeys[i].ctrl == hotkeys[j].ctrl
                    && hotkeys[i].alt == hotkeys[j].alt
                    && hotkeys[i].shift == hotkeys[j].shift) {
                    QMessageBox::warning(this, "Hotkey Conflict",
                        QString("\"%1\" and \"%2\" have the same shortcut.")
                            .arg(labels[i], labels[j]));
                    return;
                }
            }
        }

        AppSettings settings;
        settings.saveDirectory = saveDirectoryEdit_->text().trimmed();
        settings.imageFormat = imageFormatCombo_->currentText();
        settings.themeMode = SettingsViewModel::themeFromIndex(themeCombo_->currentIndex());
        settings.captureHotkey = hotkeys[0];
        settings.pasteHotkey = hotkeys[1];
        settings.hidePinsHotkey = hotkeys[2];
        settings.ocrLanguage = ocrLanguageCombo_->currentData().toString();
        settings.autoSaveOnCapture = autoSaveCheckbox_->isChecked();
        settings.repeatCaptureHotkey = hotkeys[3];
        viewModel_.save(settings);
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
                int langIdx = ocrLanguageCombo_->findData(settings.ocrLanguage);
                if (langIdx >= 0) ocrLanguageCombo_->setCurrentIndex(langIdx);
                autoSaveCheckbox_->setChecked(settings.autoSaveOnCapture);
                captureHotkeyInput_->setHotkey(settings.captureHotkey);
                pasteHotkeyInput_->setHotkey(settings.pasteHotkey);
                hidePinsHotkeyInput_->setHotkey(settings.hidePinsHotkey);
                repeatCaptureHotkeyInput_->setHotkey(settings.repeatCaptureHotkey);
            });
    connect(&viewModel_, &SettingsViewModel::saved, this, [this, saveButton] {
        const auto original = saveButton->text();
        saveButton->setText("Saved [OK]");
        saveButton->setStyleSheet("QPushButton { color: #34c759; font-weight: bold; }");
        QTimer::singleShot(2000, this, [saveButton, original] {
            saveButton->setText(original);
            saveButton->setStyleSheet("");
        });
        QToolTip::showText(saveButton->mapToGlobal(QPoint(saveButton->width() / 2, -20)),
                           "Settings saved successfully", this, {}, 1500);
    });
    connect(&viewModel_, &SettingsViewModel::errorOccurred, this, [this, saveButton](const QString& message) {
        QToolTip::showText(saveButton->mapToGlobal(QPoint(saveButton->width() / 2, -20)),
                           "Error: " + message, this, {}, 3000);
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
