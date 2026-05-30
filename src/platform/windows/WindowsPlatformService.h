#pragma once

#include "platform/IPlatformService.h"

namespace snappaste {

class WindowsPlatformService final : public IPlatformService {
public:
    bool isSystemDarkMode() const override;
    void setClickThrough(QWidget* widget, bool enabled) const override;
    void revealInExplorer(const QString& filePath) const override;
    void configureHighDpi() override;
};

} // namespace snappaste