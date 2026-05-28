#pragma once

#include "domain/history/HistoryService.h"

#include <QStandardItemModel>

namespace snappaste {

class HistoryViewModel final : public QObject {
    Q_OBJECT

public:
    explicit HistoryViewModel(HistoryService& service, QObject* parent = nullptr);

    QStandardItemModel* model() noexcept;
    double devicePixelRatio(const QString& filePath) const;

public slots:
    void refresh();
    void deleteByRow(int row);

signals:
    void errorOccurred(const QString& message);

private:
    HistoryService& service_;
    QStandardItemModel model_;
    QVector<CaptureRecord> records_;
};

} // namespace snappaste
