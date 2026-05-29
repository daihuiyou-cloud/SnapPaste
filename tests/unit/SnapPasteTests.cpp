#include "domain/capture/CaptureSelectionHistory.h"
#include "domain/capture/CaptureWorkflow.h"
#include "domain/capture/IImageStorage.h"
#include "domain/capture/IScreenCaptureService.h"
#include "domain/capture/IScreenPixelSampler.h"
#include "domain/capture/IScreenRegionDetector.h"
#include "domain/history/IHistoryRepository.h"
#include "domain/pin/PinnedItem.h"
#include "domain/settings/SettingsService.h"
#include "presentation/capture_actions/CaptureActionBar.h"
#include "presentation/viewmodels/CaptureViewModel.h"
#include "shared/events/EventHub.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

#include <memory>

using namespace snappaste;

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

class SnapPasteUnitTests final : public QObject {
    Q_OBJECT

private slots:
    void settingsServiceRejectsEmptyDirectory()
    {
        FakeSettingsRepository repository;
        SettingsService service(repository);

        AppSettings settings;
        settings.saveDirectory = "";
        settings.imageFormat = "png";

        const auto result = service.save(settings);

        QVERIFY(result.isError());
        QVERIFY(!repository.saved);
    }

    void pinnedImageStateIsClamped()
    {
        PinnedImageState state;
        state.size = QSize(1, 2);
        state.opacity = 2.0;
        state.transform.scale = 0.01;
        state.transform.rotationDegrees = -90;

        const auto normalized = normalizedState(state);

        QCOMPARE(normalized.size.width(), 1);
        QCOMPARE(normalized.size.height(), 2);
        QCOMPARE(normalized.opacity, 1.0);
        QCOMPARE(normalized.transform.scale, 0.1);
        QCOMPARE(normalized.transform.rotationDegrees, 270);
    }

    void captureSelectionHistoryKeepsRecentUniqueRegions()
    {
        CaptureSelectionHistory history(3);

        history.add(QRect(0, 0, 10, 10));
        history.add(QRect(20, 20, 10, 10));
        history.add(QRect(40, 40, 10, 10));
        history.add(QRect(60, 60, 10, 10));
        history.add(QRect(20, 20, 10, 10));

        QCOMPARE(history.size(), 3);
        QCOMPARE(history.entries().at(0), QRect(20, 20, 10, 10));
        QCOMPARE(history.entries().at(1), QRect(60, 60, 10, 10));
        QCOMPARE(history.entries().at(2), QRect(40, 40, 10, 10));
    }

    void captureSelectionHistoryCyclesBothDirections()
    {
        CaptureSelectionHistory history(3);
        history.add(QRect(0, 0, 10, 10));
        history.add(QRect(20, 20, 10, 10));

        QCOMPARE(history.previous(), QRect(20, 20, 10, 10));
        QCOMPARE(history.previous(), QRect(0, 0, 10, 10));
        QCOMPARE(history.previous(), QRect(20, 20, 10, 10));
        QCOMPARE(history.next(), QRect(0, 0, 10, 10));
    }

    void captureActionBarAnchorsInsideAvailableGeometry()
    {
        const QRect available(0, 0, 800, 600);
        const QSize barSize(220, 50);

        const auto bottomRight = CaptureActionBar::anchoredPosition(QRect(740, 560, 50, 30), barSize, available);
        QVERIFY(bottomRight.x() + barSize.width() <= available.right());
        QVERIFY(bottomRight.y() + barSize.height() <= available.bottom());

        const auto topLeft = CaptureActionBar::anchoredPosition(QRect(4, 4, 40, 30), barSize, available);
        QVERIFY(topLeft.x() >= available.left());
        QVERIFY(topLeft.y() >= available.top());
    }

    void captureActionBarAnchorsInsideSingleScreenSegment()
    {
        const QRect secondScreen(800, 0, 800, 600);
        const QSize barSize(160, 46);

        const auto position = CaptureActionBar::anchoredPosition(QRect(1510, 540, 70, 40), barSize, secondScreen);

        QVERIFY(position.x() >= secondScreen.left());
        QVERIFY(position.x() + barSize.width() <= secondScreen.right());
        QVERIFY(position.y() >= secondScreen.top());
        QVERIFY(position.y() + barSize.height() <= secondScreen.bottom());
    }

    void captureActionBarKeyboardShortcutsEmitActions()
    {
        CaptureActionBar actionBar;
        QSignalSpy copySpy(&actionBar, &CaptureActionBar::copyRequested);
        QSignalSpy pinSpy(&actionBar, &CaptureActionBar::pinRequested);
        QSignalSpy saveSpy(&actionBar, &CaptureActionBar::saveRequested);
        QSignalSpy editSpy(&actionBar, &CaptureActionBar::editRequested);
        QSignalSpy ocrSpy(&actionBar, &CaptureActionBar::ocrRequested);
        QSignalSpy cancelSpy(&actionBar, &CaptureActionBar::cancelRequested);

        QKeyEvent enterEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(&actionBar, &enterEvent);
        QKeyEvent pinEvent(QEvent::KeyPress, Qt::Key_F3, Qt::NoModifier);
        QApplication::sendEvent(&actionBar, &pinEvent);
        QKeyEvent saveEvent(QEvent::KeyPress, Qt::Key_S, Qt::ControlModifier);
        QApplication::sendEvent(&actionBar, &saveEvent);
        QKeyEvent editEvent(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
        QApplication::sendEvent(&actionBar, &editEvent);
        QKeyEvent ocrEvent(QEvent::KeyPress, Qt::Key_O, Qt::NoModifier);
        QApplication::sendEvent(&actionBar, &ocrEvent);
        QKeyEvent cancelEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(&actionBar, &cancelEvent);

        QCOMPARE(copySpy.size(), 1);
        QCOMPARE(pinSpy.size(), 1);
        QCOMPARE(saveSpy.size(), 1);
        QCOMPARE(editSpy.size(), 1);
        QCOMPARE(ocrSpy.size(), 1);
        QCOMPARE(cancelSpy.size(), 1);
    }

    void captureWorkflowUsesResolvedScreenSegments()
    {
        FakeScreenCaptureService captureService;
        FakeImageStorage imageStorage;
        FakeHistoryRepository historyRepository;
        FakeSettingsRepository settingsRepository;
        CaptureWorkflow workflow(captureService, imageStorage, historyRepository, settingsRepository);

        QVector<ScreenCaptureSegment> segments;
        ScreenCaptureSegment left;
        left.logicalRegion = QRect(0, 0, 20, 10);
        left.logicalScreenGeometry = QRect(0, 0, 20, 10);
        left.screenName = "left";
        segments.push_back(left);

        ScreenCaptureSegment right;
        right.logicalRegion = QRect(20, 0, 20, 10);
        right.logicalScreenGeometry = QRect(20, 0, 20, 10);
        right.screenName = "right";
        segments.push_back(right);

        const auto result = workflow.captureRegion(QRect(0, 0, 40, 10), segments);

        QVERIFY(result.isOk());
        QVERIFY(captureService.usedSegmentRegion);
        QVERIFY(!captureService.usedPlainRegion);
        QCOMPARE(captureService.receivedSegments.size(), 2);
        QCOMPARE(result.value().size(), QSize(40, 10));
    }

    void captureViewModelSavesExplicitImageWithoutReplacingCurrentImage()
    {
        FakeScreenCaptureService captureService;
        FakeImageStorage imageStorage;
        imageStorage.shouldSucceed = true;
        FakeHistoryRepository historyRepository;
        FakeSettingsRepository settingsRepository;
        settingsRepository.settings.saveDirectory = "captures";
        settingsRepository.settings.imageFormat = "png";
        EventHub eventHub;
        CaptureWorkflow workflow(captureService, imageStorage, historyRepository, settingsRepository);
        CaptureViewModel viewModel(workflow, eventHub);

        QImage current(12, 8, QImage::Format_RGB32);
        current.fill(Qt::red);
        QImage pinned(7, 5, QImage::Format_RGB32);
        pinned.fill(Qt::blue);
        viewModel.setCurrentImage(current);
        QSignalSpy savedSpy(&viewModel, &CaptureViewModel::saved);

        viewModel.saveImage(pinned, "pin");

        QCOMPARE(savedSpy.size(), 1);
        QCOMPARE(imageStorage.savedImage.size(), pinned.size());
        QCOMPARE(viewModel.currentImage().size(), current.size());
    }
};

QTEST_MAIN(SnapPasteUnitTests)

#include "SnapPasteTests.moc"
