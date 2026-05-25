#pragma once

#include "shared/result/Result.h"

#include <QSqlDatabase>

namespace nanosnap {

class SqliteMigrator final {
public:
    Result<void> migrate(QSqlDatabase database);

private:
    Result<int> currentVersion(QSqlDatabase database);
    Result<void> applyVersion1(QSqlDatabase database);
    Result<void> applyVersion2(QSqlDatabase database);
    Result<void> applyVersion3(QSqlDatabase database);
};

} // namespace nanosnap
