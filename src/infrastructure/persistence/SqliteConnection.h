#pragma once

#include "infrastructure/persistence/SqliteMigrator.h"
#include "shared/result/Result.h"

#include <QSqlDatabase>
#include <QString>

namespace snappaste {

class SqliteConnection final {
public:
    explicit SqliteConnection(QString databasePath);
    ~SqliteConnection();

    Result<QSqlDatabase> database();

private:
    QString connectionName_;
    QString databasePath_;
    SqliteMigrator migrator_;
    bool opened_ = false;
};

} // namespace snappaste
