#pragma once

#include <QString>
#include <QStringList>

namespace snappaste {

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
        } else if (key >= 0x41 && key <= 0x5A) {
            parts << QChar(static_cast<char>(key));
        } else if (key >= 0x30 && key <= 0x39) {
            parts << QChar(static_cast<char>(key));
        } else {
            static const struct { int vk; const char* name; } kNamed[] = {
                {0x08, "Backspace"}, {0x09, "Tab"}, {0x0D, "Enter"},
                {0x1B, "Esc"}, {0x20, "Space"},
                {0x21, "PageUp"}, {0x22, "PageDown"},
                {0x23, "End"}, {0x24, "Home"},
                {0x25, "Left"}, {0x26, "Up"}, {0x27, "Right"}, {0x28, "Down"},
                {0x2D, "Insert"}, {0x2E, "Delete"},
                {0x5B, "Win"}, {0x5C, "Win"},
                {0x6A, "*"}, {0x6B, "+"}, {0x6D, "-"}, {0x6E, "."}, {0x6F, "/"},
                {0x90, "NumLock"}, {0x91, "ScrollLock"},
                {0xBA, ";"}, {0xBB, "="}, {0xBC, ","}, {0xBD, "-"},
                {0xBE, "."}, {0xBF, "/"}, {0xC0, "`"},
                {0xDB, "["}, {0xDC, "\\"}, {0xDD, "]"}, {0xDE, "'"},
            };
            bool found = false;
            for (const auto& entry : kNamed) {
                if (entry.vk == key) {
                    parts << entry.name;
                    found = true;
                    break;
                }
            }
            if (!found) {
                parts << QString("0x%1").arg(key, 0, 16);
            }
        }
        return parts.join("+");
    }
};

} // namespace snappaste
