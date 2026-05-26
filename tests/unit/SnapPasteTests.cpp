#include "domain/capture/CaptureSelectionHistory.h"
#include "domain/capture/CaptureWorkflow.h"
#include "domain/capture/IImageStorage.h"
#include "domain/capture/IScreenCaptureService.h"
#include "domain/capture/IScreenPixelSampler.h"
#include "domain/capture/IScreenRegionDetector.h"
#include "domain/history/IHistoryRepository.h"
#include "domain/editor/AnnotationDocument.h"
#include "domain/pin/PinnedImageService.h"
#include "domain/settings/SettingsService.h"
#include "infrastructure/config/JsonSettingsRepository.h"
#include "infrastructure/image/LocalImageStorage.h"
#include "infrastructure/persistence/SqliteConnection.h"
#include "infrastructure/persistence/SqliteHistoryRepository.h"
#include "infrastructure/persistence/SqlitePinnedItemRepository.h"
#include "platform/windows/capture/QtScreenPixelSampler.h"
#include "presentation/capture_actions/CaptureActionBar.h"
#include "presentation/capture_overlay/CaptureOverlay.h"
#include "presentation/pin_window/PinWindow.h"
#include "presentation/viewmodels/CaptureViewModel.h"
#include "shared/events/EventHub.h"

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
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

class FakeScreenPixelSamplerForOverlay final : public IScreenPixelSampler {
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

class SnapPasteTests final : public QObject {
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

    void jsonSettingsRepositoryReportsWriteFailure()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());

        const auto blockerPath = temporaryDir.filePath("not_a_directory");
        QFile blocker(blockerPath);
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.write("block");
        blocker.close();
        qputenv("SNAPPASTE_DATA_DIR", blockerPath.toLocal8Bit());

        JsonSettingsRepository repository;
        AppSettings settings;
        settings.saveDirectory = temporaryDir.path();
        settings.imageFormat = "png";
        settings.themeMode = ThemeMode::System;

        const auto result = repository.save(settings);

        QVERIFY(result.isError());
        qunsetenv("SNAPPASTE_DATA_DIR");
    }

    void annotationDocumentSerializesAnnotations()
    {
        AnnotationDocument document;
        Annotation annotation;
        annotation.tool = AnnotationTool::Rectangle;
        annotation.bounds = QRect(1, 2, 30, 40);
        annotation.text = "note";

        document.add(annotation);
        const auto result = document.toJson();

        QVERIFY(result.isOk());
        QCOMPARE(result.value().size(), 1);
        QCOMPARE(result.value().at(0).toObject().value("width").toInt(), 30);
    }

    void sqliteHistoryRepositoryAddsListsAndDeletesRecords()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());

        SqliteConnection conn(temporaryDir.filePath("history.sqlite"));
        SqliteHistoryRepository repository(conn);

        CaptureRecord record;
        record.filePath = temporaryDir.filePath("capture.png");
        record.thumbnailPath = temporaryDir.filePath("thumb.jpg");
        record.width = 100;
        record.height = 80;
        record.format = "png";
        record.createdAt = QDateTime::currentDateTimeUtc();
        record.sourceScreen = "primary";

        const auto added = repository.add(record);
        QVERIFY(added.isOk());
        QVERIFY(added.value().id > 0);

        const auto recent = repository.recent(10);
        QVERIFY(recent.isOk());
        QCOMPARE(recent.value().size(), 1);

        const auto deleted = repository.markDeleted(added.value().id);
        QVERIFY(deleted.isOk());

        const auto afterDelete = repository.recent(10);
        QVERIFY(afterDelete.isOk());
        QCOMPARE(afterDelete.value().size(), 0);
    }

    void pinnedImageServiceCreatesFromImage()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());

        SqliteConnection conn(temporaryDir.filePath("history.sqlite"));
        FakeClipboardImageProvider clipboardProvider;
        SqlitePinnedItemRepository repository(conn);
        PinnedImageService service(clipboardProvider, repository);

        QImage image(80, 60, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::red);

        const auto result = service.createFromImage(image, PinSource::Screenshot);

        QVERIFY(result.isOk());
        QVERIFY(result.value().id > 0);
        QCOMPARE(result.value().state.transform.scale, 1.0);
        QCOMPARE(result.value().state.opacity, 1.0);
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

    void pinnedRepositoryRestoresAndClosesItems()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());

        SqliteConnection conn(temporaryDir.filePath("history.sqlite"));
        SqlitePinnedItemRepository repository(conn);

        PinnedItem item;
        item.image = QImage(32, 32, QImage::Format_ARGB32_Premultiplied);
        item.image.fill(Qt::blue);
        item.source = PinSource::Clipboard;
        item.state.size = item.image.size();
        item.state.opacity = 1.0;
        item.state.transform.scale = 1.0;
        item.state.options.alwaysOnTop = false;
        item.state.options.visible = true;
        item.createdAt = QDateTime::currentDateTimeUtc();
        item.updatedAt = item.createdAt;

        const auto added = repository.add(item);
        QVERIFY(added.isOk());

        const auto restored = repository.restoreActive();
        QVERIFY(restored.isOk());
        QCOMPARE(restored.value().size(), 1);
        QVERIFY(!restored.value().first().state.options.alwaysOnTop);

        const auto closed = repository.close(added.value().id);
        QVERIFY(closed.isOk());

        const auto afterClose = repository.restoreActive();
        QVERIFY(afterClose.isOk());
        QCOMPARE(afterClose.value().size(), 0);
    }

    void pinnedServicePersistsBulkVisibility()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());

        SqliteConnection conn(temporaryDir.filePath("history.sqlite"));
        FakeClipboardImageProvider clipboardProvider;
        SqlitePinnedItemRepository repository(conn);
        PinnedImageService service(clipboardProvider, repository);

        QImage image(32, 32, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::blue);

        const auto added = service.createFromImage(image, PinSource::Screenshot);
        QVERIFY(added.isOk());

        auto state = added.value().state;
        state.options.clickThrough = true;
        QVERIFY(service.updateState(added.value().id, state).isOk());

        QVERIFY(service.setAllVisible(false).isOk());
        auto hidden = service.restorePinnedItems();
        QVERIFY(hidden.isOk());
        QCOMPARE(hidden.value().size(), 1);
        QVERIFY(!hidden.value().first().state.options.visible);
        QVERIFY(hidden.value().first().state.options.clickThrough);

        QVERIFY(service.setAllVisible(true).isOk());
        auto visible = service.restorePinnedItems();
        QVERIFY(visible.isOk());
        QCOMPARE(visible.value().size(), 1);
        QVERIFY(visible.value().first().state.options.visible);
        QVERIFY(visible.value().first().state.options.clickThrough);
    }

    void pinWindowConstructionDoesNotShowWindow()
    {
        PinnedItem item;
        item.id = 1;
        item.image = QImage(80, 60, QImage::Format_RGB32);
        item.image.fill(Qt::black);
        item.state.size = item.image.size();
        item.state.opacity = 1.0;
        item.state.transform.scale = 1.0;
        item.state.options.visible = true;

        PinWindow window(item);

        QVERIFY(!window.isVisible());
    }

    void pinWindowSupportsCopyAndSaveShortcuts()
    {
        PinnedItem item;
        item.id = 1;
        item.image = QImage(80, 60, QImage::Format_RGB32);
        item.image.fill(Qt::black);
        item.state.size = item.image.size();
        item.state.opacity = 1.0;
        item.state.transform.scale = 1.0;
        item.state.options.visible = true;

        PinWindow window(item);
        QSignalSpy copySpy(&window, &PinWindow::copyRequested);
        QSignalSpy saveSpy(&window, &PinWindow::saveRequested);

        QKeyEvent copyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
        QApplication::sendEvent(&window, &copyEvent);
        QKeyEvent saveEvent(QEvent::KeyPress, Qt::Key_S, Qt::ControlModifier);
        QApplication::sendEvent(&window, &saveEvent);

        QCOMPARE(copySpy.size(), 1);
        QCOMPARE(saveSpy.size(), 1);
        QVERIFY(!copySpy.at(0).at(0).value<QImage>().isNull());
        QVERIFY(!saveSpy.at(0).at(0).value<QImage>().isNull());
    }

    void pinWindowTogglesAlwaysOnTopShortcut()
    {
        PinnedItem item;
        item.id = 1;
        item.image = QImage(80, 60, QImage::Format_RGB32);
        item.image.fill(Qt::black);
        item.state.size = item.image.size();
        item.state.opacity = 1.0;
        item.state.transform.scale = 1.0;
        item.state.options.alwaysOnTop = true;
        item.state.options.visible = true;

        PinWindow window(item);
        QSignalSpy stateSpy(&window, &PinWindow::stateChanged);
        QVERIFY(window.windowFlags().testFlag(Qt::WindowStaysOnTopHint));

        QKeyEvent toggleEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
        QApplication::sendEvent(&window, &toggleEvent);

        QCOMPARE(stateSpy.size(), 1);
        QVERIFY(!window.windowFlags().testFlag(Qt::WindowStaysOnTopHint));
        QVERIFY(!window.state().options.alwaysOnTop);
    }

    void pinWindowCloseShortcutSurvivesImmediateDeletion()
    {
        PinnedItem item;
        item.id = 1;
        item.image = QImage(80, 60, QImage::Format_RGB32);
        item.image.fill(Qt::black);
        item.state.size = item.image.size();
        item.state.opacity = 1.0;
        item.state.transform.scale = 1.0;
        item.state.options.visible = true;

        auto window = std::make_unique<PinWindow>(item);
        auto* rawWindow = window.get();
        QObject::connect(rawWindow, &PinWindow::closeRequested, rawWindow, [&window](qint64) {
            window.reset();
        });

        QKeyEvent closeEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(rawWindow, &closeEvent);

        QVERIFY(window == nullptr);
    }

    void pinWindowToolbarCloseSurvivesImmediateDeletion()
    {
        PinnedItem item;
        item.id = 1;
        item.image = QImage(200, 80, QImage::Format_RGB32);
        item.image.fill(Qt::black);
        item.state.size = item.image.size();
        item.state.opacity = 1.0;
        item.state.transform.scale = 1.0;
        item.state.options.visible = true;

        auto window = std::make_unique<PinWindow>(item);
        auto* rawWindow = window.get();
        QObject::connect(rawWindow, &PinWindow::closeRequested, rawWindow, [&window](qint64) {
            window.reset();
        });

        QEvent enterEvent(QEvent::Enter);
        QApplication::sendEvent(rawWindow, &enterEvent);
        const QPoint closeButtonPoint(35, 15);
        QMouseEvent closeEvent(QEvent::MouseButtonPress,
                               closeButtonPoint,
                               closeButtonPoint,
                               closeButtonPoint,
                               Qt::LeftButton,
                               Qt::LeftButton,
                               Qt::NoModifier);
        QApplication::sendEvent(rawWindow, &closeEvent);

        QVERIFY(window == nullptr);
    }

    void pinWindowSystemCloseSurvivesImmediateDeletion()
    {
        PinnedItem item;
        item.id = 1;
        item.image = QImage(80, 60, QImage::Format_RGB32);
        item.image.fill(Qt::black);
        item.state.size = item.image.size();
        item.state.opacity = 1.0;
        item.state.transform.scale = 1.0;
        item.state.options.visible = true;

        auto window = std::make_unique<PinWindow>(item);
        auto* rawWindow = window.get();
        QObject::connect(rawWindow, &PinWindow::closeRequested, rawWindow, [&window](qint64) {
            window.reset();
        });

        rawWindow->close();

        QVERIFY(window == nullptr);
    }

    void clipboardPinFailsWithEmptyClipboardProvider()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());

        SqliteConnection conn(temporaryDir.filePath("history.sqlite"));
        FakeClipboardImageProvider clipboardProvider;
        SqlitePinnedItemRepository repository(conn);
        PinnedImageService service(clipboardProvider, repository);

        const auto result = service.createFromClipboard();

        QVERIFY(result.isError());
    }

    void localImageStorageUsesUniqueFileNames()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        qputenv("SNAPPASTE_DATA_DIR", temporaryDir.path().toLocal8Bit());

        LocalImageStorage storage;
        QImage image(8, 8, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::green);

        const auto first = storage.saveCapture(image, temporaryDir.path(), "png");
        const auto second = storage.saveCapture(image, temporaryDir.path(), "png");

        QVERIFY(first.isOk());
        QVERIFY(second.isOk());
        QVERIFY(first.value().filePath != second.value().filePath);
        QVERIFY(QFileInfo::exists(first.value().filePath));
        QVERIFY(QFileInfo::exists(second.value().filePath));
        qunsetenv("SNAPPASTE_DATA_DIR");
    }

    void pixelSamplerReturnsColorAfterRefresh()
    {
        QtScreenPixelSampler sampler;
        sampler.refresh(QRect(0, 0, 4, 4));

        QVERIFY(sampler.sample(QPoint(0, 0)).has_value());
        QVERIFY(!sampler.sample(QPoint(4, 4)).has_value());
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

    void captureOverlayPreventsFullScreenSmartSelection()
    {
        FakeScreenRegionDetector regionDetector;
        FakeScreenPixelSamplerForOverlay pixelSampler;
        CaptureSelectionHistory history;
        CaptureOverlay overlay(regionDetector, pixelSampler, history);
        overlay.setGeometry(QRect(0, 0, 800, 600));

        regionDetector.regions = {QRect(0, 0, 800, 600)};

        const QPoint hoverPoint(120, 120);
        QMouseEvent moveEvent(QEvent::MouseMove,
                              hoverPoint,
                              hoverPoint,
                              hoverPoint,
                              Qt::NoButton,
                              Qt::NoButton,
                              Qt::NoModifier);
        QApplication::sendEvent(&overlay, &moveEvent);

        QSignalSpy copySpy(&overlay, &CaptureOverlay::copyRequested);
        QKeyEvent enterEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(&overlay, &enterEvent);

        QCOMPARE(copySpy.size(), 1);
        const auto region = copySpy.at(0).at(0).toRect();
        QCOMPARE(region, QRect(1, 1, 798, 598));
    }

    void captureOverlayKeyboardResizeAdjustsDirectionalEdges()
    {
        FakeScreenRegionDetector regionDetector;
        FakeScreenPixelSamplerForOverlay pixelSampler;
        CaptureSelectionHistory history;
        CaptureOverlay overlay(regionDetector, pixelSampler, history);
        overlay.setGeometry(QRect(0, 0, 800, 600));

        const QPoint start(100, 100);
        const QPoint end(200, 200);
        QMouseEvent pressEvent(QEvent::MouseButtonPress,
                               start,
                               start,
                               start,
                               Qt::LeftButton,
                               Qt::LeftButton,
                               Qt::NoModifier);
        QApplication::sendEvent(&overlay, &pressEvent);

        QMouseEvent moveEvent(QEvent::MouseMove,
                              end,
                              end,
                              end,
                              Qt::NoButton,
                              Qt::LeftButton,
                              Qt::NoModifier);
        QApplication::sendEvent(&overlay, &moveEvent);

        QMouseEvent releaseEvent(QEvent::MouseButtonRelease,
                                 end,
                                 end,
                                 end,
                                 Qt::LeftButton,
                                 Qt::NoButton,
                                 Qt::NoModifier);
        QApplication::sendEvent(&overlay, &releaseEvent);

        QKeyEvent shiftLeftEvent(QEvent::KeyPress, Qt::Key_Left, Qt::ShiftModifier);
        QApplication::sendEvent(&overlay, &shiftLeftEvent);
        QKeyEvent shiftUpEvent(QEvent::KeyPress, Qt::Key_Up, Qt::ShiftModifier);
        QApplication::sendEvent(&overlay, &shiftUpEvent);

        QSignalSpy copySpy(&overlay, &CaptureOverlay::copyRequested);
        QKeyEvent enterEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(&overlay, &enterEvent);

        QCOMPARE(copySpy.size(), 1);
        const auto region = copySpy.at(0).at(0).toRect();
        QCOMPARE(region, QRect(99, 99, 102, 102));
    }
};

QTEST_MAIN(SnapPasteTests)

#include "SnapPasteTests.moc"
