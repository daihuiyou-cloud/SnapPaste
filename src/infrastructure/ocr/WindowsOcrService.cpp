#include "infrastructure/ocr/WindowsOcrService.h"

#include <climits>
#include <cstring>

#include <QCoreApplication>
#include <QMetaObject>

#if defined(SNAPPASTE_HAS_WINRT_OCR)
#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/base.h>
#endif

namespace snappaste {

namespace {

QImage preprocessForOcr(const QImage& src)
{
    if (src.isNull()) return {};

    // Step 1: Convert to grayscale for better text/background separation
    QImage gray = src.convertToFormat(QImage::Format_Grayscale8);

    // Step 2: Auto-contrast (histogram stretch) to enhance faint/low-contrast text
    int minVal = 255, maxVal = 0;
    for (int y = 0; y < gray.height(); ++y) {
        const auto* line = gray.constScanLine(y);
        for (int x = 0; x < gray.width(); ++x) {
            const auto v = line[x];
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
        }
    }
    if (maxVal > minVal) {
        const double scale = 255.0 / (maxVal - minVal);
        for (int y = 0; y < gray.height(); ++y) {
            auto* line = gray.scanLine(y);
            for (int x = 0; x < gray.width(); ++x) {
                line[x] = static_cast<uchar>((line[x] - minVal) * scale + 0.5);
            }
        }
    }

    // Step 3: Mild unsharp mask sharpening to enhance text edges (anti-aliased fonts)
    if (gray.width() > 6 && gray.height() > 6) {
        QImage blurred(gray.size(), QImage::Format_Grayscale8);
        for (int y = 0; y < gray.height(); ++y) {
            for (int x = 0; x < gray.width(); ++x) {
                int sum = 0;
                int count = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    const auto* srcLine = gray.constScanLine(qBound(0, y + dy, gray.height() - 1));
                    for (int dx = -1; dx <= 1; ++dx) {
                        sum += srcLine[qBound(0, x + dx, gray.width() - 1)];
                        ++count;
                    }
                }
                blurred.scanLine(y)[x] = static_cast<uchar>(sum / count);
            }
        }
        for (int y = 0; y < gray.height(); ++y) {
            const auto* origLine = gray.constScanLine(y);
            const auto* blurLine = blurred.constScanLine(y);
            auto* outLine = gray.scanLine(y);
            for (int x = 0; x < gray.width(); ++x) {
                const int val = origLine[x] + (origLine[x] - blurLine[x]) / 2;
                outLine[x] = static_cast<uchar>(qBound(0, val, 255));
            }
        }
    }

    // Step 4: Convert to premultiplied ARGB32 for SoftwareBitmap copy
    QImage result = gray.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    // Step 4: Upscale very small images with smooth interpolation
    const double minDim = qMin(result.width(), result.height());
    if (minDim < 200) {
        const double factor = qMin(4.0, 200.0 / minDim);
        result = result.scaled(static_cast<int>(result.width() * factor),
                               static_cast<int>(result.height() * factor),
                               Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return result;
}

#if defined(SNAPPASTE_HAS_WINRT_OCR)
struct __declspec(uuid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")) IMemoryBufferByteAccess : ::IUnknown {
    virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
};
#endif

} // namespace

WindowsOcrService::WindowsOcrService(ILogger& logger)
    : logger_(logger)
{
    worker_ = std::thread([this] { workerLoop(); });
}

WindowsOcrService::~WindowsOcrService()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        done_ = true;
    }
    queueCv_.notify_one();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void WindowsOcrService::workerLoop()
{
#if defined(SNAPPASTE_HAS_WINRT_OCR)
    winrt::init_apartment(winrt::apartment_type::single_threaded);
#endif
    while (true) {
        std::packaged_task<OcrResult()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCv_.wait(lock, [this] { return done_ || !tasks_.empty(); });
            if (done_ && tasks_.empty()) break;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
#if defined(SNAPPASTE_HAS_WINRT_OCR)
    ocrEngine_ = nullptr;
    winrt::uninit_apartment();
#endif
}

void WindowsOcrService::cancel()
{
    ++currentRequestId_;
}

void WindowsOcrService::setLanguage(QString bcp47Tag)
{
    QMutexLocker lock(&langMutex_);
    language_ = std::move(bcp47Tag);
}

OcrResult WindowsOcrService::recognizeText(const QImage& source)
{
    const auto requestId = currentRequestId_.load();
    std::packaged_task<OcrResult()> task([this, source, requestId] {
        return recognizeTextImpl(source, requestId);
    });
    auto future = task.get_future();
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        tasks_.push(std::move(task));
    }
    queueCv_.notify_one();
    return future.get();
}

void WindowsOcrService::recognizeTextAsync(const QImage& image, std::function<void(OcrResult)> callback)
{
    const auto requestId = currentRequestId_.load();
    QImage copy = image;
    std::packaged_task<OcrResult()> task([this, copy, requestId, cb = std::move(callback)] {
        auto result = recognizeTextImpl(copy, requestId);
        QMetaObject::invokeMethod(QCoreApplication::instance(), [result, cb = std::move(cb)] {
            cb(result);
        }, Qt::QueuedConnection);
        return result;
    });
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        tasks_.push(std::move(task));
    }
    queueCv_.notify_one();
}

OcrResult WindowsOcrService::recognizeTextImpl(const QImage& source, int requestId)
{
#if defined(SNAPPASTE_HAS_WINRT_OCR)
    if (source.isNull()) {
        return {false, {}, QObject::tr("No image is available for OCR."), {}, {}};
    }

    try {
        if (isCancelled(requestId)) return {false, {}, QObject::tr("OCR cancelled."), {}, {}};

        QString lang;
        {
            QMutexLocker lock(&langMutex_);
            lang = language_;
        }

        {
            const std::string langStr = lang.toStdString();
            if (!ocrEngine_ || cachedLanguage_ != langStr) {
                if (langStr.empty()) {
                    ocrEngine_ = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromUserProfileLanguages();
                } else {
                    ocrEngine_ = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(
                        winrt::Windows::Globalization::Language(winrt::hstring(lang.toStdWString())));
                }
                cachedLanguage_ = langStr;
            }
        }

        if (ocrEngine_ == nullptr) {
            return {false, {}, QObject::tr("OCR is not available for the current Windows language profile."), {}, {}};
        }

        if (isCancelled(requestId)) return {false, {}, QObject::tr("OCR cancelled."), {}, {}};

        const auto processed = preprocessForOcr(source);
        const double scaleX = static_cast<double>(source.width()) / processed.width();
        const double scaleY = static_cast<double>(source.height()) / processed.height();

        winrt::Windows::Graphics::Imaging::SoftwareBitmap bitmap(
            winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,
            processed.width(),
            processed.height(),
            winrt::Windows::Graphics::Imaging::BitmapAlphaMode::Premultiplied);

        {
            const auto buffer = bitmap.LockBuffer(winrt::Windows::Graphics::Imaging::BitmapBufferAccessMode::Write);
            const auto reference = buffer.CreateReference();
            auto byteAccess = reference.as<IMemoryBufferByteAccess>();
            uint8_t* bytes = nullptr;
            uint32_t capacity = 0;
            winrt::check_hresult(byteAccess->GetBuffer(&bytes, &capacity));

            const auto plane = buffer.GetPlaneDescription(0);
            const auto srcBytesPerLine = processed.bytesPerLine();
            const auto dstBytesPerLine = static_cast<int>(plane.Stride);
            if (srcBytesPerLine == dstBytesPerLine) {
                const auto totalBytes = processed.height() * srcBytesPerLine;
                if (totalBytes <= static_cast<int>(capacity)) {
                    std::memcpy(bytes, processed.constBits(), totalBytes);
                }
            } else {
                const auto copyBytes = std::min(srcBytesPerLine, dstBytesPerLine);
                for (int y = 0; y < processed.height(); ++y) {
                    const auto targetOffset = plane.StartIndex + (y * plane.Stride);
                    if (targetOffset + copyBytes > static_cast<int>(capacity)) break;
                    std::memcpy(bytes + targetOffset, processed.constScanLine(y), copyBytes);
                }
            }
        }

        if (isCancelled(requestId)) return {false, {}, QObject::tr("OCR cancelled."), {}, {}};
        const auto result = ocrEngine_.RecognizeAsync(bitmap).get();
        if (isCancelled(requestId)) return {false, {}, QObject::tr("OCR cancelled."), {}, {}};

        QStringList lines;
        QVector<OcrBlockInfo> blocks;
        for (const auto& line : result.Lines()) {
            const auto t = QString::fromWCharArray(line.Text().c_str());
            lines << t;
            int minX = INT_MAX, minY = INT_MAX, maxX = 0, maxY = 0;
            bool hasWord = false;
            for (const auto& word : line.Words()) {
                const auto r = word.BoundingRect();
                minX = (std::min)(minX, static_cast<int>(r.X));
                minY = (std::min)(minY, static_cast<int>(r.Y));
                maxX = (std::max)(maxX, static_cast<int>(r.X + r.Width));
                maxY = (std::max)(maxY, static_cast<int>(r.Y + r.Height));
                hasWord = true;
            }
            if (hasWord) {
                QRect scaledRect(
                    static_cast<int>(minX * scaleX),
                    static_cast<int>(minY * scaleY),
                    static_cast<int>((maxX - minX) * scaleX),
                    static_cast<int>((maxY - minY) * scaleY));
                blocks.push_back(OcrBlockInfo{t, scaledRect});
            }
        }
        const auto text = lines.join('\n').trimmed();
        if (text.isEmpty()) {
            return {false, {}, QObject::tr("No text was recognized in the selected region."), {}, {}};
        }
        return {true, text, {}, source, blocks};
    } catch (const winrt::hresult_error& e) {
        auto msg = QString::fromWCharArray(e.message().c_str());
        logger_.warning("OCR failed: " + msg);
        return {false, {}, QObject::tr("OCR failed while processing the selected region."), {}, {}};
    } catch (const std::exception& e) {
        logger_.warning("OCR failed: " + QString::fromLatin1(e.what()));
        return {false, {}, QObject::tr("OCR failed while processing the selected region."), {}, {}};
    }
#else
    Q_UNUSED(source)
    Q_UNUSED(requestId)
    return {false, {}, QObject::tr("OCR is not available in this build."), {}, {}};
#endif
}

} // namespace snappaste
