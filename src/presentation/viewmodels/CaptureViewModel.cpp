#include "presentation/viewmodels/CaptureViewModel.h"

#include "shared/screen/ScreenSegmentUtil.h"

#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QScreen>
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
{
    workerPool_.setMaxThreadCount(1);
}

CaptureViewModel::~CaptureViewModel()
{
    shuttingDown_.store(true);
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

    QPointer<CaptureViewModel> self(this);
    auto task = [self, region, segments, requestId, onReady = std::move(onReady)]() mutable {
        if (self.isNull()) return;
        const auto result = self->workflow_.captureRegion(region, segments);
        if (self.isNull() || self->shuttingDown_.load() || self->requestGeneration_.load() != requestId) {
            return;
        }

        QMetaObject::invokeMethod(self.data(), [self, region, requestId, result, onReady = std::move(onReady)]() mutable {
            if (self.isNull() || self->shuttingDown_.load() || self->requestGeneration_.load() != requestId) {
                return;
            }

            if (result.isError()) {
                emit self->errorOccurred(result.error());
                return;
            }

            self->currentImage_ = result.value();
            const auto screen = QGuiApplication::screenAt(region.center());
            self->sourceScreen_ = screen != nullptr ? screen->name() : "primary";
            emit self->imageReady(self->currentImage_);
            if (onReady) {
                onReady(self->currentImage_);
            }
        }, Qt::QueuedConnection);
    };

    workerPool_.start(new FunctionRunnable(std::move(task)));
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
        emit errorOccurred("No image is available to copy.");
        return;
    }

    QApplication::clipboard()->setImage(currentImage_);
    emit copied();
}

} // namespace snappaste
