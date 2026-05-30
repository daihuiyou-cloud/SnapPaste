#pragma once

#include "domain/pin/IClipboardImageProvider.h"
#include "domain/pin/IPinnedItemRepository.h"
#include "shared/utils/ITimeProvider.h"

#include <QString>

namespace snappaste {

class PinnedImageService final {
public:
    PinnedImageService(IClipboardImageProvider& clipboardProvider,
                       IPinnedItemRepository& repository,
                       ITimeProvider& timeProvider);

    Result<PinnedItem> createFromImage(QImage image, PinSource source);
    Result<PinnedItem> createFromFile(const QString& filePath);
    Result<PinnedItem> createFromClipboard();
    Result<QVector<PinnedItem>> restorePinnedItems();
    Result<void> updateState(qint64 id, const PinnedImageState& state);
    Result<void> setAllVisible(bool visible);
    Result<void> close(qint64 id);

private:
    static PinnedImageState defaultStateFor(const QImage& image);

    IClipboardImageProvider& clipboardProvider_;
    IPinnedItemRepository& repository_;
    ITimeProvider& timeProvider_;
};

} // namespace snappaste
