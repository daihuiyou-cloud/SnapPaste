#include "domain/pin/PinnedImageService.h"

#include "shared/utils/TimeProvider.h"

#include <utility>

namespace snappaste {

PinnedImageService::PinnedImageService(IClipboardImageProvider& clipboardProvider,
                                       IPinnedItemRepository& repository)
    : clipboardProvider_(clipboardProvider)
    , repository_(repository)
{
}

Result<PinnedItem> PinnedImageService::createFromImage(QImage image, PinSource source)
{
    if (image.isNull()) {
        return Result<PinnedItem>::failure("Cannot pin an empty image.");
    }

    PinnedItem item;
    item.image = std::move(image);
    item.source = source;
    item.state = defaultStateFor(item.image);
    item.createdAt = TimeProvider::nowUtc();
    item.updatedAt = item.createdAt;

    return repository_.add(item);
}

Result<PinnedItem> PinnedImageService::createFromFile(const QString& filePath)
{
    QImage fileImage(filePath);
    if (fileImage.isNull()) {
        return Result<PinnedItem>::failure("Failed to load image file.");
    }

    return createFromImage(std::move(fileImage), PinSource::File);
}

Result<PinnedItem> PinnedImageService::createFromClipboard()
{
    auto imageResult = clipboardProvider_.imageFromClipboard();
    if (imageResult.isError()) {
        return Result<PinnedItem>::failure(imageResult.error());
    }

    return createFromImage(imageResult.value(), PinSource::Clipboard);
}

Result<QVector<PinnedItem>> PinnedImageService::restorePinnedItems()
{
    return repository_.restoreActive();
}

Result<void> PinnedImageService::updateState(qint64 id, const PinnedImageState& state)
{
    if (id <= 0) {
        return Result<void>::failure("Invalid pinned item id.");
    }

    return repository_.updateState(id, normalizedState(state));
}

Result<void> PinnedImageService::setAllVisible(bool visible)
{
    return repository_.setAllVisible(visible);
}

Result<void> PinnedImageService::close(qint64 id)
{
    if (id <= 0) {
        return Result<void>::failure("Invalid pinned item id.");
    }

    return repository_.close(id);
}

PinnedImageState PinnedImageService::defaultStateFor(const QImage& image)
{
    PinnedImageState state;
    state.position = QPoint(160, 140);
    state.size = image.size();
    state.opacity = 1.0;
    state.transform.scale = 1.0;
    state.options.alwaysOnTop = true;
    state.options.clickThrough = false;
    state.options.visible = true;
    return normalizedState(state);
}

} // namespace snappaste
