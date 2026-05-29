#include <QCoreApplication>
#include "domain/history/HistoryService.h"

namespace snappaste {

HistoryService::HistoryService(IHistoryRepository& repository)
    : repository_(repository)
{
}

Result<QVector<CaptureRecord>> HistoryService::recentCaptures(int limit)
{
    if (limit <= 0) {
        return Result<QVector<CaptureRecord>>::success({});
    }

    return repository_.recent(limit);
}

Result<void> HistoryService::deleteCapture(qint64 id)
{
    if (id <= 0) {
        return Result<void>::failure(QCoreApplication::translate("AppErrors", "Invalid capture id."));
    }

    return repository_.markDeleted(id);
}

} // namespace snappaste
