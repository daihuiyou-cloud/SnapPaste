#pragma once

#include <QString>

namespace nanosnap {

class Logger final {
public:
    static void install();
    static void info(const QString& message);
    static void warning(const QString& message);
};

} // namespace nanosnap
