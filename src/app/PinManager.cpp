#include "app/PinManager.h"

#include "presentation/pin_window/EditToolbarWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QPointer>
#include <QScreen>
#include <QSignalBlocker>
#include <QTimer>

namespace snappaste {

namespace {

constexpr int kPinBaseOffset = 16;
constexpr int kPinCascadeOffset = 24;
constexpr int kPinCascadeSlots = 8;
constexpr int kMargin = 12;

} // namespace

PinManager::PinManager(IIconProvider& iconProvider, PinViewModel& pinViewModel, QObject* parent)
    : QObject(parent)
    , iconProvider_(iconProvider)
    , pinViewModel_(pinViewModel)
{
}

void PinManager::openPinWindow(PinnedItem item,
                               std::optional<QPoint> position,
                               std::optional<QRect> avoidRegion)
{
    if (item.image.isNull()) {
        return;
    }

    if (position.has_value()) {
        item.state.position = pinnedPositionFor(item.image.size(), position.value(), avoidRegion);
        pinViewModel_.updateState(item.id, item.state);
    }

    auto pinWindow = std::make_unique<PinWindow>(item, iconProvider_);
    auto* window = pinWindow.get();
    const auto id = item.id;
    pinWindows_[id] = std::move(pinWindow);
    if (pendingPinSlot_ >= 0) {
        pinIdToSlot_[id] = pendingPinSlot_;
        pendingPinSlot_ = -1;
    }

    connect(window, &PinWindow::stateChanged, &pinViewModel_, &PinViewModel::updateState);
    connect(window, &PinWindow::closeRequested, this, [this](qint64 id) {
        pinViewModel_.close(id);
        const int slot = [this, id] {
            auto it = pinIdToSlot_.find(id);
            return it != pinIdToSlot_.end() ? it->second : -1;
        }();
        QPointer<PinManager> guard(this);
        QTimer::singleShot(0, this, [guard, id, slot] {
            if (guard) {
                guard->pinWindows_.erase(id);
                guard->pinIdToSlot_.erase(id);
                if (slot >= 0) guard->freePinSlot(slot);
            }
        });
    });
    connect(window, &PinWindow::copyRequested, this, [this, source = item.source](const QImage& image) {
        if (image.isNull()) return;
        emit copyRequested(0, image, source);
    });
    connect(window, &PinWindow::saveRequested, this, [this](const QImage& image) {
        emit saveRequested(image);
    });
    connect(window, &PinWindow::ocrRequested, this, [this](qint64 id, const QImage& image) {
        emit ocrRequested(id, image);
    });

    if (window->state().options.visible) {
        window->show();
        window->raise();
    }
}

void PinManager::hideAll()
{
    pinViewModel_.setAllVisible(false);
    for (auto& entry : pinWindows_) {
        if (entry.second) {
            entry.second->setPinnedVisible(false);
        }
    }
}

void PinManager::showAll()
{
    pinViewModel_.setAllVisible(true);
    for (auto& entry : pinWindows_) {
        if (entry.second) {
            entry.second->restoreInteraction();
        }
    }
}

void PinManager::closeAll()
{
    const auto ids = [this] {
        QVector<qint64> result;
        result.reserve(pinWindows_.size());
        for (const auto& entry : pinWindows_) {
            result.push_back(entry.first);
        }
        return result;
    }();
    for (const auto id : ids) {
        pinViewModel_.close(id);
        auto slotIt = pinIdToSlot_.find(id);
        if (slotIt != pinIdToSlot_.end()) {
            freePinSlot(slotIt->second);
        }
        pinIdToSlot_.erase(id);
        pinWindows_.erase(id);
    }
}

int PinManager::allocatePinSlot()
{
    if (!freePinSlots_.empty()) {
        int slot = *freePinSlots_.begin();
        freePinSlots_.erase(freePinSlots_.begin());
        return slot;
    }
    return (nextPinSlot_++) % kPinCascadeSlots;
}

void PinManager::freePinSlot(int slot)
{
    freePinSlots_.insert(slot);
}

QPoint PinManager::cascadedPinPosition(const QPoint& basePosition)
{
    const auto slot = allocatePinSlot();
    const auto offset = slot * kPinCascadeOffset;
    pendingPinSlot_ = slot;
    return basePosition + QPoint(offset, offset);
}

QPoint PinManager::pinnedPositionFor(const QSize& imageSize,
                                     const QPoint& preferredPosition,
                                     const std::optional<QRect>& avoidRegion) const
{
    auto position = preferredPosition;
    const auto screen = QGuiApplication::screenAt(preferredPosition);
    const auto fallback = QGuiApplication::primaryScreen();
    const auto bounds = screen != nullptr ? screen->availableGeometry()
        : fallback != nullptr ? fallback->availableGeometry()
        : QRect(0, 0, 1920, 1080);
    QRect pinRect(position, imageSize);

    if (avoidRegion.has_value() && pinRect.intersects(avoidRegion.value())) {
        position = QPoint(avoidRegion->right() + kMargin, avoidRegion->top());
        pinRect.moveTopLeft(position);
        if (!bounds.contains(pinRect)) {
            position = QPoint(avoidRegion->left(), avoidRegion->bottom() + kMargin);
            pinRect.moveTopLeft(position);
        }
    }

    if (pinRect.right() > bounds.right() - kMargin) {
        position.setX(bounds.right() - imageSize.width() - kMargin);
    }
    if (pinRect.bottom() > bounds.bottom() - kMargin) {
        position.setY(bounds.bottom() - imageSize.height() - kMargin);
    }
    if (position.x() < bounds.left() + kMargin) {
        position.setX(bounds.left() + kMargin);
    }
    if (position.y() < bounds.top() + kMargin) {
        position.setY(bounds.top() + kMargin);
    }

    return position;
}

} // namespace snappaste
