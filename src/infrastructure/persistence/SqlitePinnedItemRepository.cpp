#include "infrastructure/persistence/SqlitePinnedItemRepository.h"

#include <QBuffer>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace nanosnap {

SqlitePinnedItemRepository::SqlitePinnedItemRepository(QString databasePath)
    : connection_(std::move(databasePath))
{
}

Result<PinnedItem> SqlitePinnedItemRepository::add(const PinnedItem& item)
{
    auto dbResult = readyDatabase();
    if (dbResult.isError()) {
        return Result<PinnedItem>::failure(dbResult.error());
    }

    QSqlQuery query(dbResult.value());
    query.prepare(
        "INSERT INTO pinned_items("
        "image_png, source, x, y, width, height, opacity, scale, rotation, flip_h, flip_v, "
        "always_on_top, click_through, visible, closed, created_at, updated_at"
        ") VALUES("
        ":image_png, :source, :x, :y, :width, :height, :opacity, :scale, :rotation, :flip_h, :flip_v, "
        ":always_on_top, :click_through, :visible, 0, :created_at, :updated_at)");
    query.bindValue(":image_png", encodeImage(item.image));
    query.bindValue(":source", static_cast<int>(item.source));
    query.bindValue(":x", item.state.position.x());
    query.bindValue(":y", item.state.position.y());
    query.bindValue(":width", item.state.size.width());
    query.bindValue(":height", item.state.size.height());
    query.bindValue(":opacity", item.state.opacity);
    query.bindValue(":scale", item.state.transform.scale);
    query.bindValue(":rotation", item.state.transform.rotationDegrees);
    query.bindValue(":flip_h", item.state.transform.flippedHorizontally);
    query.bindValue(":flip_v", item.state.transform.flippedVertically);
    query.bindValue(":always_on_top", item.state.options.alwaysOnTop);
    query.bindValue(":click_through", item.state.options.clickThrough);
    query.bindValue(":visible", item.state.options.visible);
    query.bindValue(":created_at", item.createdAt.toUTC().toString(Qt::ISODate));
    query.bindValue(":updated_at", item.updatedAt.toUTC().toString(Qt::ISODate));

    if (!query.exec()) {
        return Result<PinnedItem>::failure(query.lastError().text());
    }

    auto saved = item;
    saved.id = query.lastInsertId().toLongLong();
    return Result<PinnedItem>::success(saved);
}

Result<QVector<PinnedItem>> SqlitePinnedItemRepository::restoreActive()
{
    auto dbResult = readyDatabase();
    if (dbResult.isError()) {
        return Result<QVector<PinnedItem>>::failure(dbResult.error());
    }

    QSqlQuery query(dbResult.value());
    if (!query.exec(
            "SELECT id, image_png, source, x, y, width, height, opacity, scale, rotation, "
            "flip_h, flip_v, always_on_top, click_through, visible, created_at, updated_at "
            "FROM pinned_items WHERE closed = 0 ORDER BY updated_at ASC")) {
        return Result<QVector<PinnedItem>>::failure(query.lastError().text());
    }

    QVector<PinnedItem> items;
    while (query.next()) {
        items.push_back(readItem(query));
    }

    return Result<QVector<PinnedItem>>::success(items);
}

Result<void> SqlitePinnedItemRepository::updateState(qint64 id, const PinnedImageState& state)
{
    auto dbResult = readyDatabase();
    if (dbResult.isError()) {
        return Result<void>::failure(dbResult.error());
    }

    QSqlQuery query(dbResult.value());
    query.prepare(
        "UPDATE pinned_items SET x = :x, y = :y, width = :width, height = :height, "
        "opacity = :opacity, scale = :scale, rotation = :rotation, flip_h = :flip_h, flip_v = :flip_v, "
        "always_on_top = :always_on_top, click_through = :click_through, visible = :visible, "
        "updated_at = datetime('now') WHERE id = :id");
    query.bindValue(":id", id);
    query.bindValue(":x", state.position.x());
    query.bindValue(":y", state.position.y());
    query.bindValue(":width", state.size.width());
    query.bindValue(":height", state.size.height());
    query.bindValue(":opacity", state.opacity);
    query.bindValue(":scale", state.transform.scale);
    query.bindValue(":rotation", state.transform.rotationDegrees);
    query.bindValue(":flip_h", state.transform.flippedHorizontally);
    query.bindValue(":flip_v", state.transform.flippedVertically);
    query.bindValue(":always_on_top", state.options.alwaysOnTop);
    query.bindValue(":click_through", state.options.clickThrough);
    query.bindValue(":visible", state.options.visible);

    if (!query.exec()) {
        return Result<void>::failure(query.lastError().text());
    }

    return Result<void>::success();
}

Result<void> SqlitePinnedItemRepository::setAllVisible(bool visible)
{
    auto dbResult = readyDatabase();
    if (dbResult.isError()) {
        return Result<void>::failure(dbResult.error());
    }

    QSqlQuery query(dbResult.value());
    query.prepare(
        "UPDATE pinned_items SET visible = :visible, click_through = 0, updated_at = datetime('now') "
        "WHERE closed = 0");
    query.bindValue(":visible", visible);

    if (!query.exec()) {
        return Result<void>::failure(query.lastError().text());
    }

    return Result<void>::success();
}

Result<void> SqlitePinnedItemRepository::close(qint64 id)
{
    auto dbResult = readyDatabase();
    if (dbResult.isError()) {
        return Result<void>::failure(dbResult.error());
    }

    QSqlQuery query(dbResult.value());
    query.prepare("UPDATE pinned_items SET closed = 1, updated_at = datetime('now') WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        return Result<void>::failure(query.lastError().text());
    }

    return Result<void>::success();
}

Result<QSqlDatabase> SqlitePinnedItemRepository::readyDatabase()
{
    auto dbResult = connection_.database();
    if (dbResult.isError()) {
        return dbResult;
    }

    if (!migrated_) {
        const auto migrateResult = migrator_.migrate(dbResult.value());
        if (migrateResult.isError()) {
            return Result<QSqlDatabase>::failure(migrateResult.error());
        }
        migrated_ = true;
    }

    return dbResult;
}

QByteArray SqlitePinnedItemRepository::encodeImage(const QImage& image)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

QImage SqlitePinnedItemRepository::decodeImage(const QByteArray& bytes)
{
    QImage image;
    image.loadFromData(bytes, "PNG");
    return image;
}

PinnedItem SqlitePinnedItemRepository::readItem(const QSqlQuery& query)
{
    PinnedItem item;
    item.id = query.value(0).toLongLong();
    item.image = decodeImage(query.value(1).toByteArray());
    item.source = static_cast<PinSource>(query.value(2).toInt());
    item.state.position = QPoint(query.value(3).toInt(), query.value(4).toInt());
    item.state.size = QSize(query.value(5).toInt(), query.value(6).toInt());
    item.state.opacity = query.value(7).toDouble();
    item.state.transform.scale = query.value(8).toDouble();
    item.state.transform.rotationDegrees = query.value(9).toInt();
    item.state.transform.flippedHorizontally = query.value(10).toBool();
    item.state.transform.flippedVertically = query.value(11).toBool();
    item.state.options.alwaysOnTop = query.value(12).toBool();
    item.state.options.clickThrough = query.value(13).toBool();
    item.state.options.visible = query.value(14).toBool();
    item.createdAt = QDateTime::fromString(query.value(15).toString(), Qt::ISODate);
    item.updatedAt = QDateTime::fromString(query.value(16).toString(), Qt::ISODate);
    return item;
}

} // namespace nanosnap
