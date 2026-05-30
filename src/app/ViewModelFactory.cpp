#include "app/ViewModelFactory.h"
#include "app/InfrastructureFactory.h"
#include "app/ServiceFactory.h"

#include "presentation/viewmodels/CaptureViewModel.h"
#include "presentation/viewmodels/HistoryViewModel.h"
#include "presentation/viewmodels/PinViewModel.h"
#include "presentation/viewmodels/SettingsViewModel.h"
#include "shared/events/EventHub.h"

namespace snappaste {

ViewModelServices ViewModelFactory::create(InfrastructureServices& infra,
                                           DomainServices& domain,
                                           EventHub& eventHub)
{
    ViewModelServices svc;

    svc.captureViewModel = std::make_unique<CaptureViewModel>(*domain.captureWorkflow, eventHub);
    svc.historyViewModel = std::make_unique<HistoryViewModel>(*infra.historyRepository);
    svc.settingsViewModel = std::make_unique<SettingsViewModel>(*infra.settingsRepository, eventHub);
    svc.pinViewModel = std::make_unique<PinViewModel>(*domain.pinnedImageService);

    return svc;
}

} // namespace snappaste