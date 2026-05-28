#pragma once

#include "domain/ocr/IOcrService.h"

#include <QMutex>

namespace snappaste {

class WindowsOcrService final : public IOcrService {
public:
    WindowsOcrService();
    ~WindowsOcrService() override;

    OcrResult recognizeText(const QImage& image) override;
    void setLanguage(const QString& bcp47Tag) override;

private:
    QString language_;
    QMutex mutex_;
    bool apartmentInitialized_ = false;
};

} // namespace snappaste
