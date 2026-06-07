#pragma once

#include "presentation/pin_window/PinWindow.h"
#include "presentation/icons/IIconProvider.h"
#include "presentation/viewmodels/PinViewModel.h"

#include <QObject>
#include <QPoint>
#include <QRect>

#include <map>
#include <memory>
#include <optional>
#include <set>

namespace snappaste {

class PinManager final : public QObject {
    Q_OBJECT

public:
    PinManager(IIconProvider& iconProvider, PinViewModel& pinViewModel, QObject* parent = nullptr);

    void openPinWindow(PinnedItem item,
                       std::optional<QPoint> position = std::nullopt,
                       std::optional<QRect> avoidRegion = std::nullopt);

    void hideAll();
    void showAll();
    void closeAll();

    bool hasPins() const { return !pinWindows_.empty(); }

signals:
    void copyRequested(qint64 id, const QImage& image, PinSource source);
    void saveRequested(const QImage& image);
    void ocrRequested(qint64 id, const QImage& image);

private:
    int allocatePinSlot();
    void freePinSlot(int slot);
    QPoint cascadedPinPosition(const QPoint& basePosition);
    QPoint pinnedPositionFor(const QSize& imageSize,
                             const QPoint& preferredPosition,
                             const std::optional<QRect>& avoidRegion) const;

    IIconProvider& iconProvider_;
    PinViewModel& pinViewModel_;
    std::map<qint64, std::unique_ptr<PinWindow>> pinWindows_;
    std::map<qint64, int> pinIdToSlot_;
    std::set<int> freePinSlots_;
    int nextPinSlot_ = 0;
    int pendingPinSlot_ = -1;

    static constexpr int kPinBaseOffset = 16;
    static constexpr int kPinCascadeOffset = 24;
    static constexpr int kPinCascadeSlots = 8;
};

} // namespace snappaste
