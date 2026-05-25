#pragma once

#include "shared/result/Result.h"

#include <QImage>

namespace nanosnap {

class IClipboardImageProvider {
public:
    virtual ~IClipboardImageProvider() = default;

    virtual Result<QImage> imageFromClipboard() = 0;
};

} // namespace nanosnap
