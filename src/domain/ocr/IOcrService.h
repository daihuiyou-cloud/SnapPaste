#pragma once

#include "domain/ocr/OcrTypes.h"

#include <functional>

namespace snappaste {

class IOcrService {
public:
    virtual ~IOcrService() = default;

    virtual OcrResult recognizeText(const QImage& image) = 0;

    virtual void recognizeTextAsync(const QImage& image, std::function<void(OcrResult)> callback)
    {
        callback(recognizeText(image));
    }

    virtual void setLanguage(const QString& bcp47Tag) = 0;
    virtual void cancel() {}
};

} // namespace snappaste
