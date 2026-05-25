#include "domain/capture/CaptureWorkflow.h"

#include "shared/utils/TimeProvider.h"

namespace snappaste {

CaptureWorkflow::CaptureWorkflow(IScreenCaptureService& captureService,
                                 IImageStorage& imageStorage,
                                 IHistoryRepository& historyRepository,
                                 ISettingsRepository& settingsRepository)
    : captureService_(captureService)
    , imageStorage_(imageStorage)
    , historyRepository_(historyRepository)
    , settingsRepository_(settingsRepository)
{
}

Result<QImage> CaptureWorkflow::captureRegion(const QRect& region)
{
    if (!region.isValid() || region.width() < 2 || region.height() < 2) {
        return Result<QImage>::failure("Capture region is too small.");
    }

    return captureService_.captureRegion(region);
}

Result<QImage> CaptureWorkflow::captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments)
{
    if (!region.isValid() || region.width() < 2 || region.height() < 2) {
        return Result<QImage>::failure("Capture region is too small.");
    }

    return captureService_.captureRegion(region, segments);
}

Result<CaptureRecord> CaptureWorkflow::saveCapturedImage(const QImage& image, const QString& sourceScreen)
{
    if (image.isNull()) {
        return Result<CaptureRecord>::failure("Cannot save an empty image.");
    }

    const auto settingsResult = settingsRepository_.load();
    if (settingsResult.isError()) {
        return Result<CaptureRecord>::failure(settingsResult.error());
    }

    const auto storedResult = imageStorage_.saveCapture(
        image, settingsResult.value().saveDirectory, settingsResult.value().imageFormat);
    if (storedResult.isError()) {
        return Result<CaptureRecord>::failure(storedResult.error());
    }

    CaptureRecord record;
    record.filePath = storedResult.value().filePath;
    record.thumbnailPath = storedResult.value().thumbnailPath;
    record.width = image.width();
    record.height = image.height();
    record.format = settingsResult.value().imageFormat;
    record.createdAt = TimeProvider::nowUtc();
    record.sourceScreen = sourceScreen;

    return historyRepository_.add(record);
}

} // namespace snappaste
