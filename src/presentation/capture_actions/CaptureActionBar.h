#pragma once

#include <QPoint>
#include <QRect>
#include <QWidget>

namespace snappaste {

class CaptureActionBar final : public QWidget {
    Q_OBJECT

public:
    explicit CaptureActionBar(QWidget* parent = nullptr);

    void showForRegion(const QRect& region, const QRect& availableGeometry);
    static QPoint anchoredPosition(const QRect& region, const QSize& barSize, const QRect& availableGeometry);

signals:
    void copyRequested();
    void pinRequested();
    void saveRequested();
    void editRequested();
    void ocrRequested();
    void cancelRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
};

} // namespace snappaste
