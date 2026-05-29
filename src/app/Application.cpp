#include "app/Application.h"

#include "app/AppStartup.h"
#include "infrastructure/logging/Logger.h"
#include "infrastructure/ocr/WindowsOcrService.h"
#include "presentation/ocr/OcrResultWindow.h"

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
constexpr int kPinCascadeOffset = 24;
constexpr int kPinCascadeSlots = 8;

} // namespace

Application::Application(QApplication& qtApplication)
    : QObject(&qtApplication)
    , qtApplication_(qtApplication)
    , trayController_(this)
    , toastNotifier_(this)
{
    QApplication::setQuitOnLastWindowClosed(false);
}

int Application::run()
{
    Logger::install();
    applyCurrentTheme();
    ocrService_ = std::make_unique<WindowsOcrService>();
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
        for (auto it = pinWindows_.begin(); it != pinWindows_.end(); it = pinWindows_.begin()) {
            if (it->second) {
                context_.pinViewModel().close(it->first);
                pinWindows_.erase(it);
            }
        }
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
        showStatus(tr("Screenshot copied. Press F3 to pin."), [this] {
            showMainWindow();
        });
    });
    connect(&context_.captureViewModel(), &CaptureViewModel::saved, this, [this](const QString& filePath) {
        showStatus(tr("Saved %1 \u2192 Click to open").arg(QFileInfo(filePath).fileName()), [filePath] {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        });
    });
    connect(&context_.pinViewModel(), &PinViewModel::pinCreated, this, [this](const PinnedItem& item) {
        if (!item.image.isNull()) {
            lastPinnableImage_ = item.image;
            lastPinnableSource_ = item.source;
            preferLastPinnableImage_ = true;
        }
        openPinWindow(item);
        showStatus(tr("Pinned image created. Press F3 to repeat."));
    });
    connect(&context_.pinViewModel(), &PinViewModel::pinRestored, this, [this](const PinnedItem& item) {
        openPinWindow(item);
    });
    connect(&context_.pinViewModel(), &PinViewModel::errorOccurred, this, [this](const QString& message) {
        pendingPinPosition_.reset();
        pendingPinAvoidRegion_.reset();
        showStatus(message);
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
    pendingPinPosition_ = cascadedPinPosition(QCursor::pos() + QPoint(kPinBaseOffset, kPinBaseOffset));
    pendingPinAvoidRegion_.reset();
    if (preferLastPinnableImage_ && !lastPinnableImage_.isNull()) {
        context_.pinViewModel().createFromImage(lastPinnableImage_, lastPinnableSource_);
        return;
    }
    context_.pinViewModel().createFromClipboard();
}

void Application::hideAllPins()
{
    context_.pinViewModel().setAllVisible(false);
    for (auto& entry : pinWindows_) {
        if (entry.second) {
            entry.second->setPinnedVisible(false);
        }
    }
    showStatus(tr("Pinned images hidden."));
}

void Application::showAllPins()
{
    context_.pinViewModel().setAllVisible(true);
    for (auto& entry : pinWindows_) {
        if (entry.second) {
            entry.second->restoreInteraction();
        }
    }
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
        pendingPinPosition_ = cascadedPinPosition(QCursor::pos() + QPoint(kPinBaseOffset, kPinBaseOffset));
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

void Application::editRegion(const QRect& region)
{
    captureAfterOverlayHidden(region, [this](const QImage& image) {
        if (image.isNull()) {
            showStatus(tr("Failed to capture image for editing."));
            return;
        }
        editorWindow().setImage(image);
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
        auto ocrService = ocrService_;
        QPointer<Application> guard(this);
        QApplication::setOverrideCursor(Qt::WaitCursor);
        showStatus(tr("OCR processing..."));
        std::thread worker([guard, ocrService, image]() {
            if (guard.isNull()) return;
            const auto outcome = ocrService->recognizeText(image);
            QMetaObject::invokeMethod(qApp, [guard, outcome]() {
                if (guard.isNull()) return;
                QApplication::restoreOverrideCursor();
                if (!outcome.ok) {
                    guard->showStatus(outcome.message);
                    return;
                }

                auto* win = new OcrResultWindow(outcome.image, outcome.blocks, outcome.text, nullptr);
                guard->ocrWindow_ = win;
#pragma warning(push)
#pragma warning(disable : 4573)
                QObject::connect(win, &QObject::destroyed, [guard] {
                    if (guard) guard->ocrWindow_.clear();
                });
                QObject::connect(win, &OcrResultWindow::pasteRequested, [guard] { if (guard) guard->pasteFromClipboard(); });
#pragma warning(pop)
                guard->showStatus(
                    tr("OCR \u2192 %1 characters").arg(outcome.text.length()));
            }, Qt::QueuedConnection);
        });
        worker.detach();
    });
}

void Application::showStatus(const QString& message, std::function<void()> onClick)
{
    toastNotifier_.showMessage(message, QPoint(), std::move(onClick));
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

void Application::ensureSettingsCached()
{
    if (cachedSettings_.has_value()) {
        return;
    }
    const auto settingsResult = context_.settingsService().load();
    if (settingsResult.isError()) {
        Logger::warning(settingsResult.error());
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
        Logger::warning(message);
        trayController_.showMessage(tr("SnapPaste"), message);
    }
    if (!context_.hotkeyService().registerHotkey(HotkeyAction::Paste, settings.pasteHotkey)) {
        const auto message = tr("Failed to register paste hotkey: %1").arg(settings.pasteHotkey.toDisplayString());
        Logger::warning(message);
        trayController_.showMessage(tr("SnapPaste"), message);
    }
    if (!context_.hotkeyService().registerHotkey(HotkeyAction::HideAllPins, settings.hidePinsHotkey)) {
        const auto message = tr("Failed to register hide-pins hotkey: %1").arg(settings.hidePinsHotkey.toDisplayString());
        Logger::warning(message);
        trayController_.showMessage(tr("SnapPaste"), message);
    }
    if (!context_.hotkeyService().registerHotkey(HotkeyAction::HideAllPins, settings.hidePinsHotkey)) {
        const auto message = tr("Failed to register hide-pins hotkey: %1").arg(settings.hidePinsHotkey.toDisplayString());
        Logger::warning(message);
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
        qtApplication_, cachedSettings_.value(), context_.darkModeDetector());
    if (themeResult.isError()) {
        Logger::warning(themeResult.error());
    }
}

void Application::openPinWindow(PinnedItem item)
{
    if (item.image.isNull()) {
        Logger::warning("Ignoring empty pinned image.");
        pendingPinPosition_.reset();
        pendingPinAvoidRegion_.reset();
        return;
    }

    if (pendingPinPosition_.has_value()) {
        item.state.position = pinnedPositionFor(item.image.size(), pendingPinPosition_.value(), pendingPinAvoidRegion_);
        context_.pinViewModel().updateState(item.id, item.state);
        pendingPinPosition_.reset();
        pendingPinAvoidRegion_.reset();
    }

    auto pinWindow = std::make_unique<PinWindow>(item);
    auto* window = pinWindow.get();
    pinWindows_[item.id] = std::move(pinWindow);

    connect(window, &PinWindow::stateChanged, &context_.pinViewModel(), &PinViewModel::updateState);
    connect(window, &PinWindow::closeRequested, this, [this](qint64 id) {
        context_.pinViewModel().close(id);
        QPointer<Application> guard(this);
        QTimer::singleShot(0, this, [guard, id] {
            if (guard) {
                guard->pinWindows_.erase(id);
            }
        });
    });
    connect(window, &PinWindow::copyRequested, this, [this, source = item.source](const QImage& image) {
        if (image.isNull()) {
            showStatus(tr("No pinned image is available to copy."));
            return;
        }
        lastPinnableImage_ = image;
        lastPinnableSource_ = source;
        preferLastPinnableImage_ = true;
        const QSignalBlocker blocker(QApplication::clipboard());
        QApplication::clipboard()->setImage(image);
        showStatus(tr("Pinned image copied. Press F3 to repeat."));
    });
    connect(window, &PinWindow::saveRequested, this, [this](const QImage& image) {
        context_.captureViewModel().saveImage(image, "pin");
    });
    if (window->state().options.visible) {
        window->show();
        window->raise();
    }
}

QPoint Application::cascadedPinPosition(const QPoint& basePosition)
{
    const auto slot = (nextPinSlot_++) % kPinCascadeSlots;
    const auto offset = slot * kPinCascadeOffset;
    return basePosition + QPoint(offset, offset);
}

QPoint Application::pinnedPositionFor(const QSize& imageSize,
                                      const QPoint& preferredPosition,
                                      const std::optional<QRect>& avoidRegion) const
{
    constexpr int kMargin = 12;
    auto position = preferredPosition;
    const auto screen = QGuiApplication::screenAt(preferredPosition);
    const auto fallback = QGuiApplication::primaryScreen();
    const auto bounds = screen != nullptr ? screen->availableGeometry()
        : fallback != nullptr ? fallback->availableGeometry()
        : QRect(0, 0, 1920, 1080);
    QRect pinRect(position, imageSize);

    if (avoidRegion.has_value() && pinRect.intersects(avoidRegion.value())) {
        position = QPoint(avoidRegion->right() + kMargin, avoidRegion->top());
        pinRect.moveTopLeft(position);
        if (!bounds.contains(pinRect)) {
            position = QPoint(avoidRegion->left(), avoidRegion->bottom() + kMargin);
            pinRect.moveTopLeft(position);
        }
    }

    if (pinRect.right() > bounds.right() - kMargin) {
        position.setX(bounds.right() - imageSize.width() - kMargin);
    }
    if (pinRect.bottom() > bounds.bottom() - kMargin) {
        position.setY(bounds.bottom() - imageSize.height() - kMargin);
    }
    if (position.x() < bounds.left() + kMargin) {
        position.setX(bounds.left() + kMargin);
    }
    if (position.y() < bounds.top() + kMargin) {
        position.setY(bounds.top() + kMargin);
    }

    return position;
}

CaptureOverlay& Application::overlay()
{
    if (!overlay_) {
        overlay_ = std::make_unique<CaptureOverlay>(
            context_.screenRegionDetector(),
            context_.screenPixelSampler(),
            context_.captureSelectionHistory());
        connect(overlay_.get(), &CaptureOverlay::copyRequested, this, &Application::copyRegion);
        connect(overlay_.get(), &CaptureOverlay::pinRequested, this, &Application::pinRegion);
        connect(overlay_.get(), &CaptureOverlay::saveRequested, this, &Application::saveRegion);
        connect(overlay_.get(), &CaptureOverlay::editRequested, this, &Application::editRegion);
        connect(overlay_.get(), &CaptureOverlay::ocrRequested, this, &Application::ocrRegion);
        connect(overlay_.get(), &CaptureOverlay::cancelled, this, [this] {
            pendingPinPosition_.reset();
            pendingPinAvoidRegion_.reset();
        });
    }
    return *overlay_;
}

EditorWindow& Application::editorWindow()
{
    if (!editorWindow_) {
        editorWindow_ = std::make_unique<EditorWindow>();
        connect(editorWindow_.get(), &EditorWindow::imageEdited,
                &context_.captureViewModel(), &CaptureViewModel::setCurrentImage);
        connect(editorWindow_.get(), &EditorWindow::saveRequested,
                &context_.captureViewModel(), &CaptureViewModel::saveCurrentImage);
        connect(editorWindow_.get(), &EditorWindow::copyRequested,
                &context_.captureViewModel(), &CaptureViewModel::copyCurrentImageToClipboard);
        connect(editorWindow_.get(), &EditorWindow::pinRequested, this, [this](const QImage& image) {
            context_.pinViewModel().createFromImage(image, PinSource::Screenshot);
        });
    }
    return *editorWindow_;
}

} // namespace snappaste
