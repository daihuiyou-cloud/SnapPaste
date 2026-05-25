#pragma once

#include <QWidget>

namespace snappaste {

class WindowInteractionService final {
public:
    void setClickThrough(QWidget* widget, bool enabled) const;
};

} // namespace snappaste
