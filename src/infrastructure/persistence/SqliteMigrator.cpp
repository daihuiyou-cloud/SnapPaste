#include "infrastructure/persistence/SqliteMigrator.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace snappaste {

Result<void> SqliteMigrator::migrate(QSqlDatabase& database)
{
    if (!database.transaction()) {
        return Result<void>::failure(database.lastError().text());
    }

    auto versionResult = currentVersion(database);
    if (versionResult.isError()) {
        database.rollback();
        return Result<void>::failure(versionResult.error());
    }

    auto version = versionResult.value();
    using MigrateFn = Result<void> (SqliteMigrator::*)(QSqlDatabase&);
    const MigrateFn migrations[] = {
        &SqliteMigrator::applyVersion1,
        &SqliteMigrator::applyVersion2,
        &SqliteMigrator::applyVersion3,
        &SqliteMigrator::applyVersion4,
        &SqliteMigrator::applyVersion5,
    };
    const int maxVersion = static_cast<int>(sizeof(migrations) / sizeof(migrations[0]));
    while (version < maxVersion) {
        const auto& fn = migrations[version];
        const auto result = (this->*fn)(database);
        if (result.isError()) {
            database.rollback();
            return result;
        }
        ++version;
    }

    if (!database.commit()) {
        return Result<void>::failure(database.lastError().text());
    }

    return Result<void>::success();
}

Result<int> SqliteMigrator::currentVersion(QSqlDatabase& database)
{
    QSqlQuery query(database);
    if (!query.exec("CREATE TABLE IF NOT EXISTS schema_version (version INTEGER NOT NULL, applied_at TEXT NOT NULL)")) {
        return Result<int>::failure(query.lastError().text());
    }

    if (!query.exec("SELECT MAX(version) FROM schema_version")) {
        return Result<int>::failure(query.lastError().text());
    }

    if (query.next() && !query.isNull(0)) {
        return Result<int>::success(query.value(0).toInt());
    }

    return Result<int>::success(0);
}

Result<void> SqliteMigrator::applyVersion1(QSqlDatabase& database)
{
    QSqlQuery query(database);

    const QString createCaptures =
        "CREATE TABLE IF NOT EXISTS captures ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "file_path TEXT NOT NULL,"
        "thumbnail_path TEXT NOT NULL,"
        "width INTEGER NOT NULL,"
        "height INTEGER NOT NULL,"
        "format TEXT NOT NULL,"
        "created_at TEXT NOT NULL,"
        "source_screen TEXT NOT NULL,"
        "deleted INTEGER NOT NULL DEFAULT 0"
        ")";

    if (!query.exec(createCaptures)) {
        return Result<void>::failure(query.lastError().text());
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_captures_created_at ON captures(created_at DESC)")) {
        return Result<void>::failure(query.lastError().text());
    }

    query.prepare("INSERT INTO schema_version(version, applied_at) VALUES(1, datetime('now'))");
    if (!query.exec()) {
        return Result<void>::failure(query.lastError().text());
    }

    return Result<void>::success();
}

Result<void> SqliteMigrator::applyVersion2(QSqlDatabase& database)
{
    QSqlQuery query(database);

    const QString createPinnedItems =
        "CREATE TABLE IF NOT EXISTS pinned_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "image_png BLOB NOT NULL,"
        "source INTEGER NOT NULL,"
        "x INTEGER NOT NULL,"
        "y INTEGER NOT NULL,"
        "width INTEGER NOT NULL,"
        "height INTEGER NOT NULL,"
        "opacity REAL NOT NULL,"
        "scale REAL NOT NULL,"
        "rotation INTEGER NOT NULL,"
        "flip_h INTEGER NOT NULL,"
        "flip_v INTEGER NOT NULL,"
        "click_through INTEGER NOT NULL,"
        "visible INTEGER NOT NULL,"
        "closed INTEGER NOT NULL DEFAULT 0,"
        "created_at TEXT NOT NULL,"
        "updated_at TEXT NOT NULL"
        ")";

    if (!query.exec(createPinnedItems)) {
        return Result<void>::failure(query.lastError().text());
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_pinned_items_closed ON pinned_items(closed, updated_at)")) {
        return Result<void>::failure(query.lastError().text());
    }

    query.prepare("INSERT INTO schema_version(version, applied_at) VALUES(2, datetime('now'))");
    if (!query.exec()) {
        return Result<void>::failure(query.lastError().text());
    }

    return Result<void>::success();
}

Result<void> SqliteMigrator::applyVersion3(QSqlDatabase& database)
{
    QSqlQuery query(database);

    if (!query.exec("ALTER TABLE pinned_items ADD COLUMN always_on_top INTEGER NOT NULL DEFAULT 1")) {
        return Result<void>::failure(query.lastError().text());
    }

    query.prepare("INSERT INTO schema_version(version, applied_at) VALUES(3, datetime('now'))");
    if (!query.exec()) {
        return Result<void>::failure(query.lastError().text());
    }

    return Result<void>::success();
}

Result<void> SqliteMigrator::applyVersion4(QSqlDatabase& database)
{
    QSqlQuery query(database);

    if (!query.exec("ALTER TABLE pinned_items ADD COLUMN device_pixel_ratio REAL NOT NULL DEFAULT 1.0")) {
        return Result<void>::failure(query.lastError().text());
    }

    query.prepare("INSERT INTO schema_version(version, applied_at) VALUES(4, datetime('now'))");
    if (!query.exec()) {
        return Result<void>::failure(query.lastError().text());
    }

    return Result<void>::success();
}

Result<void> SqliteMigrator::applyVersion5(QSqlDatabase& database)
{
    QSqlQuery query(database);

    if (!query.exec("ALTER TABLE captures ADD COLUMN device_pixel_ratio REAL NOT NULL DEFAULT 1.0")) {
        return Result<void>::failure(query.lastError().text());
    }

    query.prepare("INSERT INTO schema_version(version, applied_at) VALUES(5, datetime('now'))");
    if (!query.exec()) {
        return Result<void>::failure(query.lastError().text());
    }

    return Result<void>::success();
}

} // namespace snappaste
