#pragma once

#include "infrastructure/filesystem/IAppPaths.h"
#include "platform/IPlatformService.h"
#include "presentation/icons/IIconProvider.h"

#include <memory>
#include <QString>

namespace snappaste {

class SqliteConnection;
class TimeProvider;
class JsonSettingsRepository;
class SqliteHistoryRepository;
class SqlitePinnedItemRepository;
class ClipboardImageProvider;
class DxgiScreenCaptureService;
class WinScreenRegionDetector;
class QtScreenPixelSampler;
class LocalImageStorage;
class WinHotkeyService;

struct InfrastructureServices {
    InfrastructureServices() = default;
    ~InfrastructureServices();
    InfrastructureServices(InfrastructureServices&&) = default;
    InfrastructureServices& operator=(InfrastructureServices&&) = default;
    std::unique_ptr<SqliteConnection> databaseConnection;
    std::unique_ptr<JsonSettingsRepository> settingsRepository;
    std::unique_ptr<SqliteHistoryRepository> historyRepository;
    std::unique_ptr<SqlitePinnedItemRepository> pinnedItemRepository;
    std::unique_ptr<ClipboardImageProvider> clipboardImageProvider;
    std::unique_ptr<DxgiScreenCaptureService> captureService;
    std::unique_ptr<WinScreenRegionDetector> screenRegionDetector;
    std::unique_ptr<QtScreenPixelSampler> screenPixelSampler;
    std::unique_ptr<LocalImageStorage> imageStorage;
    std::unique_ptr<WinHotkeyService> hotkeyService;
    std::unique_ptr<IPlatformService> platformService;
    std::unique_ptr<TimeProvider> timeProvider;
    std::unique_ptr<IAppPaths> appPaths;
    std::unique_ptr<IIconProvider> iconProvider;
};

class InfrastructureFactory final {
public:
    static InfrastructureServices create(std::unique_ptr<IAppPaths> appPaths);
};

} // namespace snappaste