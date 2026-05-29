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
            switch (key) {
            case 0x08: parts << "Backspace"; break;
            case 0x09: parts << "Tab"; break;
            case 0x0D: parts << "Enter"; break;
            case 0x1B: parts << "Esc"; break;
            case 0x20: parts << "Space"; break;
            case 0x21: parts << "PageUp"; break;
            case 0x22: parts << "PageDown"; break;
            case 0x23: parts << "End"; break;
            case 0x24: parts << "Home"; break;
            case 0x25: parts << "Left"; break;
            case 0x26: parts << "Up"; break;
            case 0x27: parts << "Right"; break;
            case 0x28: parts << "Down"; break;
            case 0x2D: parts << "Insert"; break;
            case 0x2E: parts << "Delete"; break;
            case 0x5B: case 0x5C: parts << "Win"; break;
            case 0x6A: parts << "*"; break;
            case 0x6B: parts << "+"; break;
            case 0x6D: parts << "-"; break;
            case 0x6E: parts << "."; break;
            case 0x6F: parts << "/"; break;
            case 0x90: parts << "NumLock"; break;
            case 0x91: parts << "ScrollLock"; break;
            case 0xBA: parts << ";"; break;
            case 0xBB: parts << "="; break;
            case 0xBC: parts << ","; break;
            case 0xBD: parts << "-"; break;
            case 0xBE: parts << "."; break;
            case 0xBF: parts << "/"; break;
            case 0xC0: parts << "`"; break;
            case 0xDB: parts << "["; break;
            case 0xDC: parts << "\\"; break;
            case 0xDD: parts << "]"; break;
            case 0xDE: parts << "'"; break;
            default:
                parts << QString("0x%1").arg(key, 0, 16);
                break;
            }
        }
        return parts.join("+");
    }
};

} // namespace snappaste
