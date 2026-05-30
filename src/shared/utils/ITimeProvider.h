#pragma once

#include <QDateTime>

namespace snappaste {

struct ITimeProvider {
    virtual ~ITimeProvider() = default;
    virtual QDateTime nowUtc() = 0;
};

} // namespace snappaste
