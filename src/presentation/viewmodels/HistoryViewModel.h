#pragma once

#include "domain/history/IHistoryRepository.h"
#include "shared/result/Result.h"

#include <QStandardItemModel>

namespace snappaste {

class HistoryViewModel final : public QObject {
    Q_OBJECT

public:
    explicit HistoryViewModel(IHistoryRepository& repository, QObject* parent = nullptr);

    QStandardItemModel* model() noexcept;
    double devicePixelRatio(const QString& filePath) const;

public slots:
    void refresh();
    void deleteByRow(int row);

signals:
    void errorOccurred(const QString& message);

private:
    Result<QVector<CaptureRecord>> recentCaptures(int limit);
    Result<void> deleteCapture(qint64 id);

    IHistoryRepository& repository_;
    QStandardItemModel model_;
    QVector<CaptureRecord> records_;
};

} // namespace snappaste
