#include "infrastructure/persistence/SqliteConnection.h"

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
            return Result<QSqlDatabase>::failure("Failed to open SQLite database.");
        }
        opened_ = true;
    }

    return Result<QSqlDatabase>::success(db);
}

} // namespace snappaste
