#include "presentation/viewmodels/CaptureViewModel.h"

#include "shared/screen/ScreenSegmentUtil.h"

#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QScreen>

#include <memory>
#include <QVector>

namespace snappaste {

namespace {

class FunctionRunnable final : public QRunnable {
public:
    explicit FunctionRunnable(std::function<void()> function)
        : function_(std::move(function))
    {
        setAutoDelete(true);
    }

    void run() override
    {
        function_();
    }

private:
    std::function<void()> function_;
};

} // namespace

CaptureViewModel::CaptureViewModel(CaptureWorkflow& workflow, EventHub& eventHub, QObject* parent)
    : QObject(parent)
    , workflow_(workflow)
    , eventHub_(eventHub)
    , alive_(std::make_shared<std::atomic<bool>>(true))
{
    workerPool_.setMaxThreadCount(1);
}

CaptureViewModel::~CaptureViewModel()
{
    shuttingDown_.store(true);
    *alive_ = false;
    workerPool_.clear();
    workerPool_.waitForDone();
}

const QImage& CaptureViewModel::currentImage() const noexcept
{
    return currentImage_;
}

void CaptureViewModel::setCurrentImage(const QImage& image)
{
    currentImage_ = image;
}

void CaptureViewModel::captureRegion(const QRect& region)
{
    auto result = workflow_.captureRegion(region);
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    currentImage_ = std::move(result.value());
    const auto screen = QGuiApplication::screenAt(region.center());
    sourceScreen_ = screen != nullptr ? screen->name() : "primary";
    emit imageReady(currentImage_);
}

void CaptureViewModel::captureRegionAsync(const QRect& region, std::function<void(const QImage&)> onReady)
{
    const auto requestId = requestGeneration_.fetch_add(1) + 1;
    const auto segments = captureSegmentsFor(region);
    sourceScreen_ = segments.isEmpty() ? QString("primary") : segments.first().screenName;
    workerPool_.clear();

    QPointer<CaptureViewModel> guard(this);
    auto weakAlive = alive_;
    auto task = [weakAlive, guard, region, segments, requestId, onReady = std::move(onReady), &workflow = workflow_]() mutable {
        if (!*weakAlive) return;
        auto result = workflow.captureRegion(region, segments);
        if (!*weakAlive) return;

        QMetaObject::invokeMethod(qApp, [weakAlive, guard, region, requestId, result = std::move(result), onReady = std::move(onReady)]() mutable {
            if (!*weakAlive || guard.isNull()) return;
            if (guard->requestGeneration_.load() != requestId) return;

            if (result.isError()) {
                emit guard->errorOccurred(result.error());
                return;
            }

            guard->currentImage_ = std::move(result.value());
            emit guard->imageReady(guard->currentImage_);
            if (onReady) {
                onReady(guard->currentImage_);
            }
        }, Qt::QueuedConnection);
    };

    workerPool_.start(std::make_unique<FunctionRunnable>(std::move(task)).release());
}

void CaptureViewModel::saveImage(const QImage& image, const QString& sourceScreen)
{
    const auto result = workflow_.saveCapturedImage(image, sourceScreen);
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    emit saved(result.value().filePath);
    emit eventHub_.historyChanged();
}

void CaptureViewModel::saveCurrentImage()
{
    saveImage(currentImage_, sourceScreen_);
}

void CaptureViewModel::copyCurrentImageToClipboard()
{
    if (currentImage_.isNull()) {
        emit errorOccurred(tr("No image is available to copy."));
        return;
    }

    { const QSignalBlocker blocker(QApplication::clipboard()); QApplication::clipboard()->setImage(currentImage_); }
    emit copied();
}

} // namespace snappaste
