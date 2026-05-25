#pragma once

#include "app/AppContext.h"
#include "presentation/capture_overlay/CaptureOverlay.h"
#include "presentation/editor/EditorWindow.h"
#include "presentation/main_window/MainWindow.h"
#include "presentation/pin_window/PinWindow.h"
#include "presentation/toast/ToastNotifier.h"
#include "presentation/tray/TrayController.h"

#include "shared/types/AppSettings.h"

#include <QApplication>
#include <QImage>
#include <functional>
#include <map>
#include <memory>
#include <optional>

namespace nanosnap {

class Application final : public QObject {
    Q_OBJECT

public:
    explicit Application(QApplication& qtApplication);

    int run();

private:
    void connectCoreSignals();
    void showMainWindow();
    void startCapture();
    void pasteFromClipboard();
    void hideAllPins();
    void showAllPins();
    void copyRegion(const QRect& region);
    void pinRegion(const QRect& region);
    void saveRegion(const QRect& region);
    void editRegion(const QRect& region);
    void ocrRegion(const QRect& region);
    void showStatus(const QString& message);
    void captureAfterOverlayHidden(const QRect& region, std::function<void(const QImage&)> onReady);
    void ensureSettingsCached();
    void invalidateSettingsCache();
    void registerHotkey();
    void applyCurrentTheme();
    void openPinWindow(PinnedItem item);
    QPoint cascadedPinPosition(const QPoint& basePosition) const;
    QPoint pinnedPositionFor(const QSize& imageSize,
                             const QPoint& preferredPosition,
                             const std::optional<QRect>& avoidRegion) const;
    CaptureOverlay& overlay();
    EditorWindow& editorWindow();

    QApplication& qtApplication_;
    AppContext context_;
    TrayController trayController_;
    ToastNotifier toastNotifier_;
    std::unique_ptr<MainWindow> mainWindow_;
    std::unique_ptr<CaptureOverlay> overlay_;
    std::unique_ptr<EditorWindow> editorWindow_;
    std::optional<AppSettings> cachedSettings_;
    std::map<qint64, std::unique_ptr<PinWindow>> pinWindows_;
    std::optional<QPoint> pendingPinPosition_;
    std::optional<QRect> pendingPinAvoidRegion_;
    QImage lastPinnableImage_;
    PinSource lastPinnableSource_ = PinSource::Screenshot;
    bool preferLastPinnableImage_ = false;
};

} // namespace nanosnap
