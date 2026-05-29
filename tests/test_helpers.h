#pragma once

#include "domain/pin/IClipboardImageProvider.h"
#include "shared/result/Result.h"

#include <QImage>
#include <QString>

#include <utility>

namespace snappaste {

class ScopedEnvVar final {
public:
    ScopedEnvVar(const char* name, const QString& value)
        : name_(name)
    {
        previous_ = qEnvironmentVariable(name);
        if (!value.isEmpty()) {
            qputenv(name_, value.toLocal8Bit());
        } else {
            qunsetenv(name_);
        }
    }

    ~ScopedEnvVar()
    {
        if (!previous_.isEmpty()) {
            qputenv(name_, previous_.toLocal8Bit());
        } else {
            qunsetenv(name_);
        }
    }

    ScopedEnvVar(const ScopedEnvVar&) = delete;
    ScopedEnvVar& operator=(const ScopedEnvVar&) = delete;

private:
    const char* name_;
    QString previous_;
};

class FakeClipboardImageProvider final : public IClipboardImageProvider {
public:
    Result<QImage> imageFromClipboard() override
    {
        if (image.isNull()) {
            return Result<QImage>::failure("No clipboard image.");
        }
        return Result<QImage>::success(image);
    }

    QImage image;
};

} // namespace snappaste
