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

    QImage result;
    if (src.format() == QImage::Format_ARGB32_Premultiplied) {
        result = src;
    } else {
        result = src.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    const double minDim = qMin(result.width(), result.height());
    if (minDim < 200) {
        const double factor = qMin(4.0, 200.0 / minDim);
        result = result.scaled(static_cast<int>(result.width() * factor),
                               static_cast<int>(result.height() * factor),
                               Qt::KeepAspectRatio, Qt::FastTransformation);
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

void WindowsOcrService::setLanguage(const QString& bcp47Tag)
{
    QMutexLocker lock(&langMutex_);
    language_ = bcp47Tag;
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
