#include "domain/capture/CaptureSelectionHistory.h"
#include "domain/capture/CaptureWorkflow.h"
#include "domain/pin/PinnedItem.h"
#include "presentation/viewmodels/SettingsViewModel.h"
#include "presentation/capture_actions/CaptureActionBar.h"
#include "presentation/viewmodels/CaptureViewModel.h"
#include "shared/events/EventHub.h"
#include "tests/test_helpers.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

#include <memory>

using namespace snappaste;

// ---------------------------------------------------------------------------
// Fake implementations for testing
// ---------------------------------------------------------------------------

class FakeIconProvider final : public IIconProvider {
public:
    QIcon icon(IconName name) override { return {}; }
};

class FakeTimeProvider final : public ITimeProvider {
public:
    QDateTime nowUtc() override { return QDateTime::currentDateTimeUtc(); }
};

class SnapPasteUnitTests final : public QObject {
    Q_OBJECT

private slots:
    void settingsServiceRejectsEmptyDirectory()
    {
        FakeSettingsRepository repository;
        EventHub eventHub;
        SettingsViewModel viewModel(repository, eventHub);

        AppSettings settings;
        settings.saveDirectory = "";
        settings.imageFormat = "png";

        viewModel.save(settings);

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
        QCOMPARE(history.previous(), QRect(0, 0, 10, 10));
        QCOMPARE(history.next(), QRect(20, 20, 10, 10));
        QCOMPARE(history.next(), QRect(20, 20, 10, 10));
        QCOMPARE(history.previous(), QRect(0, 0, 10, 10));
        QCOMPARE(history.next(), QRect(20, 20, 10, 10));
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
        FakeIconProvider iconProvider;
        CaptureActionBar actionBar(iconProvider);
        QSignalSpy copySpy(&actionBar, &CaptureActionBar::copyRequested);
        QSignalSpy pinSpy(&actionBar, &CaptureActionBar::pinRequested);
        QSignalSpy saveSpy(&actionBar, &CaptureActionBar::saveRequested);
        QSignalSpy editSpy(&actionBar, &CaptureActionBar::editRequested);
        QSignalSpy cancelSpy(&actionBar, &CaptureActionBar::cancelRequested);

        QKeyEvent enterEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(&actionBar, &enterEvent);
        QKeyEvent pinEvent(QEvent::KeyPress, Qt::Key_F3, Qt::NoModifier);
        QApplication::sendEvent(&actionBar, &pinEvent);
        QKeyEvent saveEvent(QEvent::KeyPress, Qt::Key_S, Qt::ControlModifier);
        QApplication::sendEvent(&actionBar, &saveEvent);
        QKeyEvent editEvent(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
        QApplication::sendEvent(&actionBar, &editEvent);
        QKeyEvent cancelEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(&actionBar, &cancelEvent);

        QCOMPARE(copySpy.size(), 1);
        QCOMPARE(pinSpy.size(), 1);
        QCOMPARE(saveSpy.size(), 1);
        QCOMPARE(editSpy.size(), 1);
        QCOMPARE(cancelSpy.size(), 1);
    }

    void captureWorkflowUsesResolvedScreenSegments()
    {
        FakeScreenCaptureService captureService;
        FakeImageStorage imageStorage;
        FakeHistoryRepository historyRepository;
        FakeSettingsRepository settingsRepository;
        FakeTimeProvider timeProvider;
        CaptureWorkflow workflow(captureService, imageStorage, historyRepository, settingsRepository, timeProvider);

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
        FakeTimeProvider timeProvider;
        CaptureWorkflow workflow(captureService, imageStorage, historyRepository, settingsRepository, timeProvider);
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
