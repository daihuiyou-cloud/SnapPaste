#pragma once

#include "domain/history/IHistoryRepository.h"
#include "shared/result/Result.h"

#include <QStandardItemModel>

class QTimer;

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

private slots:
    void batchLoadSlot();

private:
    Result<QVector<CaptureRecord>> recentCaptures(int limit);
    Result<void> deleteCapture(qint64 id);
    void loadThumbnail(int row);
    static QString fileNameFromPath(const QString& filePath);

    IHistoryRepository& repository_;
    QStandardItemModel model_;
    QVector<CaptureRecord> records_;
    int batchLoadIndex_ = 0;
    QTimer* batchTimer_ = nullptr;
};

} // namespace snappaste
