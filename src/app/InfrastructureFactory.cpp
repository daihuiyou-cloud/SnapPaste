#include "app/InfrastructureFactory.h"

#include "infrastructure/clipboard/ClipboardImageProvider.h"
#include "infrastructure/config/JsonSettingsRepository.h"
#include "infrastructure/image/LocalImageStorage.h"
#include "infrastructure/persistence/SqliteConnection.h"
#include "infrastructure/persistence/SqliteHistoryRepository.h"
#include "infrastructure/persistence/SqlitePinnedItemRepository.h"
#include "platform/windows/WindowsPlatformService.h"
#include "platform/windows/capture/DxgiScreenCaptureService.h"
#include "platform/windows/capture/QtScreenPixelSampler.h"
#include "platform/windows/capture/WinScreenRegionDetector.h"
#include "platform/windows/hotkey/WinHotkeyService.h"

namespace snappaste {

InfrastructureServices InfrastructureFactory::create(const QString& databaseFilePath)
{
    InfrastructureServices svc;

    svc.platformService = std::make_unique<WindowsPlatformService>();
    svc.databaseConnection = std::make_unique<SqliteConnection>(databaseFilePath);
    svc.settingsRepository = std::make_unique<JsonSettingsRepository>();
    svc.historyRepository = std::make_unique<SqliteHistoryRepository>(*svc.databaseConnection);
    svc.pinnedItemRepository = std::make_unique<SqlitePinnedItemRepository>(*svc.databaseConnection);
    svc.clipboardImageProvider = std::make_unique<ClipboardImageProvider>();
    svc.captureService = std::make_unique<DxgiScreenCaptureService>();
    svc.screenRegionDetector = std::make_unique<WinScreenRegionDetector>();
    svc.screenPixelSampler = std::make_unique<QtScreenPixelSampler>();
    svc.imageStorage = std::make_unique<LocalImageStorage>();
    svc.hotkeyService = std::make_unique<WinHotkeyService>();

    return svc;
}

} // namespace snappaste