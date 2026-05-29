#include "platform/windows/hotkey/WinHotkeyService.h"

#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace snappaste {

namespace {
constexpr int kCaptureHotkeyId = 1001;
constexpr int kPasteHotkeyId = 1002;
constexpr int kHidePinsHotkeyId = 1003;
constexpr int kRepeatCaptureHotkeyId = 1004;
}

WinHotkeyService::WinHotkeyService()
{
    QCoreApplication::instance()->installNativeEventFilter(this);
}

WinHotkeyService::~WinHotkeyService()
{
    unregisterAll();
    if (QCoreApplication::instance() != nullptr) {
        QCoreApplication::instance()->removeNativeEventFilter(this);
    }
}

bool WinHotkeyService::registerHotkey(HotkeyAction action, const Hotkey& hotkey)
{
#ifdef Q_OS_WIN
    const auto id = idFor(action);
    const auto registered = RegisterHotKey(nullptr, id, modifiersFor(hotkey), static_cast<UINT>(hotkey.key)) != FALSE;
    if (registered) {
        unregisterHotkey(action);
        registeredActions_.insert(action);
    }
    return registered;
#else
    Q_UNUSED(action)
    Q_UNUSED(hotkey)
    return false;
#endif
}

void WinHotkeyService::unregisterHotkey(HotkeyAction action)
{
#ifdef Q_OS_WIN
    if (registeredActions_.find(action) != registeredActions_.end()) {
        UnregisterHotKey(nullptr, idFor(action));
        registeredActions_.erase(action);
    }
#else
    Q_UNUSED(action)
#endif
}

void WinHotkeyService::unregisterAll()
{
    const auto actions = registeredActions_;
    for (const auto action : actions) {
        unregisterHotkey(action);
    }
}

void WinHotkeyService::setActionCallback(HotkeyAction action, Callback callback)
{
    callbacks_[action] = std::move(callback);
}

bool WinHotkeyService::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)

#ifdef Q_OS_WIN
    auto* msg = static_cast<MSG*>(message);
    if (msg != nullptr && msg->message == WM_HOTKEY) {
        const auto action = actionForId(static_cast<int>(msg->wParam));
        const auto callback = callbacks_.find(action);
        if (callback != callbacks_.end() && callback->second) {
            callback->second();
            return true;
        }
        return false;
    }
#else
    Q_UNUSED(message)
#endif

    return false;
}

unsigned int WinHotkeyService::modifiersFor(const Hotkey& hotkey)
{
#ifdef Q_OS_WIN
    unsigned int modifiers = MOD_NOREPEAT;
    if (hotkey.ctrl) {
        modifiers |= MOD_CONTROL;
    }
    if (hotkey.alt) {
        modifiers |= MOD_ALT;
    }
    if (hotkey.shift) {
        modifiers |= MOD_SHIFT;
    }
    return modifiers;
#else
    Q_UNUSED(hotkey)
    return 0;
#endif
}

int WinHotkeyService::idFor(HotkeyAction action)
{
    switch (action) {
    case HotkeyAction::Paste:
        return kPasteHotkeyId;
    case HotkeyAction::HideAllPins:
        return kHidePinsHotkeyId;
    case HotkeyAction::RepeatCapture:
        return kRepeatCaptureHotkeyId;
    case HotkeyAction::Capture:
    default:
        return kCaptureHotkeyId;
    }
}

HotkeyAction WinHotkeyService::actionForId(int id)
{
    switch (id) {
    case kPasteHotkeyId:
        return HotkeyAction::Paste;
    case kHidePinsHotkeyId:
        return HotkeyAction::HideAllPins;
    case kRepeatCaptureHotkeyId:
        return HotkeyAction::RepeatCapture;
    case kCaptureHotkeyId:
        return HotkeyAction::Capture;
    default:
        Q_ASSERT_X(false, "actionForId", "unknown hotkey id");
        return HotkeyAction::Capture;
    }
}

} // namespace snappaste
