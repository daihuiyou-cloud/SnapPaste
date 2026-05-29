#include "presentation/settings/SettingsWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
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
        setText(recording_ ? tr("Press shortcut...") : hotkey_.toDisplayString());
        if (recording_) {
            timer_.start();
            grabKeyboard();
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
    , uiLanguageCombo_(new QComboBox(this))
    , autoSaveCheckbox_(new QCheckBox(tr("Auto-save on capture"), this))
    , captureHotkeyInput_(new HotkeyInput(this))
    , pasteHotkeyInput_(new HotkeyInput(this))
    , hidePinsHotkeyInput_(new HotkeyInput(this))
    , repeatCaptureHotkeyInput_(new HotkeyInput(this))
{
    auto* browseButton = new QPushButton(tr("Browse"), this);

    auto* saveButton = new QPushButton(tr("Save Settings"), this);
    auto* restoreButton = new QPushButton(tr("Restore Defaults"), this);

    imageFormatCombo_->addItems({"png", "jpg"});
    themeCombo_->addItems({tr("System"), tr("Light"), tr("Dark")});

    struct UiLangEntry { QString label; QString tag; };
    const UiLangEntry kUiLanguages[] = {
        {tr("Auto (System Default)"), QStringLiteral("")},
        {QStringLiteral("English"),   QStringLiteral("en")},
        {QStringLiteral("\xe4\xb8\xad\xe6\x96\x87\xef\xbc\x88\xe7\xae\x80\xe4\xbd\x93\xef\xbc\x89"), QStringLiteral("zh_CN")},
    };
    for (const auto& lang : kUiLanguages) {
        uiLanguageCombo_->addItem(lang.label, lang.tag);
    }

    struct LangEntry { const char* label; const char* tag; };
    const LangEntry kLanguages[] = {
        {QT_TRANSLATE_NOOP("SettingsWidget", "Auto (System Default)"), ""},
        {QT_TRANSLATE_NOOP("SettingsWidget", "English"), "en"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Chinese (Simplified)"), "zh-Hans"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Chinese (Traditional)"), "zh-Hant"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Japanese"), "ja"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Korean"), "ko"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "French"), "fr"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "German"), "de"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Spanish"), "es"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Italian"), "it"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Portuguese (Brazil)"), "pt-BR"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Russian"), "ru"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Arabic"), "ar"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Dutch"), "nl"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Polish"), "pl"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Swedish"), "sv"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Turkish"), "tr"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Czech"), "cs"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Danish"), "da"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Finnish"), "fi"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Greek"), "el"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Hungarian"), "hu"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Norwegian"), "nb"},
        {QT_TRANSLATE_NOOP("SettingsWidget", "Thai"), "th"},
    };
    for (const auto& lang : kLanguages) {
        ocrLanguageCombo_->addItem(QCoreApplication::translate("SettingsWidget", lang.label), QLatin1String(lang.tag));
    }

    auto* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(saveDirectoryEdit_);
    pathLayout->addWidget(browseButton);

    auto* form = new QFormLayout();
    form->addRow(tr("Save directory"), pathLayout);
    form->addRow(tr("Image format"), imageFormatCombo_);
    form->addRow(tr("Theme"), themeCombo_);
    form->addRow(tr("Language"), uiLanguageCombo_);
    form->addRow(tr("OCR language"), ocrLanguageCombo_);
    form->addRow("", autoSaveCheckbox_);
    form->addRow(tr("Capture hotkey"), captureHotkeyInput_);
    form->addRow(tr("Paste hotkey"), pasteHotkeyInput_);
    form->addRow(tr("Hide pins hotkey"), hidePinsHotkeyInput_);
    form->addRow(tr("Repeat capture hotkey"), repeatCaptureHotkeyInput_);

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
        const auto directory = QFileDialog::getExistingDirectory(this, tr("Choose capture directory"), saveDirectoryEdit_->text());
        if (!directory.isEmpty()) {
            saveDirectoryEdit_->setText(directory);
        }
    });
    connect(saveButton, &QPushButton::clicked, this, [this] {
        if (saveDirectoryEdit_->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Validation"), tr("Save directory cannot be empty."));
            saveDirectoryEdit_->setFocus();
            return;
        }

        const Hotkey hotkeys[] = {
            captureHotkeyInput_->hotkey(),
            pasteHotkeyInput_->hotkey(),
            hidePinsHotkeyInput_->hotkey(),
            repeatCaptureHotkeyInput_->hotkey()
        };
        const QString labels[] = {tr("Capture"), tr("Paste"), tr("Hide Pins"), tr("Repeat Capture")};
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) {
                if (hotkeys[i].key > 0 && hotkeys[j].key > 0
                    && hotkeys[i].key == hotkeys[j].key
                    && hotkeys[i].ctrl == hotkeys[j].ctrl
                    && hotkeys[i].alt == hotkeys[j].alt
                    && hotkeys[i].shift == hotkeys[j].shift) {
                    QMessageBox::warning(this, tr("Hotkey Conflict"),
                        tr("\"%1\" and \"%2\" have the same shortcut.")
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
        settings.language = uiLanguageCombo_->currentData().toString();
        settings.autoSaveOnCapture = autoSaveCheckbox_->isChecked();
        settings.repeatCaptureHotkey = hotkeys[3];
        viewModel_.save(settings);
    });
    connect(restoreButton, &QPushButton::clicked, this, [this] {
        auto ret = QMessageBox::question(this, tr("Restore Defaults"),
            tr("Reset all settings to their default values?"),
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
                int uiLangIdx = uiLanguageCombo_->findData(settings.language);
                if (uiLangIdx >= 0) uiLanguageCombo_->setCurrentIndex(uiLangIdx);
                autoSaveCheckbox_->setChecked(settings.autoSaveOnCapture);
                captureHotkeyInput_->setHotkey(settings.captureHotkey);
                pasteHotkeyInput_->setHotkey(settings.pasteHotkey);
                hidePinsHotkeyInput_->setHotkey(settings.hidePinsHotkey);
                repeatCaptureHotkeyInput_->setHotkey(settings.repeatCaptureHotkey);
            });
    connect(&viewModel_, &SettingsViewModel::saved, this, [this, saveButton] {
        const auto original = saveButton->text();
        saveButton->setText(tr("Saved [OK]"));
        saveButton->setStyleSheet("QPushButton { color: #34c759; font-weight: bold; }");
        QTimer::singleShot(2000, this, [saveButton, original] {
            saveButton->setText(original);
            saveButton->setStyleSheet("");
        });
        QToolTip::showText(saveButton->mapToGlobal(QPoint(saveButton->width() / 2, -20)),
                           tr("Settings saved successfully"), this, {}, 1500);
    });
    connect(&viewModel_, &SettingsViewModel::errorOccurred, this, [this, saveButton](const QString& message) {
        QToolTip::showText(saveButton->mapToGlobal(QPoint(saveButton->width() / 2, -20)),
                           tr("Error: ") + message, this, {}, 3000);
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
