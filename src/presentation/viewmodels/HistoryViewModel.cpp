#include "presentation/viewmodels/HistoryViewModel.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPixmap>
#include <QPixmapCache>
#include <QStandardItem>
#include <QTimer>

namespace snappaste {

namespace {
    constexpr int kThumbSize = 48;
    constexpr int kBatchSize = 8;
}

QString HistoryViewModel::fileNameFromPath(const QString& filePath)
{
    auto slashPos = filePath.lastIndexOf('/');
    auto backslashPos = filePath.lastIndexOf('\\');
    auto nameStart = qMax(slashPos, backslashPos) + 1;
    return filePath.mid(nameStart);
}

HistoryViewModel::HistoryViewModel(IHistoryRepository& repository, QObject* parent)
    : QObject(parent)
    , repository_(repository)
    , batchTimer_(new QTimer(this))
{
    model_.setHorizontalHeaderLabels({tr("Capture History")});
    batchTimer_->setInterval(0);
    batchTimer_->setSingleShot(true);
    connect(batchTimer_, &QTimer::timeout, this, &HistoryViewModel::batchLoadSlot);
}

QStandardItemModel* HistoryViewModel::model() noexcept
{
    return &model_;
}

void HistoryViewModel::refresh()
{
    const auto result = recentCaptures(100);
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    records_ = result.value();
    batchLoadIndex_ = 0;
    model_.clear();
    model_.setHorizontalHeaderLabels({tr("Capture History")});

    for (const auto& record : records_) {
        auto* item = new QStandardItem(fileNameFromPath(record.filePath));
        item->setData(record.id, Qt::UserRole + 1);
        item->setData(record.filePath, Qt::UserRole + 2);
        item->setData(record.thumbnailPath, Qt::UserRole + 3);
        item->setData(record.devicePixelRatio, Qt::UserRole + 4);
        item->setToolTip(tr("%1\n%2x%3").arg(record.filePath).arg(record.width).arg(record.height));
        model_.appendRow(item);
    }
    batchTimer_->start();
}

void HistoryViewModel::batchLoadSlot()
{
    int end = qMin(batchLoadIndex_ + kBatchSize, records_.size());
    for (int i = batchLoadIndex_; i < end; ++i)
        loadThumbnail(i);
    batchLoadIndex_ = end;
    if (batchLoadIndex_ < records_.size())
        batchTimer_->start();
}

void HistoryViewModel::loadThumbnail(int row)
{
    const auto& record = records_[row];
    if (record.thumbnailPath.isEmpty())
        return;
    auto* item = model_.item(row);
    if (!item || !item->data(Qt::DecorationRole).isNull())
        return;

    QString cacheKey = QStringLiteral("hthumb_") + record.thumbnailPath;
    QPixmap cached;
    if (QPixmapCache::find(cacheKey, &cached)) {
        item->setData(cached, Qt::DecorationRole);
        return;
    }
    QImage raw(record.thumbnailPath);
    if (raw.isNull())
        return;
    QPixmap thumb = QPixmap::fromImage(
        raw.scaled(kThumbSize, kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QPixmapCache::insert(cacheKey, thumb);
    item->setData(std::move(thumb), Qt::DecorationRole);
}

double HistoryViewModel::devicePixelRatio(const QString& filePath) const
{
    for (const auto& r : records_) {
        if (r.filePath == filePath) return r.devicePixelRatio;
    }
    return 1.0;
}

void HistoryViewModel::deleteByRow(int row)
{
    if (row < 0 || row >= records_.size()) {
        return;
    }

    const auto& record = records_.at(row);

    if (!QFile::remove(record.filePath) && QFileInfo::exists(record.filePath)) {
        qWarning("Failed to remove file: %s", qPrintable(record.filePath));
    }
    if (!record.thumbnailPath.isEmpty()) {
        if (!QFile::remove(record.thumbnailPath) && QFileInfo::exists(record.thumbnailPath)) {
            qWarning("Failed to remove thumbnail: %s", qPrintable(record.thumbnailPath));
        }
    }

    const auto result = deleteCapture(record.id);
    if (result.isError()) {
        emit errorOccurred(result.error());
    }

    refresh();
}

Result<QVector<CaptureRecord>> HistoryViewModel::recentCaptures(int limit)
{
    if (limit <= 0) {
        return Result<QVector<CaptureRecord>>::success({});
    }
    return repository_.recent(limit);
}

Result<void> HistoryViewModel::deleteCapture(qint64 id)
{
    if (id <= 0) {
        return Result<void>::failure(QCoreApplication::translate("AppErrors", "Invalid capture id."));
    }
    return repository_.markDeleted(id);
}

} // namespace snappaste
