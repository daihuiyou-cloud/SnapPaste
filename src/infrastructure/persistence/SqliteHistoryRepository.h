#pragma once

#include "domain/history/IHistoryRepository.h"
#include "infrastructure/persistence/SqliteConnection.h"
#include "infrastructure/persistence/SqliteMigrator.h"

namespace nanosnap {

class SqliteHistoryRepository final : public IHistoryRepository {
public:
    explicit SqliteHistoryRepository(QString databasePath);

    Result<CaptureRecord> add(const CaptureRecord& record) override;
    Result<QVector<CaptureRecord>> recent(int limit) override;
    Result<void> markDeleted(qint64 id) override;

private:
    Result<QSqlDatabase> readyDatabase();
    static CaptureRecord readRecord(const QSqlQuery& query);

    SqliteConnection connection_;
    SqliteMigrator migrator_;
    bool migrated_ = false;
};

} // namespace nanosnap
