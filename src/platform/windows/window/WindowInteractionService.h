#pragma once

#include <QWidget>

namespace nanosnap {

class WindowInteractionService final {
public:
    void setClickThrough(QWidget* widget, bool enabled) const;
};

} // namespace nanosnap
