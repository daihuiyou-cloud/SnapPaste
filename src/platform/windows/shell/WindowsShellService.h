#pragma once

#include <QString>

namespace snappaste {

class WindowsShellService final {
public:
    void revealInExplorer(const QString& filePath) const;
};

} // namespace snappaste
