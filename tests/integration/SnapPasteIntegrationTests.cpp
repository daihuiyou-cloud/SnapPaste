#include "domain/capture/CaptureSelectionHistory.h"
#include "domain/capture/CaptureWorkflow.h"
#include "domain/pin/PinnedImageService.h"

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
#include "tests/test_helpers.h"

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QtTest/QtTest>

#include <memory>

using namespace snappaste;

// ---------------------------------------------------------------------------
// Fake implementations for testing
// ---------------------------------------------------------------------------

class FakeAppPaths final : public IAppPaths {
public:
    QString dataDirectory() override { return {}; }
    QString configFilePath() override { return {}; }
    QString databaseFilePath() override { return {}; }
    QString defaultCaptureDirectory() override { return {}; }
    QString thumbnailDirectory() override { return {}; }
    bool ensureDirectory(const QString& path) override { return true; }
};

class FakeIconProvider final : public IIconProvider {
public:
    QIcon icon(IconName name) override { return {}; }
};

class FakeTimeProvider final : public ITimeProvider {
public:
    QDateTime nowUtc() override { return QDateTime::currentDateTimeUtc(); }
};

class SnapPasteIntegrationTests final : public QObject {
    Q_OBJECT

private slots:
    void jsonSettingsRepositoryReportsWriteFailure()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());

        const auto blockerPath = temporaryDir.filePath("not_a_directory");
        QFile blocker(blockerPath);
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.write("block");
        blocker.close();
        ScopedEnvVar env("SNAPPASTE_DATA_DIR", blockerPath);

        FakeAppPaths appPaths;
        JsonSettingsRepository repository(appPaths);
        AppSettings settings;
        settings.saveDirectory = temporaryDir.path();
        settings.imageFormat = "png";
        settings.themeMode = ThemeMode::System;

        const auto result = repository.save(settings);

        QVERIFY(result.isError());
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
        FakeTimeProvider timeProvider;
        PinnedImageService service(clipboardProvider, repository, timeProvider);

        QImage image(80, 60, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::red);

        const auto result = service.createFromImage(image, PinSource::Screenshot);

        QVERIFY(result.isOk());
        QVERIFY(result.value().id > 0);
        QCOMPARE(result.value().state.transform.scale, 1.0);
        QCOMPARE(result.value().state.opacity, 1.0);
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
        FakeTimeProvider timeProvider;
        PinnedImageService service(clipboardProvider, repository, timeProvider);

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

        FakeIconProvider iconProvider;
        PinWindow window(item, iconProvider);

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

        FakeIconProvider iconProvider;
        PinWindow window(item, iconProvider);
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

        FakeIconProvider iconProvider;
        PinWindow window(item, iconProvider);
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

        FakeIconProvider iconProvider;
        auto window = std::make_unique<PinWindow>(item, iconProvider);
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

        FakeIconProvider iconProvider;
        auto window = std::make_unique<PinWindow>(item, iconProvider);
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

        FakeIconProvider iconProvider;
        auto window = std::make_unique<PinWindow>(item, iconProvider);
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
        FakeTimeProvider timeProvider;
        PinnedImageService service(clipboardProvider, repository, timeProvider);

        const auto result = service.createFromClipboard();

        QVERIFY(result.isError());
    }

    void localImageStorageUsesUniqueFileNames()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        ScopedEnvVar env("SNAPPASTE_DATA_DIR", temporaryDir.path());

        FakeAppPaths appPaths;
        LocalImageStorage storage(appPaths);
        QImage image(8, 8, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::green);

        const auto first = storage.saveCapture(image, temporaryDir.path(), "png");
        const auto second = storage.saveCapture(image, temporaryDir.path(), "png");

        QVERIFY(first.isOk());
        QVERIFY(second.isOk());
        QVERIFY(first.value().filePath != second.value().filePath);
        QVERIFY(QFileInfo::exists(first.value().filePath));
        QVERIFY(QFileInfo::exists(second.value().filePath));
    }

    void pixelSamplerReturnsColorAfterRefresh()
    {
        QtScreenPixelSampler sampler;
        sampler.refresh(QRect(0, 0, 4, 4));

        QVERIFY(sampler.sample(QPoint(0, 0)).has_value());
        QVERIFY(!sampler.sample(QPoint(4, 4)).has_value());
    }

    void captureOverlayPreventsFullScreenSmartSelection()
    {
        FakeScreenRegionDetector regionDetector;
        FakeScreenPixelSampler pixelSampler;
        CaptureSelectionHistory history;
        FakeIconProvider iconProvider;
        CaptureOverlay overlay(iconProvider, regionDetector, pixelSampler, history);
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
        FakeScreenPixelSampler pixelSampler;
        CaptureSelectionHistory history;
        FakeIconProvider iconProvider;
        CaptureOverlay overlay(iconProvider, regionDetector, pixelSampler, history);
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

QTEST_MAIN(SnapPasteIntegrationTests)

#include "SnapPasteIntegrationTests.moc"
