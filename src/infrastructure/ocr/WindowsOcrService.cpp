#include "infrastructure/ocr/WindowsOcrService.h"

#include <atomic>
#include <climits>
#include <cstring>

#if defined(_WIN32) && __has_include(<winrt/Windows.Media.Ocr.h>)
#define SNAPPASTE_HAS_WINRT_OCR 1
#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/base.h>
#endif

namespace snappaste {

namespace {

// Compute Otsu threshold from grayscale histogram
int otsuThreshold(const std::vector<int>& hist, int total)
{
    double sum = 0;
    for (int i = 0; i < 256; ++i) sum += i * hist[i];

    double sumB = 0;
    int wB = 0;
    double maxVariance = 0;
    int threshold = 0;

    for (int i = 0; i < 256; ++i) {
        wB += hist[i];
        if (wB == 0) continue;
        int wF = total - wB;
        if (wF == 0) break;
        sumB += i * hist[i];
        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;
        double variance = static_cast<double>(wB) * wF * (mB - mF) * (mB - mF);
        if (variance > maxVariance) {
            maxVariance = variance;
            threshold = i;
        }
    }
    return threshold;
}

// Apply 3x3 sharpen kernel (unsharp mask)
void sharpenImage(QImage& img)
{
    if (img.format() != QImage::Format_Grayscale8) return;
    QImage src = img.copy();
    for (int y = 1; y < img.height() - 1; ++y) {
        const auto* s = src.constScanLine(y);
        auto* d = img.scanLine(y);
        for (int x = 1; x < img.width() - 1; ++x) {
            int v = 5 * s[x]
                  - s[x - 1] - s[x + 1]
                  - src.constScanLine(y - 1)[x]
                  - src.constScanLine(y + 1)[x];
            d[x] = static_cast<uint8_t>(qBound(0, v, 255));
        }
    }
}

    // Pre-process image for better OCR accuracy:
    //  - Grayscale + sharpen to define text edges
    //  - Otsu binarization for clean text/background separation
    //  - Aggressive upscale to ensure readable text size
    QImage preprocessForOcr(const QImage& src)
    {
        if (src.isNull()) return {};

        // Step 1: grayscale
        QImage gray = src.convertToFormat(QImage::Format_Grayscale8);

        // Step 2: sharpen to make text edges crisper
        sharpenImage(gray);

        // Step 3: Otsu binarization
        std::vector<int> hist(256, 0);
        for (int y = 0; y < gray.height(); ++y) {
            const auto* line = gray.constScanLine(y);
            for (int x = 0; x < gray.width(); ++x) {
                hist[line[x]]++;
            }
        }

        int total = gray.width() * gray.height();
        int threshold = otsuThreshold(hist, total);

        // Determine if text is light-on-dark or dark-on-light
        // Count pixels below threshold (potential text if dark-on-light)
        int darkPixels = 0;
        for (int i = 0; i < threshold; ++i) darkPixels += hist[i];
        bool inverted = (darkPixels > total / 2);

        for (int y = 0; y < gray.height(); ++y) {
            auto* line = gray.scanLine(y);
            for (int x = 0; x < gray.width(); ++x) {
                line[x] = (line[x] > threshold) == inverted ? 0 : 255;
            }
        }

        // Step 4: aggressive upscale - target min dimension 200px
        double minDim = qMin(gray.width(), gray.height());
        QImage result = gray;
        if (minDim < 200) {
            double factor = qMin(4.0, 200.0 / minDim);
            result = gray.scaled(static_cast<int>(gray.width() * factor),
                                 static_cast<int>(gray.height() * factor),
                                 Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        // Convert back to ARGB32_Premultiplied for WinRT bitmap
        return result.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

#if defined(SNAPPASTE_HAS_WINRT_OCR)
struct __declspec(uuid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")) IMemoryBufferByteAccess : ::IUnknown {
    virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
};

winrt::Windows::Globalization::Language createLanguageFromTag(const std::wstring& tag)
{
    using namespace winrt::Windows::Globalization;
    auto factory = winrt::get_activation_factory<Language, ILanguageFactory>();
    auto abiPtr = reinterpret_cast<winrt::impl::abi<ILanguageFactory>::type*>(
        winrt::get_abi(factory));
    Language lang{ nullptr };
    winrt::hstring hstr(tag);
    winrt::check_hresult(abiPtr->CreateLanguage(
        winrt::get_abi(hstr), winrt::put_abi(lang)));
    return lang;
}
#endif

} // namespace

WindowsOcrService::WindowsOcrService(ILogger& logger)
    : logger_(logger)
{
}

WindowsOcrService::~WindowsOcrService()
{
}

void WindowsOcrService::cancel()
{
    cancelled_ = true;
    ++currentRequestId_;
}

void WindowsOcrService::setLanguage(const QString& bcp47Tag)
{
    QMutexLocker lock(&mutex_);
    language_ = bcp47Tag;
}

OcrResult WindowsOcrService::recognizeText(const QImage& source)
{
    cancelled_ = false;
    const auto requestId = ++currentRequestId_;
#if defined(SNAPPASTE_HAS_WINRT_OCR)
    struct ComGuard {
        ComGuard() { winrt::init_apartment(winrt::apartment_type::single_threaded); }
        ~ComGuard() { winrt::uninit_apartment(); }
    } comGuard;
    Q_UNUSED(comGuard)

    if (source.isNull()) {
        return {false, {}, QObject::tr("No image is available for OCR."), {}, {}};
    }

    try {
        if (isCancelled(requestId)) return {false, {}, QObject::tr("OCR cancelled."), {}, {}};

        QString lang;
        {
            QMutexLocker lock(&mutex_);
            lang = language_;
        }

        winrt::Windows::Media::Ocr::OcrEngine engine = nullptr;
        if (lang.isEmpty()) {
            engine = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromUserProfileLanguages();
        } else {
            auto langObj = createLanguageFromTag(lang.toStdWString());
            engine = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(langObj);
        }
        if (engine == nullptr) {
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
            const auto copyBytes = std::min(srcBytesPerLine, dstBytesPerLine);
            for (int y = 0; y < processed.height(); ++y) {
                const auto targetOffset = plane.StartIndex + (y * plane.Stride);
                if (targetOffset + copyBytes > static_cast<int>(capacity)) {
                    break;
                }
                std::memcpy(bytes + targetOffset, processed.constScanLine(y), copyBytes);
            }
        }

        if (isCancelled(requestId)) return {false, {}, QObject::tr("OCR cancelled."), {}, {}};
        const auto result = engine.RecognizeAsync(bitmap).get();
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
    return {false, {}, QObject::tr("OCR is not available in this build."), {}, {}};
#endif
}

} // namespace snappaste
