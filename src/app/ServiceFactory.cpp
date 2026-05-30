#include "app/ServiceFactory.h"
#include "app/InfrastructureFactory.h"

#include "domain/capture/CaptureWorkflow.h"
#include "domain/capture/CaptureSelectionHistory.h"
#include "domain/pin/PinnedImageService.h"
#include "infrastructure/clipboard/ClipboardImageProvider.h"
#include "infrastructure/config/JsonSettingsRepository.h"
#include "infrastructure/image/LocalImageStorage.h"
#include "infrastructure/persistence/SqliteHistoryRepository.h"
#include "infrastructure/persistence/SqlitePinnedItemRepository.h"
#include "platform/windows/capture/DxgiScreenCaptureService.h"
#include "shared/utils/TimeProvider.h"

namespace snappaste {

DomainServices::~DomainServices() = default;

DomainServices ServiceFactory::create(InfrastructureServices& infra)
{
    DomainServices svc;

    svc.captureWorkflow = std::make_unique<CaptureWorkflow>(
        *infra.captureService, *infra.imageStorage, *infra.historyRepository, *infra.settingsRepository,
        *infra.timeProvider);
    svc.pinnedImageService = std::make_unique<PinnedImageService>(
        *infra.clipboardImageProvider, *infra.pinnedItemRepository, *infra.timeProvider);
    svc.captureSelectionHistory = std::make_unique<CaptureSelectionHistory>();

    return svc;
}

} // namespace snappaste