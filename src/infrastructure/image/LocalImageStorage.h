#pragma once

#include "domain/capture/IImageStorage.h"

namespace snappaste {

class LocalImageStorage final : public IImageStorage {
public:
    Result<StoredImage> saveCapture(const QImage& image,
                                    const QString& directory,
                                    const QString& format) override;

private:
    QString nextBaseName() const;
};

} // namespace snappaste
