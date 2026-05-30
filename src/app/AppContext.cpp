#include "app/AppContext.h"

#include "infrastructure/config/JsonSettingsRepository.h"
#include "infrastructure/filesystem/AppPaths.h"
#include "platform/windows/capture/QtScreenPixelSampler.h"
#include "platform/windows/capture/WinScreenRegionDetector.h"
#include "platform/windows/hotkey/WinHotkeyService.h"
#include "shared/events/EventHub.h"

namespace snappaste {

AppContext::AppContext()
    : eventHub_(std::make_unique<EventHub>())
    , infra_(InfrastructureFactory::create(std::make_unique<AppPaths>()))
    , domain_(ServiceFactory::create(infra_))
    , viewModels_(ViewModelFactory::create(infra_, domain_, *eventHub_))
{
}

AppContext::~AppContext() = default;

EventHub& AppContext::eventHub() noexcept
{
    return *eventHub_;
}

ISettingsRepository& AppContext::settingsRepository() noexcept
{
    return *infra_.settingsRepository;
}

IHotkeyService& AppContext::hotkeyService() noexcept
{
    return *infra_.hotkeyService;
}

IPlatformService& AppContext::platformService() noexcept
{
    return *infra_.platformService;
}

IIconProvider& AppContext::iconProvider() noexcept
{
    return *infra_.iconProvider;
}

CaptureViewModel& AppContext::captureViewModel() noexcept
{
    return *viewModels_.captureViewModel;
}

HistoryViewModel& AppContext::historyViewModel() noexcept
{
    return *viewModels_.historyViewModel;
}

SettingsViewModel& AppContext::settingsViewModel() noexcept
{
    return *viewModels_.settingsViewModel;
}

PinViewModel& AppContext::pinViewModel() noexcept
{
    return *viewModels_.pinViewModel;
}

IScreenRegionDetector& AppContext::screenRegionDetector() noexcept
{
    return *infra_.screenRegionDetector;
}

IScreenPixelSampler& AppContext::screenPixelSampler() noexcept
{
    return *infra_.screenPixelSampler;
}

CaptureSelectionHistory& AppContext::captureSelectionHistory() noexcept
{
    return *domain_.captureSelectionHistory;
}

} // namespace snappaste
