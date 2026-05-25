#include "presentation/viewmodels/PinViewModel.h"

namespace nanosnap {

PinViewModel::PinViewModel(PinnedImageService& service, QObject* parent)
    : QObject(parent)
    , service_(service)
{
}

void PinViewModel::createFromImage(const QImage& image, PinSource source)
{
    const auto result = service_.createFromImage(image, source);
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    emit pinCreated(result.value());
}

void PinViewModel::createFromClipboard()
{
    const auto result = service_.createFromClipboard();
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    emit pinCreated(result.value());
}

void PinViewModel::restore()
{
    const auto result = service_.restorePinnedItems();
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    for (const auto& item : result.value()) {
        emit pinRestored(item);
    }
}

void PinViewModel::updateState(qint64 id, const PinnedImageState& state)
{
    const auto result = service_.updateState(id, state);
    if (result.isError()) {
        emit errorOccurred(result.error());
    }
}

void PinViewModel::setAllVisible(bool visible)
{
    const auto result = service_.setAllVisible(visible);
    if (result.isError()) {
        emit errorOccurred(result.error());
    }
}

void PinViewModel::close(qint64 id)
{
    const auto result = service_.close(id);
    if (result.isError()) {
        emit errorOccurred(result.error());
    }
}

} // namespace nanosnap
