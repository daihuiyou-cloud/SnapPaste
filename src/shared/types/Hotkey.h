#pragma once

#include <QString>
#include <QStringList>

namespace nanosnap {

struct Hotkey final {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    int key = 0x70;

    QString toDisplayString() const
    {
        QStringList parts;
        if (ctrl) {
            parts << "Ctrl";
        }
        if (alt) {
            parts << "Alt";
        }
        if (shift) {
            parts << "Shift";
        }
        if (key >= 0x70 && key <= 0x7B) {
            parts << QString("F%1").arg(key - 0x6F);
        } else {
            parts << QChar(static_cast<char>(key)).toUpper();
        }
        return parts.join("+");
    }
};

} // namespace nanosnap
