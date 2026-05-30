#pragma once

#include <QString>

class QWidget;

namespace snappaste {

class IPlatformService {
public:
    virtual ~IPlatformService() = default;

    virtual bool isSystemDarkMode() const = 0;
    virtual void setClickThrough(QWidget* widget, bool enabled) const = 0;
    virtual void revealInExplorer(const QString& filePath) const = 0;
    virtual void configureHighDpi() = 0;
};

} // namespace snappaste