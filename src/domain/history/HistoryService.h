#pragma once

#include "domain/history/IHistoryRepository.h"

namespace snappaste {

class HistoryService final {
public:
    explicit HistoryService(IHistoryRepository& repository);

    Result<QVector<CaptureRecord>> recentCaptures(int limit);
    Result<void> deleteCapture(qint64 id);

private:
    IHistoryRepository& repository_;
};

} // namespace snappaste
