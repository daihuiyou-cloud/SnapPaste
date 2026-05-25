#pragma once

#include <QString>

namespace nanosnap {

class WindowsShellService final {
public:
    void revealInExplorer(const QString& filePath) const;
};

} // namespace nanosnap
