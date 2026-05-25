#pragma once

#include "domain/pin/IPinnedItemRepository.h"
#include "infrastructure/persistence/SqliteConnection.h"
#include "infrastructure/persistence/SqliteMigrator.h"

namespace nanosnap {

class SqlitePinnedItemRepository final : public IPinnedItemRepository {
public:
    explicit SqlitePinnedItemRepository(QString databasePath);

    Result<PinnedItem> add(const PinnedItem& item) override;
    Result<QVector<PinnedItem>> restoreActive() override;
    Result<void> updateState(qint64 id, const PinnedImageState& state) override;
    Result<void> setAllVisible(bool visible) override;
    Result<void> close(qint64 id) override;

private:
    Result<QSqlDatabase> readyDatabase();
    static QByteArray encodeImage(const QImage& image);
    static QImage decodeImage(const QByteArray& bytes);
    static PinnedItem readItem(const QSqlQuery& query);

    SqliteConnection connection_;
    SqliteMigrator migrator_;
    bool migrated_ = false;
};

} // namespace nanosnap
