#include "presentation/viewmodels/HistoryViewModel.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QPixmap>
#include <QStandardItem>

namespace snappaste {

HistoryViewModel::HistoryViewModel(IHistoryRepository& repository, QObject* parent)
    : QObject(parent)
    , repository_(repository)
{
    model_.setHorizontalHeaderLabels({tr("Capture History")});
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
    model_.clear();
    model_.setHorizontalHeaderLabels({tr("Capture History")});

    for (const auto& record : records_) {
        const QFileInfo file(record.filePath);
        auto* item = new QStandardItem(file.fileName());
        item->setData(record.id, Qt::UserRole + 1);
        item->setData(record.filePath, Qt::UserRole + 2);
        item->setData(record.thumbnailPath, Qt::UserRole + 3);
        item->setData(record.devicePixelRatio, Qt::UserRole + 4);
        if (!record.thumbnailPath.isEmpty()) {
            QPixmap thumb(record.thumbnailPath);
            if (!thumb.isNull()) {
                item->setData(thumb.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation),
                              Qt::DecorationRole);
            }
        }
        item->setToolTip(tr("%1\n%2x%3").arg(record.filePath).arg(record.width).arg(record.height));
        model_.appendRow(item);
    }
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
        qWarning() << "Failed to remove file:" << record.filePath;
    }
    if (!record.thumbnailPath.isEmpty()) {
        if (!QFile::remove(record.thumbnailPath) && QFileInfo::exists(record.thumbnailPath)) {
            qWarning() << "Failed to remove thumbnail:" << record.thumbnailPath;
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
