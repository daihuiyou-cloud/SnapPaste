#pragma once

#include <QString>

namespace snappaste {

struct ILogger {
    virtual ~ILogger() = default;
    virtual void info(const QString& message) = 0;
    virtual void warning(const QString& message) = 0;
    virtual void install() = 0;
};

} // namespace snappaste
