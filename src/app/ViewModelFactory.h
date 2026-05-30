#pragma once

#include <memory>

namespace snappaste {

struct InfrastructureServices;
struct DomainServices;
class EventHub;
class CaptureViewModel;
class HistoryViewModel;
class SettingsViewModel;
class PinViewModel;

struct ViewModelServices {
    ViewModelServices() = default;
    ~ViewModelServices();
    ViewModelServices(ViewModelServices&&) = default;
    ViewModelServices& operator=(ViewModelServices&&) = default;
    std::unique_ptr<CaptureViewModel> captureViewModel;
    std::unique_ptr<HistoryViewModel> historyViewModel;
    std::unique_ptr<SettingsViewModel> settingsViewModel;
    std::unique_ptr<PinViewModel> pinViewModel;
};

class ViewModelFactory final {
public:
    static ViewModelServices create(InfrastructureServices& infra,
                                    DomainServices& domain,
                                    EventHub& eventHub);
};

} // namespace snappaste