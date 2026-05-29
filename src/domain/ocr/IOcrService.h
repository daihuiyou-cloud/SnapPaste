#pragma once

#include "domain/ocr/OcrTypes.h"

namespace snappaste {

class IOcrService {
public:
    virtual ~IOcrService() = default;

    virtual OcrResult recognizeText(const QImage& image) = 0;
    virtual void setLanguage(const QString& bcp47Tag) = 0;
    virtual void cancel() {}
};

} // namespace snappaste
