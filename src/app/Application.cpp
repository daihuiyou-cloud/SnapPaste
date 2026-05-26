#include "app/Application.h"

#include "app/AppStartup.h"
#include "infrastructure/logging/Logger.h"

#include <QPointer>

#include <QMessageBox>
#include <QClipboard>
#include <QCursor>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMetaObject>
#include <QScreen>
#include <QSignalBlocker>
#include <QStringList>
#include <QTimer>

#include <algorithm>
#include <cstring>

#if defined(_WIN32) && __has_include(<winrt/Windows.Media.Ocr.h>)
#define SNAPPASTE_HAS_WINRT_OCR 1
#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/base.h>
#if defined(_MSC_VER)
#pragma comment(lib, "windowsapp")
#endif
#endif

namespace snappaste {

namespace {

constexpr int kCaptureAfterHideDelayMs = 120;
constexpr int kPinBaseOffset = 16;
constexpr int kPinCascadeOffset = 24;
constexpr int kPinCascadeSlots = 8;

struct OcrOutcome {
    bool ok = false;
    QString text;
    QString message;
};

#if defined(SNAPPASTE_HAS_WINRT_OCR)
struct __declspec(uuid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")) IMemoryBufferByteAccess : ::IUnknown {
    virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
};
#endif

OcrOutcome recognizeTextFromImage(const QImage& source)
{
#if defined(SNAPPASTE_HAS_WINRT_OCR)
    if (source.isNull()) {
        return {false, {}, "No image is available for OCR."};
    }

    try {
        const auto engine = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromUserProfileLanguages();
        if (engine == nullptr) {
            return {false, {}, "OCR is not available for the current Windows language profile."};
        }

        const auto image = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        winrt::Windows::Graphics::Imaging::SoftwareBitmap bitmap(
            winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,
            image.width(),
            image.height(),
            winrt::Windows::Graphics::Imaging::BitmapAlphaMode::Premultiplied);

        {
            const auto buffer = bitmap.LockBuffer(winrt::Windows::Graphics::Imaging::BitmapBufferAccessMode::Write);
            const auto reference = buffer.CreateReference();
            auto byteAccess = reference.as<IMemoryBufferByteAccess>();
            uint8_t* bytes = nullptr;
            uint32_t capacity = 0;
            winrt::check_hresult(byteAccess->GetBuffer(&bytes, &capacity));

            const auto plane = buffer.GetPlaneDescription(0);
            const auto rowBytes = image.width() * 4;
            for (int y = 0; y < image.height(); ++y) {
                const auto targetOffset = plane.StartIndex + (y * plane.Stride);
                if (targetOffset + rowBytes > static_cast<int>(capacity)) {
                    break;
                }
                std::memcpy(bytes + targetOffset, image.constScanLine(y), std::min(rowBytes, image.bytesPerLine()));
            }
        }

        const auto result = engine.RecognizeAsync(bitmap).get();
        QStringList lines;
        for (const auto& line : result.Lines()) {
            lines << QString::fromWCharArray(line.Text().c_str());
        }
        const auto text = lines.join('\n').trimmed();
        if (text.isEmpty()) {
            return {false, {}, "No text was recognized in the selected region."};
        }
        return {true, text, {}};
    } catch (...) {
        return {false, {}, "OCR failed while processing the selected region."};
    }
#else
    Q_UNUSED(source)
    return {false, {}, "OCR is not available in this build."};
#endif
}

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
    connectCoreSignals();

#if defined(SNAPPASTE_HAS_WINRT_OCR)
    try {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
    } catch (...) {
        Logger::warning("Failed to initialize WinRT apartment for OCR.");
    }
#endif

    trayController_.show();
    registerHotkey();
    context_.pinViewModel().restore();

    const auto exitCode = qtApplication_.exec();

#if defined(SNAPPASTE_HAS_WINRT_OCR)
    winrt::uninit_apartment();
#endif

    return exitCode;
}

void Application::connectCoreSignals()
{
    connect(&trayController_, &TrayController::captureRequested, this, &Application::startCapture);
    connect(&trayController_, &TrayController::openFileRequested, this, &Application::openFile);
    connect(&trayController_, &TrayController::showWindowRequested, this, &Application::showMainWindow);
    connect(&trayController_, &TrayController::hidePinsRequested, this, &Application::hideAllPins);
    connect(&trayController_, &TrayController::showPinsRequested, this, &Application::showAllPins);
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

    connect(&context_.captureViewModel(), &CaptureViewModel::errorOccurred, this, [this](const QString& message) {
        QMessageBox::warning(mainWindow_.get(), "SnapPaste", message);
    });
    connect(&context_.captureViewModel(), &CaptureViewModel::copied, this, [this] {
        showStatus("Screenshot copied. Press F3 to pin.");
    });
    connect(&context_.captureViewModel(), &CaptureViewModel::saved, this, [this](const QString& filePath) {
        showStatus("Saved " + QFileInfo(filePath).fileName() + ". Press F3 to pin.");
    });
    connect(&context_.pinViewModel(), &PinViewModel::pinCreated, this, [this](const PinnedItem& item) {
        if (!item.image.isNull()) {
            lastPinnableImage_ = item.image;
            lastPinnableSource_ = item.source;
            preferLastPinnableImage_ = true;
        }
        openPinWindow(item);
        showStatus("Pinned image created. Press F3 to repeat.");
    });
    connect(&context_.pinViewModel(), &PinViewModel::pinRestored, this, [this](const PinnedItem& item) {
        openPinWindow(item);
    });
    connect(&context_.pinViewModel(), &PinViewModel::errorOccurred, this, [this](const QString& message) {
        pendingPinPosition_.reset();
        pendingPinAvoidRegion_.reset();
        QMessageBox::warning(mainWindow_.get(), "SnapPaste", message);
    });
    connect(&context_.eventHub(), &EventHub::historyChanged, &context_.historyViewModel(), &HistoryViewModel::refresh);
    connect(&context_.eventHub(), &EventHub::settingsChanged, this, [this] {
        invalidateSettingsCache();
        applyCurrentTheme();
        registerHotkey();
    });
}

void Application::showMainWindow()
{
    if (!mainWindow_) {
        mainWindow_ = std::make_unique<MainWindow>(context_.historyViewModel(), context_.settingsViewModel());
        connect(mainWindow_.get(), &MainWindow::captureRequested, this, &Application::startCapture);
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
        mainWindow_.get(), "Open Image", QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;All Files (*)");
    if (path.isEmpty()) {
        return;
    }

    QImage image(path);
    if (image.isNull()) {
        showStatus("Failed to open image: " + QFileInfo(path).fileName());
        return;
    }

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
    showStatus("Pinned images hidden.");
}

void Application::showAllPins()
{
    context_.pinViewModel().setAllVisible(true);
    for (auto& entry : pinWindows_) {
        if (entry.second) {
            entry.second->restoreInteraction();
        }
    }
    showStatus("Pinned images restored.");
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
            showStatus("Failed to capture image for pinning.");
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
            showStatus("Failed to capture image for editing.");
            return;
        }
        editorWindow().setImage(image);
    });
}

void Application::ocrRegion(const QRect& region)
{
    captureAfterOverlayHidden(region, [this](const QImage& image) {
        const auto outcome = recognizeTextFromImage(image);
        if (!outcome.ok) {
            showStatus(outcome.message);
            return;
        }

        QApplication::clipboard()->setText(outcome.text);
        showStatus("Recognized text copied to the clipboard.");
    });
}

void Application::showStatus(const QString& message)
{
    toastNotifier_.showMessage(message);
}

void Application::captureAfterOverlayHidden(const QRect& region, std::function<void(const QImage&)> onReady)
{
    overlay().hide();
    QPointer<Application> guard(this);
    QTimer::singleShot(kCaptureAfterHideDelayMs, this, [guard, region, onReady = std::move(onReady)]() mutable {
        if (guard.isNull()) return;
        guard->context_.captureViewModel().captureRegionAsync(region, [guard, onReady = std::move(onReady)](const QImage& image) mutable {
            if (guard.isNull()) return;
            if (!image.isNull()) {
                guard->lastPinnableImage_ = image;
                guard->lastPinnableSource_ = PinSource::Screenshot;
                guard->preferLastPinnableImage_ = true;
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
        const auto message = "Failed to register capture hotkey: " + settings.captureHotkey.toDisplayString();
        Logger::warning(message);
        trayController_.showMessage("SnapPaste", message);
    }
    if (!context_.hotkeyService().registerHotkey(HotkeyAction::Paste, settings.pasteHotkey)) {
        const auto message = "Failed to register paste hotkey: " + settings.pasteHotkey.toDisplayString();
        Logger::warning(message);
        trayController_.showMessage("SnapPaste", message);
    }
    if (!context_.hotkeyService().registerHotkey(HotkeyAction::HideAllPins, settings.hidePinsHotkey)) {
        const auto message = "Failed to register hide-pins hotkey: " + settings.hidePinsHotkey.toDisplayString();
        Logger::warning(message);
        trayController_.showMessage("SnapPaste", message);
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
            showStatus("No pinned image is available to copy.");
            return;
        }
        lastPinnableImage_ = image;
        lastPinnableSource_ = source;
        preferLastPinnableImage_ = true;
        const QSignalBlocker blocker(QApplication::clipboard());
        QApplication::clipboard()->setImage(image);
        showStatus("Pinned image copied. Press F3 to repeat.");
    });
    connect(window, &PinWindow::saveRequested, this, [this](const QImage& image) {
        context_.captureViewModel().saveImage(image, "pin");
    });
    if (window->state().options.visible) {
        window->show();
        window->raise();
    }
}

QPoint Application::cascadedPinPosition(const QPoint& basePosition) const
{
    const auto slot = static_cast<int>(pinWindows_.size() % kPinCascadeSlots);
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
    }
    return *editorWindow_;
}

} // namespace snappaste
