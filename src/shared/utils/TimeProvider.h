#pragma once

#include "shared/utils/ITimeProvider.h"

namespace snappaste {

class TimeProvider final : public ITimeProvider {
public:
    QDateTime nowUtc() override
    {
        return QDateTime::currentDateTimeUtc();
    }
};

} // namespace snappaste
