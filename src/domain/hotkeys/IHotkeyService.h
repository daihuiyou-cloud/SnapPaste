#pragma once

#include "shared/types/Hotkey.h"

#include <functional>

namespace nanosnap {

enum class HotkeyAction {
    Capture,
    Paste,
    HideAllPins
};

class IHotkeyService {
public:
    using Callback = std::function<void()>;

    virtual ~IHotkeyService() = default;

    virtual bool registerHotkey(HotkeyAction action, const Hotkey& hotkey) = 0;
    virtual void unregisterHotkey(HotkeyAction action) = 0;
    virtual void unregisterAll() = 0;
    virtual void setActionCallback(HotkeyAction action, Callback callback) = 0;
};

} // namespace nanosnap
