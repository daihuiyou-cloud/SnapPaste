#include <QCoreApplication>
#include "domain/capture/CaptureWorkflow.h"

namespace snappaste {

CaptureWorkflow::CaptureWorkflow(IScreenCaptureService& captureService,
                                 IImageStorage& imageStorage,
                                 IHistoryRepository& historyRepository,
                                 ISettingsRepository& settingsRepository,
                                 ITimeProvider& timeProvider)
    : captureService_(captureService)
    , imageStorage_(imageStorage)
    , historyRepository_(historyRepository)
    , settingsRepository_(settingsRepository)
    , timeProvider_(timeProvider)
{
}

Result<void> CaptureWorkflow::validateRegion(const QRect& region)
{
    if (!region.isValid()) {
        return Result<void>::failure(QCoreApplication::translate("AppErrors", "Selection is empty. Please select a region to capture."));
    }
    if (region.width() < 8 || region.height() < 8) {
        return Result<void>::failure(QCoreApplication::translate("AppErrors", "Selection too small. Please select a larger area."));
    }
    return Result<void>::success();
}

Result<QImage> CaptureWorkflow::captureRegion(const QRect& region)
{
    QMutexLocker lock(&mutex_);
    auto validation = validateRegion(region);
    if (validation.isError()) {
        return Result<QImage>::failure(validation.error());
    }
    return captureService_.captureRegion(region);
}

Result<QImage> CaptureWorkflow::captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments)
{
    QMutexLocker lock(&mutex_);
    auto validation = validateRegion(region);
    if (validation.isError()) {
        return Result<QImage>::failure(validation.error());
    }
    return captureService_.captureRegion(region, segments);
}

Result<CaptureRecord> CaptureWorkflow::saveCapturedImage(const QImage& image, const QString& sourceScreen)
{
    QMutexLocker lock(&mutex_);
    if (image.isNull()) {
        return Result<CaptureRecord>::failure(QCoreApplication::translate("AppErrors", "No image data to save. Try capturing again."));
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
    record.devicePixelRatio = image.devicePixelRatio();
    record.format = settingsResult.value().imageFormat;
    record.createdAt = timeProvider_.nowUtc();
    record.sourceScreen = sourceScreen;

    return historyRepository_.add(record);
}

} // namespace snappaste
