#pragma once

#include <QDateTime>

namespace nanosnap {

class TimeProvider final {
public:
    static QDateTime nowUtc()
    {
        return QDateTime::currentDateTimeUtc();
    }
};

} // namespace nanosnap
