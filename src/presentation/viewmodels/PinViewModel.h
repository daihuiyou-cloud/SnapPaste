#pragma once

#include "domain/pin/PinnedImageService.h"

#include <QObject>

namespace snappaste {

class PinViewModel final : public QObject {
    Q_OBJECT

public:
    explicit PinViewModel(PinnedImageService& service, QObject* parent = nullptr);

public slots:
    void createFromImage(QImage image, PinSource source);
    void createFromClipboard();
    void restore();
    void updateState(qint64 id, const PinnedImageState& state);
    void setAllVisible(bool visible);
    void close(qint64 id);

signals:
    void pinCreated(const PinnedItem& item);
    void pinRestored(const PinnedItem& item);
    void errorOccurred(const QString& message);

private:
    PinnedImageService& service_;
};

} // namespace snappaste
