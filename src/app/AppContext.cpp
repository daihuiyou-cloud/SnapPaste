#include "app/AppContext.h"

#include "infrastructure/filesystem/AppPaths.h"

namespace snappaste {

AppContext::AppContext()
    : eventHub_(std::make_unique<EventHub>())
    , databaseConnection_(std::make_unique<SqliteConnection>(AppPaths::databaseFilePath()))
    , settingsRepository_(std::make_unique<JsonSettingsRepository>())
    , historyRepository_(std::make_unique<SqliteHistoryRepository>(*databaseConnection_))
    , pinnedItemRepository_(std::make_unique<SqlitePinnedItemRepository>(*databaseConnection_))
    , clipboardImageProvider_(std::make_unique<ClipboardImageProvider>())
    , captureService_(std::make_unique<DxgiScreenCaptureService>())
    , screenRegionDetector_(std::make_unique<WinScreenRegionDetector>())
    , screenPixelSampler_(std::make_unique<QtScreenPixelSampler>())
    , imageStorage_(std::make_unique<LocalImageStorage>())
    , hotkeyService_(std::make_unique<WinHotkeyService>())
    , darkModeDetector_(std::make_unique<DarkModeDetector>())
    , settingsService_(std::make_unique<SettingsService>(*settingsRepository_))
    , historyService_(std::make_unique<HistoryService>(*historyRepository_))
    , captureWorkflow_(std::make_unique<CaptureWorkflow>(
          *captureService_, *imageStorage_, *historyRepository_, *settingsRepository_))
    , pinnedImageService_(std::make_unique<PinnedImageService>(*clipboardImageProvider_, *pinnedItemRepository_))
    , captureSelectionHistory_(std::make_unique<CaptureSelectionHistory>())
    , captureViewModel_(std::make_unique<CaptureViewModel>(*captureWorkflow_, *eventHub_))
    , historyViewModel_(std::make_unique<HistoryViewModel>(*historyService_))
    , settingsViewModel_(std::make_unique<SettingsViewModel>(*settingsService_, *eventHub_))
    , pinViewModel_(std::make_unique<PinViewModel>(*pinnedImageService_))
{
}

EventHub& AppContext::eventHub() noexcept
{
    return *eventHub_;
}

SettingsService& AppContext::settingsService() noexcept
{
    return *settingsService_;
}

IHotkeyService& AppContext::hotkeyService() noexcept
{
    return *hotkeyService_;
}

DarkModeDetector& AppContext::darkModeDetector() noexcept
{
    return *darkModeDetector_;
}

CaptureViewModel& AppContext::captureViewModel() noexcept
{
    return *captureViewModel_;
}

HistoryViewModel& AppContext::historyViewModel() noexcept
{
    return *historyViewModel_;
}

SettingsViewModel& AppContext::settingsViewModel() noexcept
{
    return *settingsViewModel_;
}

PinViewModel& AppContext::pinViewModel() noexcept
{
    return *pinViewModel_;
}

IScreenRegionDetector& AppContext::screenRegionDetector() noexcept
{
    return *screenRegionDetector_;
}

IScreenPixelSampler& AppContext::screenPixelSampler() noexcept
{
    return *screenPixelSampler_;
}

CaptureSelectionHistory& AppContext::captureSelectionHistory() noexcept
{
    return *captureSelectionHistory_;
}

} // namespace snappaste
