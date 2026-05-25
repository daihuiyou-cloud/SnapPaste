#pragma once

#include <QRect>
#include <QVector>

namespace snappaste {

class CaptureSelectionHistory final {
public:
    explicit CaptureSelectionHistory(int capacity = 20);

    void add(const QRect& region);
    bool isEmpty() const noexcept;
    int size() const noexcept;
    QVector<QRect> entries() const;
    QRect previous();
    QRect next();

private:
    int capacity_ = 20;
    QVector<QRect> entries_;
    int cursor_ = -1;
};

} // namespace snappaste
