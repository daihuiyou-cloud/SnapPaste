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

QVector<QRect> CaptureSelectionHistory::entries() const
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
        cursor_ = (cursor_ - 1 + entries_.size()) % entries_.size();
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
    } else {
        cursor_ = (cursor_ + 1) % entries_.size();
    }
    return entries_.at(cursor_);
}

} // namespace snappaste
