#pragma once

#include <QDateTime>

namespace snappaste {

class TimeProvider final {
public:
    static QDateTime nowUtc()
    {
        return QDateTime::currentDateTimeUtc();
    }
};

} // namespace snappaste
