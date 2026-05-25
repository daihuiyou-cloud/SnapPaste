#pragma once

#include "domain/hotkeys/IHotkeyService.h"

#include <QAbstractNativeEventFilter>
#include <map>
#include <set>

namespace nanosnap {

class WinHotkeyService final : public QAbstractNativeEventFilter, public IHotkeyService {
public:
    WinHotkeyService();
    ~WinHotkeyService() override;

    bool registerHotkey(HotkeyAction action, const Hotkey& hotkey) override;
    void unregisterHotkey(HotkeyAction action) override;
    void unregisterAll() override;
    void setActionCallback(HotkeyAction action, Callback callback) override;

    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;

private:
    static unsigned int modifiersFor(const Hotkey& hotkey);
    static int idFor(HotkeyAction action);
    static HotkeyAction actionForId(int id);

    std::map<HotkeyAction, Callback> callbacks_;
    std::set<HotkeyAction> registeredActions_;
};

} // namespace nanosnap
