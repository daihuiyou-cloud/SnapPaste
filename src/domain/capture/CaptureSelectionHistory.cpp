#include "domain/capture/CaptureSelectionHistory.h"

#include <algorithm>

namespace snappaste {

CaptureSelectionHistory::CaptureSelectionHistory(int capacity)
    : capacity_(std::max(1, capacity))
{
}

void CaptureSelectionHistory::add(const QRect& region)
{
    const auto normalized = region.normalized();
    if (!normalized.isValid()) {
        return;
    }

    entries_.removeAll(normalized);
    entries_.push_front(normalized);
    while (entries_.size() > capacity_) {
        entries_.pop_back();
    }
    cursor_ = -1;
}

bool CaptureSelectionHistory::isEmpty() const noexcept
{
    return entries_.isEmpty();
}

int CaptureSelectionHistory::size() const noexcept
{
    return entries_.size();
}

QVector<QRect> CaptureSelectionHistory::entries() const noexcept
{
    return entries_;
}

QRect CaptureSelectionHistory::previous()
{
    if (entries_.isEmpty()) {
        return {};
    }
    if (cursor_ < 0) {
        cursor_ = 0;
    } else {
        cursor_ = std::min(cursor_ + 1, entries_.size() - 1);
    }
    return entries_.at(cursor_);
}

QRect CaptureSelectionHistory::next()
{
    if (entries_.isEmpty()) {
        return {};
    }
    if (cursor_ < 0) {
        cursor_ = 0;
        return entries_.at(0);
    }
    if (cursor_ == 0) {
        cursor_ = -1;
        return entries_.at(0);
    }
    cursor_ -= 1;
    return entries_.at(cursor_);
}

} // namespace snappaste
