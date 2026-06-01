#include "infrastructure/persistence/SqlitePinnedItemRepository.h"

#include <QBuffer>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace snappaste {

SqlitePinnedItemRepository::SqlitePinnedItemRepository(SqliteConnection& connection)
    : connection_(connection)
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
        "always_on_top, click_through, visible, device_pixel_ratio, closed, created_at, updated_at"
        ") VALUES("
        ":image_png, :source, :x, :y, :width, :height, :opacity, :scale, :rotation, :flip_h, :flip_v, "
        ":always_on_top, :click_through, :visible, :device_pixel_ratio, 0, :created_at, :updated_at)");
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
    query.bindValue(":device_pixel_ratio", item.state.devicePixelRatio);
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
            "flip_h, flip_v, always_on_top, click_through, visible, device_pixel_ratio, created_at, updated_at "
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
        "device_pixel_ratio = :device_pixel_ratio, "
        "updated_at = strftime('%Y-%m-%dT%H:%M:%SZ', 'now') WHERE id = :id");
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
    query.bindValue(":device_pixel_ratio", state.devicePixelRatio);

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
        "UPDATE pinned_items SET visible = :visible, updated_at = strftime('%Y-%m-%dT%H:%M:%SZ', 'now') "
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
    query.prepare("UPDATE pinned_items SET closed = 1, updated_at = strftime('%Y-%m-%dT%H:%M:%SZ', 'now') WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        return Result<void>::failure(query.lastError().text());
    }

    return Result<void>::success();
}

Result<QSqlDatabase> SqlitePinnedItemRepository::readyDatabase()
{
    return connection_.database();
}

QByteArray SqlitePinnedItemRepository::encodeImage(const QImage& image)
{
    if (image.isNull()) {
        return {};
    }
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

QImage SqlitePinnedItemRepository::decodeImage(const QByteArray& bytes)
{
    if (bytes.isEmpty()) {
        return {};
    }
    QImage image;
    image.loadFromData(bytes, "PNG");
    return image;
}

PinnedItem SqlitePinnedItemRepository::readItem(const QSqlQuery& query)
{
    PinnedItem item;
    item.id = query.value(PColId).toLongLong();
    item.image = decodeImage(query.value(PColImagePng).toByteArray());
    if (item.image.isNull()) {
        item.image = QImage(1, 1, QImage::Format_ARGB32);
        item.image.fill(Qt::transparent);
    }
    item.source = static_cast<PinSource>(query.value(PColSource).toInt());
    item.state.position = QPoint(query.value(PColX).toInt(), query.value(PColY).toInt());
    item.state.size = QSize(query.value(PColWidth).toInt(), query.value(PColHeight).toInt());
    item.state.opacity = query.value(PColOpacity).toDouble();
    item.state.transform.scale = query.value(PColScale).toDouble();
    item.state.transform.rotationDegrees = query.value(PColRotation).toInt();
    item.state.transform.flippedHorizontally = query.value(PColFlipH).toBool();
    item.state.transform.flippedVertically = query.value(PColFlipV).toBool();
    item.state.options.alwaysOnTop = query.value(PColAlwaysOnTop).toBool();
    item.state.options.clickThrough = query.value(PColClickThrough).toBool();
    item.state.options.visible = query.value(PColVisible).toBool();
    item.state.devicePixelRatio = query.value(PColDevicePixelRatio).toDouble();
    item.image.setDevicePixelRatio(item.state.devicePixelRatio);
    item.createdAt = QDateTime::fromString(query.value(PColCreatedAt).toString(), Qt::ISODate);
    item.createdAt.setTimeSpec(Qt::UTC);
    item.updatedAt = QDateTime::fromString(query.value(PColUpdatedAt).toString(), Qt::ISODate);
    item.updatedAt.setTimeSpec(Qt::UTC);
    return item;
}

} // namespace snappaste
