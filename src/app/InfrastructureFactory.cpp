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
#include "infrastructure/filesystem/AppPaths.h"
#include "shared/utils/TimeProvider.h"

namespace snappaste {

InfrastructureServices InfrastructureFactory::create(std::unique_ptr<IAppPaths> appPaths)
{
    InfrastructureServices svc;

    svc.appPaths = std::move(appPaths);
    svc.platformService = std::make_unique<WindowsPlatformService>();
    svc.databaseConnection = std::make_unique<SqliteConnection>(svc.appPaths->databaseFilePath());
    svc.settingsRepository = std::make_unique<JsonSettingsRepository>(*svc.appPaths);
    svc.historyRepository = std::make_unique<SqliteHistoryRepository>(*svc.databaseConnection);
    svc.pinnedItemRepository = std::make_unique<SqlitePinnedItemRepository>(*svc.databaseConnection);
    svc.clipboardImageProvider = std::make_unique<ClipboardImageProvider>();
    svc.captureService = std::make_unique<DxgiScreenCaptureService>();
    svc.screenRegionDetector = std::make_unique<WinScreenRegionDetector>();
    svc.screenPixelSampler = std::make_unique<QtScreenPixelSampler>();
    svc.imageStorage = std::make_unique<LocalImageStorage>(*svc.appPaths);
    svc.hotkeyService = std::make_unique<WinHotkeyService>();
    svc.timeProvider = std::make_unique<TimeProvider>();

    return svc;
}

} // namespace snappaste