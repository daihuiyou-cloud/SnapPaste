#include "infrastructure/ocr/WindowsOcrService.h"

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
#if defined(_MSC_VER)
#pragma comment(lib, "windowsapp")
#endif
#endif

namespace snappaste {

namespace {

#if defined(SNAPPASTE_HAS_WINRT_OCR)
struct __declspec(uuid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")) IMemoryBufferByteAccess : ::IUnknown {
    virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
};

// Workaround: C++/WinRT Windows.Globalization Language constructor
// is declared but not inline in SDK 10.0.19041 cppwinrt headers.
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

WindowsOcrService::WindowsOcrService()
{
#if defined(SNAPPASTE_HAS_WINRT_OCR)
    try {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
    } catch (...) {
    }
#endif
}

WindowsOcrService::~WindowsOcrService()
{
#if defined(SNAPPASTE_HAS_WINRT_OCR)
    winrt::uninit_apartment();
#endif
}

void WindowsOcrService::setLanguage(const QString& bcp47Tag)
{
    language_ = bcp47Tag;
}

OcrResult WindowsOcrService::recognizeText(const QImage& source)
{
#if defined(SNAPPASTE_HAS_WINRT_OCR)
    if (source.isNull()) {
        return {false, {}, "No image is available for OCR.", {}, {}};
    }

    try {
        winrt::Windows::Media::Ocr::OcrEngine engine = nullptr;
        if (language_.isEmpty()) {
            engine = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromUserProfileLanguages();
        } else {
            auto lang = createLanguageFromTag(language_.toStdWString());
            engine = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(lang);
        }
        if (engine == nullptr) {
            return {false, {}, "OCR is not available for the current Windows language profile.", {}, {}};
        }

        const auto image = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        winrt::Windows::Graphics::Imaging::SoftwareBitmap bitmap(
            winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,
            image.width(),
            image.height(),
            winrt::Windows::Graphics::Imaging::BitmapAlphaMode::Premultiplied);

        {
            const auto buffer = bitmap.LockBuffer(winrt::Windows::Graphics::Imaging::BitmapBufferAccessMode::Write);
            const auto reference = buffer.CreateReference();
            auto byteAccess = reference.as<IMemoryBufferByteAccess>();
            uint8_t* bytes = nullptr;
            uint32_t capacity = 0;
            winrt::check_hresult(byteAccess->GetBuffer(&bytes, &capacity));

            const auto plane = buffer.GetPlaneDescription(0);
            const auto srcBytesPerLine = image.bytesPerLine();
            const auto dstBytesPerLine = static_cast<int>(plane.Stride);
            const auto copyBytes = std::min(srcBytesPerLine, dstBytesPerLine);
            for (int y = 0; y < image.height(); ++y) {
                const auto targetOffset = plane.StartIndex + (y * plane.Stride);
                if (targetOffset + copyBytes > static_cast<int>(capacity)) {
                    break;
                }
                std::memcpy(bytes + targetOffset, image.constScanLine(y), copyBytes);
            }
        }

        const auto result = engine.RecognizeAsync(bitmap).get();
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
                blocks.push_back(OcrBlockInfo{t, QRect(minX, minY, maxX - minX, maxY - minY)});
            }
        }
        const auto text = lines.join('\n').trimmed();
        if (text.isEmpty()) {
            return {false, {}, "No text was recognized in the selected region.", {}, {}};
        }
        return {true, text, {}, source, blocks};
    } catch (...) {
        return {false, {}, "OCR failed while processing the selected region.", {}, {}};
    }
#else
    Q_UNUSED(source)
    return {false, {}, "OCR is not available in this build.", {}, {}};
#endif
}

} // namespace snappaste
