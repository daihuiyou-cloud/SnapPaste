#pragma once

#include "app/InfrastructureFactory.h"
#include "app/ServiceFactory.h"
#include "app/ViewModelFactory.h"

#include "domain/capture/CaptureSelectionHistory.h"
#include "domain/capture/IScreenPixelSampler.h"
#include "domain/capture/IScreenRegionDetector.h"
#include "domain/hotkeys/IHotkeyService.h"
#include "domain/settings/ISettingsRepository.h"
#include "platform/IPlatformService.h"

#include <memory>

namespace snappaste {

class AppContext final {
public:
    AppContext();

    EventHub& eventHub() noexcept;
    ISettingsRepository& settingsRepository() noexcept;
    IHotkeyService& hotkeyService() noexcept;
    IPlatformService& platformService() noexcept;

    CaptureViewModel& captureViewModel() noexcept;
    HistoryViewModel& historyViewModel() noexcept;
    SettingsViewModel& settingsViewModel() noexcept;
    PinViewModel& pinViewModel() noexcept;
    IScreenRegionDetector& screenRegionDetector() noexcept;
    IScreenPixelSampler& screenPixelSampler() noexcept;
    CaptureSelectionHistory& captureSelectionHistory() noexcept;

private:
    std::unique_ptr<EventHub> eventHub_;
    InfrastructureServices infra_;
    DomainServices domain_;
    ViewModelServices viewModels_;
};

} // namespace snappaste
