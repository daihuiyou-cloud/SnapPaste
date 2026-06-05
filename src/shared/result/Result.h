#pragma once

#include <QString>

#include <optional>
#include <utility>

namespace snappaste {

template <typename T>
class Result final {
public:
    static Result success(T value)
    {
        Result r;
        r.ok_ = true;
        r.value_.emplace(std::move(value));
        return r;
    }

    static Result failure(QString error)
    {
        Result r;
        r.error_ = std::move(error);
        return r;
    }

    bool isOk() const noexcept { return ok_; }
    bool isError() const noexcept { return !ok_; }
    const T& value() const noexcept { Q_ASSERT(ok_); return *value_; }
    T& value() noexcept { Q_ASSERT(ok_); return *value_; }
    T takeValue() noexcept { Q_ASSERT(ok_); return std::move(*value_); }
    const QString& error() const noexcept { return error_; }

private:
    Result() = default;

    bool ok_ = false;
    std::optional<T> value_;
    QString error_;
};

template <>
class Result<void> final {
public:
    static Result success() { return Result(true, {}); }
    static Result failure(QString error) { return Result(false, std::move(error)); }

    bool isOk() const noexcept { return ok_; }
    bool isError() const noexcept { return !ok_; }
    const QString& error() const noexcept { return error_; }

private:
    Result(bool ok, QString error)
        : ok_(ok)
        , error_(std::move(error))
    {
    }

    bool ok_ = false;
    QString error_;
};

} // namespace snappaste
