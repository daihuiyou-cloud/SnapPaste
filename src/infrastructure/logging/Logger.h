#pragma once

#include "infrastructure/logging/ILogger.h"

namespace snappaste {

class Logger final : public ILogger {
public:
    void install() override;
    void info(const QString& message) override;
    void warning(const QString& message) override;
};

} // namespace snappaste
