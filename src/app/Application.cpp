#include "app/Application.h"

#include "app/AppStartup.h"
#include "infrastructure/ocr/WindowsOcrService.h"
#include "presentation/ocr/OcrResultWindow.h"
#include "presentation/viewmodels/CaptureViewModel.h"
#include "presentation/viewmodels/HistoryViewModel.h"
#include "presentation/viewmodels/SettingsViewModel.h"
#include "shared/events/EventHub.h"

#include <climits>

#include <QPointer>

#include <QMessageBox>
#include <QClipboard>
#include <QCursor>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QUrl>
#include <QMetaObject>
#include <QScreen>
#include <QSignalBlocker>
#include <QStringList>
#include <QTimer>

#include <algorithm>
#include <cstring>
#include <thread>

namespace snappaste {

namespace {

constexpr int kCaptureAfterHideDelayMs = 16;
constexpr int kPinBaseOffset = 16;

} // namespace

Application::Application(QApplication& qtApplication, ILogger& logger)
    : QObject(&qtApplication)
    , logger_(logger)
    , qtApplication_(qtApplication)
    , trayController_(context_.iconProvider(), this)
    , toastNotifier_(this)
    , pinManager_(context_.iconProvider(), context_.pinViewModel(), this)
    , alive_(std::make_shared<std::atomic<bool>>(true))
{
    QApplication::setQuitOnLastWindowClosed(false);
}

Application::~Application()
{
    *alive_ = false;
}

int Application::run()
{
    logger_.install();
    applyCurrentTheme();
    ocrService_ = std::make_unique<WindowsOcrService>(logger_);
    if (cachedSettings_) {
        ocrService_->setLanguage(cachedSettings_->ocrLanguage);
    }
    connectCoreSignals();

    trayController_.show();
    registerHotkey();
    context_.pinViewModel().restore();

    const auto exitCode = qtApplication_.exec();

    return exitCode;
}

void Application::connectCoreSignals()
{
    connect(&trayController_, &TrayController::captureRequested, this, &Application::startCapture);
    connect(&trayController_, &TrayController::openFileRequested, this, &Application::openFile);
    connect(&trayController_, &TrayController::showWindowRequested, this, &Application::showMainWindow);
    connect(&trayController_, &TrayController::hidePinsRequested, this, &Application::hideAllPins);
    connect(&trayController_, &TrayController::showPinsRequested, this, &Application::showAllPins);
    connect(&trayController_, &TrayController::closeAllPinsRequested, this, [this] {
        pinManager_.closeAll();
        showStatus(tr("All pinned images closed."));
    });
    connect(&trayController_, &TrayController::quitRequested, &qtApplication_, &QApplication::quit);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, [this] {
        preferLastPinnableImage_ = false;
    });

    QPointer<Application> guard(this);
    context_.hotkeyService().setActionCallback(HotkeyAction::Capture, [guard] {
        if (guard) QMetaObject::invokeMethod(guard, [guard] { if (guard) guard->startCapture(); }, Qt::QueuedConnection);
    });
    context_.hotkeyService().setActionCallback(HotkeyAction::Paste, [guard] {
        if (guard) QMetaObject::invokeMethod(guard, [guard] { if (guard) guard->pasteFromClipboard(); }, Qt::QueuedConnection);
    });
    context_.hotkeyService().setActionCallback(HotkeyAction::HideAllPins, [guard] {
        if (guard) QMetaObject::invokeMethod(guard, [guard] { if (guard) guard->hideAllPins(); }, Qt::QueuedConnection);
    });
    context_.hotkeyService().setActionCallback(HotkeyAction::RepeatCapture, [guard] {
        if (guard) QMetaObject::invokeMethod(guard, [guard] { if (guard) guard->repeatLastCapture(); }, Qt::QueuedConnection);
    });

    connect(&context_.captureViewModel(), &CaptureViewModel::errorOccurred, this, [this](const QString& message) {
        showStatus(message);
    });
    connect(&context_.captureViewModel(), &CaptureViewModel::copied, this, [this] {
        showStatus(tr("Screenshot copied. Press %1 to pin.")
            .arg(cachedSettings_ ? hotkeyDisplayString(cachedSettings_->pasteHotkey, "F3") : QStringLiteral("F3")), [this] {
            showMainWindow();
        });
    });
    connect(&context_.captureViewModel(), &CaptureViewModel::saved, this, [this](const QString& filePath) {
        showStatus(tr("Saved %1 - Click to open").arg(QFileInfo(filePath).fileName()), [filePath] {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        });
    });
    connect(&context_.pinViewModel(), &PinViewModel::pinCreated, this, [this](const PinnedItem& item) {
        if (!item.image.isNull()) {
            lastPinnableImage_ = item.image;
            lastPinnableSource_ = item.source;
            preferLastPinnableImage_ = true;
        }
        const auto pos = pendingPinPosition_;
        const auto avoid = pendingPinAvoidRegion_;
        pendingPinPosition_.reset();
        pendingPinAvoidRegion_.reset();
        pinManager_.openPinWindow(item, pos, avoid);
        showStatus(tr("Pinned image created. Press %1 to repeat.")
            .arg(cachedSettings_ ? hotkeyDisplayString(cachedSettings_->repeatCaptureHotkey, "F4") : QStringLiteral("F4")));
    });
    connect(&context_.pinViewModel(), &PinViewModel::pinRestored, this, [this](const PinnedItem& item) {
        pinManager_.openPinWindow(item);
    });
    connect(&context_.pinViewModel(), &PinViewModel::errorOccurred, this, [this](const QString& message) {
        pendingPinPosition_.reset();
        pendingPinAvoidRegion_.reset();
        showStatus(message);
    });

    connect(&pinManager_, &PinManager::copyRequested, this, [this](qint64 /*id*/, const QImage& image, PinSource source) {
        lastPinnableImage_ = image;
        lastPinnableSource_ = source;
        preferLastPinnableImage_ = true;
        const QSignalBlocker blocker(QApplication::clipboard());
        QApplication::clipboard()->setImage(image);
        showStatus(tr("Pinned image copied. Press %1 to repeat.")
            .arg(cachedSettings_ ? hotkeyDisplayString(cachedSettings_->repeatCaptureHotkey, "F4") : QStringLiteral("F4")));
    });
    connect(&pinManager_, &PinManager::saveRequested, this, [this](const QImage& image) {
        context_.captureViewModel().saveImage(image, "pin");
    });

    QPointer<Application> appGuard(this);
    connect(&pinManager_, &PinManager::ocrRequested, this, [this, appGuard](qint64 /*id*/, const QImage& image) {
        if (image.isNull()) {
            showStatus(tr("No image available for OCR."));
            return;
        }
        if (ocrService_) {
            ocrService_->cancel();
        }
        auto weakAlive = alive_;
        QPointer<Application> guard(this);
        QApplication::setOverrideCursor(Qt::WaitCursor);
        showStatus(tr("OCR processing..."));

        ocrService_->recognizeTextAsync(image, [weakAlive, guard](OcrResult outcome) {
            if (!*weakAlive || guard.isNull()) return;
            QApplication::restoreOverrideCursor();
            if (!outcome.ok) {
                guard->showStatus(outcome.message);
                return;
            }

            auto* win = new OcrResultWindow(std::move(outcome.image), std::move(outcome.blocks), outcome.text, nullptr);
            guard->ocrWindow_ = win;
#pragma warning(push)
#pragma warning(disable: 4573)
            QObject::connect(win, &QObject::destroyed, win, [guard] {
                if (guard) guard->ocrWindow_.clear();
            });
            QObject::connect(win, &OcrResultWindow::pasteRequested, win, [guard] { if (guard) guard->pasteFromClipboard(); });
#pragma warning(pop)
            guard->showStatus(
                tr("OCR - %1 characters").arg(outcome.text.length()));
        });
    });

    connect(&context_.eventHub(), &EventHub::historyChanged, &context_.historyViewModel(), &HistoryViewModel::refresh);
    connect(&context_.eventHub(), &EventHub::settingsChanged, this, [this] {
        invalidateSettingsCache();
        applyCurrentTheme();
        registerHotkey();
        if (ocrService_ && cachedSettings_) {
            ocrService_->setLanguage(cachedSettings_->ocrLanguage);
        }
    });
}

void Application::showMainWindow()
{
    if (!mainWindow_) {
        mainWindow_ = std::make_unique<MainWindow>(context_.historyViewModel(), context_.settingsViewModel());
        connect(mainWindow_.get(), &MainWindow::captureRequested, this, &Application::startCapture);
        connect(mainWindow_.get(), &MainWindow::repinRequested, this, [this](const QString& filePath) {
            QImage img(filePath);
            if (!img.isNull()) {
                const auto dpr = context_.historyViewModel().devicePixelRatio(filePath);
                img.setDevicePixelRatio(dpr);
                context_.pinViewModel().createFromImage(img, PinSource::Screenshot);
            }
        });
    }

    mainWindow_->show();
    mainWindow_->raise();
    mainWindow_->activateWindow();
}

void Application::startCapture()
{
    toastNotifier_.hide();
    auto& captureOverlay = overlay();
    captureOverlay.prepareForCapture();
    captureOverlay.showForCapture();
}

void Application::openFile()
{
    const auto path = QFileDialog::getOpenFileName(
        mainWindow_.get(), tr("Open Image"), QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;All Files (*)"));
    if (path.isEmpty()) {
        return;
    }

    QImage image(path);
    if (image.isNull()) {
        showStatus(tr("Failed to open image: %1").arg(QFileInfo(path).fileName()));
        return;
    }

    image.setDevicePixelRatio(context_.historyViewModel().devicePixelRatio(path));
    context_.pinViewModel().createFromImage(image, PinSource::File);
}

void Application::pasteFromClipboard()
{
    pendingPinPosition_ = QCursor::pos() + QPoint(kPinBaseOffset, kPinBaseOffset);
    pendingPinAvoidRegion_.reset();
    if (preferLastPinnableImage_ && !lastPinnableImage_.isNull()) {
        context_.pinViewModel().createFromImage(lastPinnableImage_, lastPinnableSource_);
        return;
    }
    context_.pinViewModel().createFromClipboard();
}

void Application::hideAllPins()
{
    pinManager_.hideAll();
    showStatus(tr("Pinned images hidden."));
}

void Application::showAllPins()
{
    pinManager_.showAll();
    showStatus(tr("Pinned images restored."));
}

void Application::copyRegion(const QRect& region)
{
    captureAfterOverlayHidden(region, [this](const QImage&) {
        const QSignalBlocker blocker(QApplication::clipboard());
        context_.captureViewModel().copyCurrentImageToClipboard();
    });
}

void Application::pinRegion(const QRect& region)
{
    captureAfterOverlayHidden(region, [this, region](const QImage& image) {
        if (image.isNull()) {
            showStatus(tr("Failed to capture image for pinning."));
            return;
        }
        pendingPinPosition_ = QCursor::pos() + QPoint(kPinBaseOffset, kPinBaseOffset);
        pendingPinAvoidRegion_ = region;
        context_.pinViewModel().createFromImage(image, PinSource::Screenshot);
    });
}

void Application::saveRegion(const QRect& region)
{
    captureAfterOverlayHidden(region, [this](const QImage&) {
        context_.captureViewModel().saveCurrentImage();
    });
}

void Application::ocrRegion(const QRect& region)
{
    captureAfterOverlayHidden(region, [this](const QImage& image) {
        if (image.isNull()) {
            showStatus(tr("Failed to capture image for OCR."));
            return;
        }
        if (ocrService_) {
            ocrService_->cancel();
        }
        auto weakAlive = alive_;
        QPointer<Application> guard(this);
        QApplication::setOverrideCursor(Qt::WaitCursor);
        showStatus(tr("OCR processing..."));

        ocrService_->recognizeTextAsync(image, [weakAlive, guard](OcrResult outcome) {
            if (!*weakAlive || guard.isNull()) return;
            QApplication::restoreOverrideCursor();
            if (!outcome.ok) {
                guard->showStatus(outcome.message);
                return;
            }

            auto* win = new OcrResultWindow(std::move(outcome.image), std::move(outcome.blocks), outcome.text, nullptr);
            guard->ocrWindow_ = win;
#pragma warning(push)
#pragma warning(disable: 4573)
            QObject::connect(win, &QObject::destroyed, win, [guard] {
                if (guard) guard->ocrWindow_.clear();
            });
            QObject::connect(win, &OcrResultWindow::pasteRequested, win, [guard] { if (guard) guard->pasteFromClipboard(); });
#pragma warning(pop)
            guard->showStatus(
                tr("OCR - %1 characters").arg(outcome.text.length()));
        });
    });
}

void Application::showStatus(QString message, std::function<void()> onClick)
{
    toastNotifier_.showMessage(std::move(message), QPoint(), std::move(onClick));
}

void Application::repeatLastCapture()
{
    if (!lastCaptureRegion_.has_value()) {
        startCapture();
        return;
    }
    const auto region = lastCaptureRegion_.value();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    context_.captureViewModel().captureRegionAsync(region, [this](const QImage& image) {
        QApplication::restoreOverrideCursor();
        if (image.isNull()) return;
        lastPinnableImage_ = image;
        lastPinnableSource_ = PinSource::Screenshot;
        preferLastPinnableImage_ = true;
        if (cachedSettings_ && cachedSettings_->autoSaveOnCapture) {
            context_.captureViewModel().saveImage(image, "capture");
        }
        const QSignalBlocker blocker(QApplication::clipboard());
        context_.captureViewModel().copyCurrentImageToClipboard();
    });
}

void Application::captureAfterOverlayHidden(const QRect& region, std::function<void(const QImage&)> onReady)
{
    lastCaptureRegion_ = region;
    overlay().hide();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QPointer<Application> guard(this);
    QTimer::singleShot(kCaptureAfterHideDelayMs, this, [guard, region, onReady = std::move(onReady)]() mutable {
        if (guard.isNull()) return;
        QApplication::restoreOverrideCursor();
        guard->context_.captureViewModel().captureRegionAsync(region, [guard, onReady = std::move(onReady)](const QImage& image) mutable {
            if (guard.isNull()) return;
            if (!image.isNull()) {
                guard->lastPinnableImage_ = image;
                guard->lastPinnableSource_ = PinSource::Screenshot;
                guard->preferLastPinnableImage_ = true;
                if (guard->cachedSettings_ && guard->cachedSettings_->autoSaveOnCapture) {
                    guard->context_.captureViewModel().saveImage(image, "capture");
                }
            }
            if (onReady) {
                onReady(image);
            }
        });
    });
}

QString Application::hotkeyDisplayString(const Hotkey& hk, const char* fallback) const
{
    if (hk.key > 0) {
        return hk.toDisplayString();
    }
    return QString::fromLatin1(fallback);
}

void Application::ensureSettingsCached()
{
    if (cachedSettings_.has_value()) {
        return;
    }
    const auto settingsResult = context_.settingsRepository().load();
    if (settingsResult.isError()) {
        logger_.warning(settingsResult.error());
        return;
    }
    cachedSettings_ = settingsResult.value();
}

void Application::invalidateSettingsCache()
{
    cachedSettings_.reset();
}

void Application::registerHotkey()
{
    ensureSettingsCached();
    if (!cachedSettings_.has_value()) {
        return;
    }
    const auto& settings = cachedSettings_.value();

    context_.hotkeyService().unregisterAll();
    if (!context_.hotkeyService().registerHotkey(HotkeyAction::Capture, settings.captureHotkey)) {
        const auto message = tr("Failed to register capture hotkey: %1").arg(settings.captureHotkey.toDisplayString());
        logger_.warning(message);
        trayController_.showMessage(tr("SnapPaste"), message);
    }
    if (!context_.hotkeyService().registerHotkey(HotkeyAction::Paste, settings.pasteHotkey)) {
        const auto message = tr("Failed to register paste hotkey: %1").arg(settings.pasteHotkey.toDisplayString());
        logger_.warning(message);
        trayController_.showMessage(tr("SnapPaste"), message);
    }
    if (!context_.hotkeyService().registerHotkey(HotkeyAction::HideAllPins, settings.hidePinsHotkey)) {
        const auto message = tr("Failed to register hide-pins hotkey: %1").arg(settings.hidePinsHotkey.toDisplayString());
        logger_.warning(message);
        trayController_.showMessage(tr("SnapPaste"), message);
    }
    if (!context_.hotkeyService().registerHotkey(HotkeyAction::RepeatCapture, settings.repeatCaptureHotkey)) {
        const auto message = tr("Failed to register repeat-capture hotkey: %1").arg(settings.repeatCaptureHotkey.toDisplayString());
        logger_.warning(message);
        trayController_.showMessage(tr("SnapPaste"), message);
    }
}

void Application::applyCurrentTheme()
{
    ensureSettingsCached();
    if (!cachedSettings_.has_value()) {
        return;
    }

    const auto themeResult = AppStartup::applyTheme(
        qtApplication_, cachedSettings_.value(), context_.platformService());
    if (themeResult.isError()) {
        logger_.warning(themeResult.error());
    }
}

CaptureOverlay& Application::overlay()
{
    if (!overlay_) {
        overlay_ = std::make_unique<CaptureOverlay>(
            context_.iconProvider(),
            context_.screenRegionDetector(),
            context_.screenPixelSampler(),
            context_.captureSelectionHistory());
        connect(overlay_.get(), &CaptureOverlay::copyRequested, this, &Application::copyRegion);
        connect(overlay_.get(), &CaptureOverlay::pinRequested, this, &Application::pinRegion);
        connect(overlay_.get(), &CaptureOverlay::saveRequested, this, &Application::saveRegion);
        connect(overlay_.get(), &CaptureOverlay::cancelled, this, [this] {
            pendingPinPosition_.reset();
            pendingPinAvoidRegion_.reset();
        });
    }
    return *overlay_;
}

} // namespace snappaste
