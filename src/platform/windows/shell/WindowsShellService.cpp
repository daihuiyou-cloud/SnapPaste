#include "platform/windows/shell/WindowsShellService.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

namespace snappaste {

void WindowsShellService::revealInExplorer(const QString& filePath) const
{
    const QFileInfo info(filePath);
    if (info.exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
    }
}

} // namespace snappaste
