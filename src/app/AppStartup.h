#pragma once

#include "platform/IPlatformService.h"
#include "shared/result/Result.h"
#include "shared/types/AppSettings.h"

#include <QApplication>

namespace snappaste {

class AppStartup final {
public:
    static Result<void> applyTheme(QApplication& application,
                                   const AppSettings& settings,
                                   const IPlatformService& platformService);

private:
    static QString themePath(const AppSettings& settings, const IPlatformService& platformService);
};

} // namespace snappaste
