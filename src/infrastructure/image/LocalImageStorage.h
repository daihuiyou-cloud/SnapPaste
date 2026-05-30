#pragma once

#include "domain/capture/IImageStorage.h"
#include "infrastructure/filesystem/IAppPaths.h"

namespace snappaste {

class LocalImageStorage final : public IImageStorage {
public:
    explicit LocalImageStorage(IAppPaths& appPaths);

    Result<StoredImage> saveCapture(const QImage& image,
                                    const QString& directory,
                                    const QString& format) override;

private:
    QString nextBaseName() const;

    IAppPaths& appPaths_;
};

} // namespace snappaste
