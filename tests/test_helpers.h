#pragma once

#include "domain/capture/IImageStorage.h"
#include "domain/capture/IScreenCaptureService.h"
#include "domain/capture/IScreenPixelSampler.h"
#include "domain/capture/IScreenRegionDetector.h"
#include "domain/history/IHistoryRepository.h"
#include "domain/pin/IClipboardImageProvider.h"
#include "domain/settings/ISettingsRepository.h"
#include "shared/result/Result.h"

#include <QImage>
#include <QRect>
#include <QString>
#include <QVector>

#include <optional>
#include <utility>

namespace snappaste {

class ScopedEnvVar final {
public:
    ScopedEnvVar(const char* name, const QString& value)
        : name_(name)
    {
        previous_ = qEnvironmentVariable(name);
        if (!value.isEmpty()) {
            qputenv(name_, value.toLocal8Bit());
        } else {
            qunsetenv(name_);
        }
    }

    ~ScopedEnvVar()
    {
        if (!previous_.isEmpty()) {
            qputenv(name_, previous_.toLocal8Bit());
        } else {
            qunsetenv(name_);
        }
    }

    ScopedEnvVar(const ScopedEnvVar&) = delete;
    ScopedEnvVar& operator=(const ScopedEnvVar&) = delete;

private:
    const char* name_;
    QString previous_;
};

// ---------------------------------------------------------------------------
// Fake implementations of domain interfaces for testing
// ---------------------------------------------------------------------------

class FakeClipboardImageProvider final : public IClipboardImageProvider {
public:
    Result<QImage> imageFromClipboard() override
    {
        if (image.isNull()) {
            return Result<QImage>::failure("No clipboard image.");
        }
        return Result<QImage>::success(image);
    }

    QImage image;
};

class FakeSettingsRepository final : public ISettingsRepository {
public:
    Result<AppSettings> load() override
    {
        return Result<AppSettings>::success(settings);
    }

    Result<void> save(const AppSettings& newSettings) override
    {
        settings = newSettings;
        saved = true;
        return Result<void>::success();
    }

    AppSettings defaultSettings() override
    {
        return {};
    }

    AppSettings settings;
    bool saved = false;
};

class FakeScreenCaptureService final : public IScreenCaptureService {
public:
    Result<QImage> capturePrimaryScreen() override
    {
        return Result<QImage>::failure("Not implemented.");
    }

    Result<QImage> captureRegion(const QRect& region) override
    {
        usedPlainRegion = true;
        QImage image(region.size(), QImage::Format_RGB32);
        image.fill(Qt::black);
        return Result<QImage>::success(image);
    }

    Result<QImage> captureRegion(const QRect& region, const QVector<ScreenCaptureSegment>& segments) override
    {
        usedSegmentRegion = true;
        receivedSegments = segments;
        QImage image(region.size(), QImage::Format_RGB32);
        image.fill(Qt::black);
        return Result<QImage>::success(image);
    }

    bool usedPlainRegion = false;
    bool usedSegmentRegion = false;
    QVector<ScreenCaptureSegment> receivedSegments;
};

class FakeImageStorage final : public IImageStorage {
public:
    Result<StoredImage> saveCapture(const QImage& image, const QString& directory, const QString& format) override
    {
        savedImage = image;
        savedDirectory = directory;
        savedFormat = format;
        if (shouldSucceed) {
            StoredImage stored;
            stored.filePath = "capture." + format;
            stored.thumbnailPath = "capture-thumb.jpg";
            return Result<StoredImage>::success(stored);
        }
        return Result<StoredImage>::failure("Not implemented.");
    }

    bool shouldSucceed = false;
    QImage savedImage;
    QString savedDirectory;
    QString savedFormat;
};

class FakeHistoryRepository final : public IHistoryRepository {
public:
    Result<CaptureRecord> add(const CaptureRecord& record) override
    {
        return Result<CaptureRecord>::success(record);
    }

    Result<QVector<CaptureRecord>> recent(int limit) override
    {
        Q_UNUSED(limit)
        return Result<QVector<CaptureRecord>>::success({});
    }

    Result<void> markDeleted(qint64 id) override
    {
        Q_UNUSED(id)
        return Result<void>::success();
    }
};

class FakeScreenRegionDetector final : public IScreenRegionDetector {
public:
    QVector<QRect> regionsAt(const QPoint& globalPosition, const QRect& desktopBounds) override
    {
        Q_UNUSED(globalPosition)
        Q_UNUSED(desktopBounds)
        return regions;
    }

    QVector<QRect> regions;
};

class FakeScreenPixelSampler final : public IScreenPixelSampler {
public:
    void refresh(const QRect& bounds) override
    {
        Q_UNUSED(bounds)
    }

    std::optional<QColor> sample(const QPoint& globalPosition) const override
    {
        Q_UNUSED(globalPosition)
        return QColor(Qt::white);
    }

    QImage sampleRegion(const QPoint& center, int halfExtent) const override
    {
        Q_UNUSED(center)
        Q_UNUSED(halfExtent)
        return {};
    }
};

} // namespace snappaste
