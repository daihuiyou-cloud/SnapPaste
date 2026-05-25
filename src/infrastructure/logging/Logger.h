#pragma once

#include <QString>

namespace snappaste {

class Logger final {
public:
    static void install();
    static void info(const QString& message);
    static void warning(const QString& message);
};

} // namespace snappaste
