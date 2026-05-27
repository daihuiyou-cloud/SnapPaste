#pragma once

#include "domain/capture/IImageStorage.h"
#include "domain/capture/IScreenCaptureService.h"
#include "domain/history/IHistoryRepository.h"
#include "domain/settings/ISettingsRepository.h"
#include "shared/result/Result.h"
#include "shared/types/CaptureRecord.h"

#include <QImage>
#include <QRect>

namespace snappaste {

class CaptureWorkflow final {
public:
    CaptureWorkflow(IScreenCaptureService& captureService,
                    IImageStorage& imageStorage,
                    IHistoryRepository& historyRepository,
                    ISettingsRepository& settingsRepository);

    Result<QImage> captureRegion(const QRect& region);
    Result<QImage> captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments);
    Result<CaptureRecord> saveCapturedImage(const QImage& image, const QString& sourceScreen = "primary");

private:
    static Result<void> validateRegion(const QRect& region);

    IScreenCaptureService& captureService_;
    IImageStorage& imageStorage_;
    IHistoryRepository& historyRepository_;
    ISettingsRepository& settingsRepository_;
};

} // namespace snappaste
