#pragma once

#include "shared/result/Result.h"

#include <QSqlDatabase>

namespace snappaste {

class SqliteMigrator final {
public:
    Result<void> migrate(QSqlDatabase database);

private:
    Result<int> currentVersion(QSqlDatabase database);
    Result<void> applyVersion1(QSqlDatabase database);
    Result<void> applyVersion2(QSqlDatabase database);
    Result<void> applyVersion3(QSqlDatabase database);
    Result<void> applyVersion4(QSqlDatabase database);
    Result<void> applyVersion5(QSqlDatabase database);
};

} // namespace snappaste
