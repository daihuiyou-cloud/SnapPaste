#include "app/ServiceFactory.h"
#include "app/InfrastructureFactory.h"

#include "domain/capture/CaptureWorkflow.h"
#include "domain/capture/CaptureSelectionHistory.h"
#include "domain/pin/PinnedImageService.h"

namespace snappaste {

DomainServices ServiceFactory::create(InfrastructureServices& infra)
{
    DomainServices svc;

    svc.captureWorkflow = std::make_unique<CaptureWorkflow>(
        *infra.captureService, *infra.imageStorage, *infra.historyRepository, *infra.settingsRepository);
    svc.pinnedImageService = std::make_unique<PinnedImageService>(
        *infra.clipboardImageProvider, *infra.pinnedItemRepository);
    svc.captureSelectionHistory = std::make_unique<CaptureSelectionHistory>();

    return svc;
}

} // namespace snappaste