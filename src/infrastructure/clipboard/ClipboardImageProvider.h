#pragma once

#include "domain/pin/IClipboardImageProvider.h"

namespace nanosnap {

class ClipboardImageProvider final : public IClipboardImageProvider {
public:
    Result<QImage> imageFromClipboard() override;

private:
    static Result<QImage> imageFromHtml(const QString& html);
    static Result<QImage> imageFromText(const QString& text);
    static Result<QImage> colorImageFromText(const QString& text);
};

} // namespace nanosnap
