#pragma once

#include "domain/capture/CaptureWorkflow.h"
#include "shared/events/EventHub.h"

#include <QImage>
#include <QObject>
#include <QThreadPool>

#include <atomic>
#include <functional>

namespace snappaste {

class CaptureViewModel final : public QObject {
    Q_OBJECT

public:
    CaptureViewModel(CaptureWorkflow& workflow, EventHub& eventHub, QObject* parent = nullptr);
    ~CaptureViewModel() override;

    const QImage& currentImage() const noexcept;
    void captureRegionAsync(const QRect& region, std::function<void(const QImage&)> onReady = {});

public slots:
    void setCurrentImage(const QImage& image);
    void captureRegion(const QRect& region);
    void saveImage(const QImage& image, const QString& sourceScreen = "pin");
    void saveCurrentImage();
    void copyCurrentImageToClipboard();

signals:
    void imageReady(const QImage& image);
    void saved(const QString& filePath);
    void copied();
    void errorOccurred(const QString& message);

private:
    CaptureWorkflow& workflow_;
    EventHub& eventHub_;
    QImage currentImage_;
    QString sourceScreen_;
    QThreadPool workerPool_;
    std::atomic_bool shuttingDown_{false};
    std::atomic_int requestGeneration_{0};
};

} // namespace snappaste
