#pragma once

#include "domain/capture/CaptureWorkflow.h"
#include "domain/capture/CaptureSelectionHistory.h"
#include "domain/capture/IScreenPixelSampler.h"
#include "domain/capture/IScreenRegionDetector.h"
#include "domain/history/HistoryService.h"
#include "domain/hotkeys/IHotkeyService.h"
#include "domain/pin/PinnedImageService.h"
#include "domain/settings/SettingsService.h"
#include "infrastructure/clipboard/ClipboardImageProvider.h"
#include "infrastructure/config/JsonSettingsRepository.h"
#include "infrastructure/image/LocalImageStorage.h"
#include "infrastructure/persistence/SqliteHistoryRepository.h"
#include "infrastructure/persistence/SqlitePinnedItemRepository.h"
#include "platform/windows/capture/GdiScreenCaptureService.h"
#include "platform/windows/capture/QtScreenPixelSampler.h"
#include "platform/windows/capture/WinScreenRegionDetector.h"
#include "platform/windows/darkmode/DarkModeDetector.h"
#include "platform/windows/hotkey/WinHotkeyService.h"
#include "presentation/viewmodels/CaptureViewModel.h"
#include "presentation/viewmodels/HistoryViewModel.h"
#include "presentation/viewmodels/PinViewModel.h"
#include "presentation/viewmodels/SettingsViewModel.h"
#include "shared/events/EventHub.h"

#include <memory>

namespace nanosnap {

class AppContext final {
public:
    AppContext();

    EventHub& eventHub() noexcept;
    SettingsService& settingsService() noexcept;
    IHotkeyService& hotkeyService() noexcept;
    DarkModeDetector& darkModeDetector() noexcept;

    CaptureViewModel& captureViewModel() noexcept;
    HistoryViewModel& historyViewModel() noexcept;
    SettingsViewModel& settingsViewModel() noexcept;
    PinViewModel& pinViewModel() noexcept;
    IScreenRegionDetector& screenRegionDetector() noexcept;
    IScreenPixelSampler& screenPixelSampler() noexcept;
    CaptureSelectionHistory& captureSelectionHistory() noexcept;

private:
    std::unique_ptr<EventHub> eventHub_;

    std::unique_ptr<JsonSettingsRepository> settingsRepository_;
    std::unique_ptr<SqliteHistoryRepository> historyRepository_;
    std::unique_ptr<SqlitePinnedItemRepository> pinnedItemRepository_;
    std::unique_ptr<ClipboardImageProvider> clipboardImageProvider_;
    std::unique_ptr<GdiScreenCaptureService> captureService_;
    std::unique_ptr<WinScreenRegionDetector> screenRegionDetector_;
    std::unique_ptr<QtScreenPixelSampler> screenPixelSampler_;
    std::unique_ptr<LocalImageStorage> imageStorage_;
    std::unique_ptr<WinHotkeyService> hotkeyService_;
    std::unique_ptr<DarkModeDetector> darkModeDetector_;

    std::unique_ptr<SettingsService> settingsService_;
    std::unique_ptr<HistoryService> historyService_;
    std::unique_ptr<CaptureWorkflow> captureWorkflow_;
    std::unique_ptr<PinnedImageService> pinnedImageService_;
    std::unique_ptr<CaptureSelectionHistory> captureSelectionHistory_;

    std::unique_ptr<CaptureViewModel> captureViewModel_;
    std::unique_ptr<HistoryViewModel> historyViewModel_;
    std::unique_ptr<SettingsViewModel> settingsViewModel_;
    std::unique_ptr<PinViewModel> pinViewModel_;
};

} // namespace nanosnap
