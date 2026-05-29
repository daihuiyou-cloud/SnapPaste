#pragma once

#include "domain/ocr/IOcrService.h"

#include <QMutex>
#include <atomic>

namespace snappaste {

class WindowsOcrService final : public IOcrService {
public:
    WindowsOcrService();
    ~WindowsOcrService() override;

    OcrResult recognizeText(const QImage& image) override;
    void setLanguage(const QString& bcp47Tag) override;
    void cancel() override;

private:
    QString language_;
    QMutex mutex_;
    bool apartmentInitialized_ = false;
    std::atomic<bool> cancelled_{false};
};

} // namespace snappaste
