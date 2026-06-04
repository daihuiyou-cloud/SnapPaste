#include "infrastructure/persistence/SqliteHistoryRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace snappaste {

SqliteHistoryRepository::SqliteHistoryRepository(SqliteConnection& connection)
    : connection_(connection)
{
}

Result<CaptureRecord> SqliteHistoryRepository::add(const CaptureRecord& record)
{
    auto dbResult = readyDatabase();
    if (dbResult.isError()) {
        return Result<CaptureRecord>::failure(dbResult.error());
    }

    QSqlQuery query(dbResult.value());
    query.prepare(
        "INSERT INTO captures(file_path, thumbnail_path, width, height, device_pixel_ratio, format, created_at, source_screen, deleted) "
        "VALUES(:file_path, :thumbnail_path, :width, :height, :device_pixel_ratio, :format, :created_at, :source_screen, 0)");
    query.bindValue(":file_path", record.filePath);
    query.bindValue(":thumbnail_path", record.thumbnailPath);
    query.bindValue(":width", record.width);
    query.bindValue(":height", record.height);
    query.bindValue(":device_pixel_ratio", record.devicePixelRatio);
    query.bindValue(":format", record.format);
    query.bindValue(":created_at", record.createdAt.toUTC().toString(Qt::ISODate));
    query.bindValue(":source_screen", record.sourceScreen);

    if (!query.exec()) {
        return Result<CaptureRecord>::failure(query.lastError().text());
    }

    auto saved = record;
    saved.id = query.lastInsertId().toLongLong();
    return Result<CaptureRecord>::success(std::move(saved));
}

Result<QVector<CaptureRecord>> SqliteHistoryRepository::recent(int limit)
{
    auto dbResult = readyDatabase();
    if (dbResult.isError()) {
        return Result<QVector<CaptureRecord>>::failure(dbResult.error());
    }

    QSqlQuery query(dbResult.value());
    query.prepare(
        "SELECT id, file_path, thumbnail_path, width, height, device_pixel_ratio, format, created_at, source_screen, deleted "
        "FROM captures WHERE deleted = 0 ORDER BY created_at DESC LIMIT :limit");
    query.bindValue(":limit", limit);

    if (!query.exec()) {
        return Result<QVector<CaptureRecord>>::failure(query.lastError().text());
    }

    QVector<CaptureRecord> records;
    records.reserve(limit);
    while (query.next()) {
        records.push_back(readRecord(query));
    }

    return Result<QVector<CaptureRecord>>::success(std::move(records));
}

Result<void> SqliteHistoryRepository::markDeleted(qint64 id)
{
    auto dbResult = readyDatabase();
    if (dbResult.isError()) {
        return Result<void>::failure(dbResult.error());
    }

    QSqlQuery query(dbResult.value());
    query.prepare("UPDATE captures SET deleted = 1 WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        return Result<void>::failure(query.lastError().text());
    }

    return Result<void>::success();
}

Result<QSqlDatabase> SqliteHistoryRepository::readyDatabase()
{
    return connection_.database();
}

CaptureRecord SqliteHistoryRepository::readRecord(const QSqlQuery& query)
{
    CaptureRecord record;
    record.id = query.value(ColId).toLongLong();
    record.filePath = query.value(ColFilePath).toString();
    record.thumbnailPath = query.value(ColThumbnailPath).toString();
    record.width = query.value(ColWidth).toInt();
    record.height = query.value(ColHeight).toInt();
    record.devicePixelRatio = query.value(ColDevicePixelRatio).toDouble();
    record.format = query.value(ColFormat).toString();
    record.createdAt = QDateTime::fromString(query.value(ColCreatedAt).toString(), Qt::ISODate);
    if (!record.createdAt.isValid()) {
        record.createdAt = QDateTime::currentDateTimeUtc();
    } else {
        record.createdAt.setTimeSpec(Qt::UTC);
    }
    record.sourceScreen = query.value(ColSourceScreen).toString();
    record.deleted = query.value(ColDeleted).toBool();
    return record;
}

} // namespace snappaste
