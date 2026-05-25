#pragma once

#include "domain/pin/PinnedItem.h"
#include "shared/result/Result.h"

#include <QVector>

namespace snappaste {

class IPinnedItemRepository {
public:
    virtual ~IPinnedItemRepository() = default;

    virtual Result<PinnedItem> add(const PinnedItem& item) = 0;
    virtual Result<QVector<PinnedItem>> restoreActive() = 0;
    virtual Result<void> updateState(qint64 id, const PinnedImageState& state) = 0;
    virtual Result<void> setAllVisible(bool visible) = 0;
    virtual Result<void> close(qint64 id) = 0;
};

} // namespace snappaste
