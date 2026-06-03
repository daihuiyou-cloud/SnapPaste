#pragma once

#include "domain/ocr/IOcrService.h"
#include "infrastructure/logging/ILogger.h"

#include <QMutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

#if __has_include(<winrt/Windows.Media.Ocr.h>)
#include <winrt/Windows.Media.Ocr.h>
#define SNAPPASTE_HAS_WINRT_OCR 1
#endif

namespace snappaste {

class WindowsOcrService final : public IOcrService {
public:
    explicit WindowsOcrService(ILogger& logger);
    ~WindowsOcrService() override;

    OcrResult recognizeText(const QImage& image) override;
    void setLanguage(const QString& bcp47Tag) override;
    void cancel() override;

    bool isCancelled(int requestId) const noexcept
    {
        return cancelled_.load() || requestId != currentRequestId_.load();
    }

private:
    void workerLoop();
    OcrResult recognizeTextImpl(const QImage& source);

    ILogger& logger_;
    QString language_;
    QMutex langMutex_;
    std::atomic<bool> cancelled_{false};
    std::atomic<int> currentRequestId_{0};

    std::thread worker_;
    std::mutex queueMutex_;
    std::condition_variable queueCv_;
    std::queue<std::packaged_task<OcrResult()>> tasks_;
    std::atomic<bool> done_{false};

#if defined(SNAPPASTE_HAS_WINRT_OCR)
    winrt::Windows::Media::Ocr::OcrEngine ocrEngine_{nullptr};
    std::string cachedLanguage_;
#endif
};

} // namespace snappaste
