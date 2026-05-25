#pragma once

#include "shared/result/Result.h"
#include "shared/types/CaptureRecord.h"

#include <QVector>

namespace nanosnap {

class IHistoryRepository {
public:
    virtual ~IHistoryRepository() = default;

    virtual Result<CaptureRecord> add(const CaptureRecord& record) = 0;
    virtual Result<QVector<CaptureRecord>> recent(int limit) = 0;
    virtual Result<void> markDeleted(qint64 id) = 0;
};

} // namespace nanosnap
