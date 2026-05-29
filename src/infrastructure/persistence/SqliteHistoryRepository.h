#pragma once

#include "domain/history/IHistoryRepository.h"
#include "infrastructure/persistence/SqliteConnection.h"

namespace snappaste {

class SqliteHistoryRepository final : public IHistoryRepository {
public:
    explicit SqliteHistoryRepository(SqliteConnection& connection);

    Result<CaptureRecord> add(const CaptureRecord& record) override;
    Result<QVector<CaptureRecord>> recent(int limit) override;
    Result<void> markDeleted(qint64 id) override;

private:
    enum CaptureColumn {
        ColId = 0,
        ColFilePath,
        ColThumbnailPath,
        ColWidth,
        ColHeight,
        ColDevicePixelRatio,
        ColFormat,
        ColCreatedAt,
        ColSourceScreen,
        ColDeleted,
    };

    Result<QSqlDatabase> readyDatabase();
    static CaptureRecord readRecord(const QSqlQuery& query);

    SqliteConnection& connection_;
};

} // namespace snappaste
