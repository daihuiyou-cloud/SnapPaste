#include "presentation/viewmodels/CaptureViewModel.h"

#include "shared/screen/ScreenSegmentUtil.h"

#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QMetaObject>
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
    const auto result = workflow_.captureRegion(region);
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    currentImage_ = result.value();
    const auto screen = QGuiApplication::screenAt(region.center());
    sourceScreen_ = screen != nullptr ? screen->name() : "primary";
    emit imageReady(currentImage_);
}

void CaptureViewModel::captureRegionAsync(const QRect& region, std::function<void(const QImage&)> onReady)
{
    const auto requestId = requestGeneration_.fetch_add(1) + 1;
    const auto segments = captureSegmentsFor(region);
    workerPool_.clear();

    auto weakAlive = alive_;
    auto task = [weakAlive, region, segments, requestId, onReady = std::move(onReady), this]() mutable {
        if (!*weakAlive) return;
        const auto result = this->workflow_.captureRegion(region, segments);
        if (!*weakAlive || this->shuttingDown_.load() || this->requestGeneration_.load() != requestId) {
            return;
        }

        QMetaObject::invokeMethod(this, [weakAlive, region, requestId, result, onReady = std::move(onReady), this]() mutable {
            if (!*weakAlive || this->shuttingDown_.load() || this->requestGeneration_.load() != requestId) {
                return;
            }

            if (result.isError()) {
                emit this->errorOccurred(result.error());
                return;
            }

            this->currentImage_ = result.value();
            const auto screen = QGuiApplication::screenAt(region.center());
            this->sourceScreen_ = screen != nullptr ? screen->name() : "primary";
            emit this->imageReady(this->currentImage_);
            if (onReady) {
                onReady(this->currentImage_);
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
