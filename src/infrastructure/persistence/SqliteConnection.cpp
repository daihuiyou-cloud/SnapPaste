#include <QCoreApplication>
#include "infrastructure/persistence/SqliteConnection.h"

#include <QSqlQuery>
#include <QUuid>

namespace snappaste {

SqliteConnection::SqliteConnection(QString databasePath)
    : connectionName_("snappaste_" + QUuid::createUuid().toString(QUuid::Id128))
    , databasePath_(std::move(databasePath))
{
}

SqliteConnection::~SqliteConnection()
{
    if (QSqlDatabase::contains(connectionName_)) {
        auto db = QSqlDatabase::database(connectionName_);
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName_);
    }
}

Result<QSqlDatabase> SqliteConnection::database()
{
    QSqlDatabase db;
    if (QSqlDatabase::contains(connectionName_)) {
        db = QSqlDatabase::database(connectionName_);
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName_);
        db.setDatabaseName(databasePath_);
    }

    if (!opened_) {
        if (!db.open()) {
            return Result<QSqlDatabase>::failure(QCoreApplication::translate("AppErrors", "Failed to open SQLite database."));
        }

        QSqlQuery query(db);
        query.exec("PRAGMA journal_mode = WAL");
        query.exec("PRAGMA busy_timeout = 3000");

        auto migrateResult = migrator_.migrate(db);
        if (migrateResult.isError()) {
            return Result<QSqlDatabase>::failure(migrateResult.error());
        }

        opened_ = true;
    }

    return Result<QSqlDatabase>::success(db);
}

} // namespace snappaste
