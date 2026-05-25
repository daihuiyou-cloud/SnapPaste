#include "presentation/viewmodels/HistoryViewModel.h"

#include <QFileInfo>
#include <QStandardItem>

namespace snappaste {

HistoryViewModel::HistoryViewModel(HistoryService& service, QObject* parent)
    : QObject(parent)
    , service_(service)
{
    model_.setHorizontalHeaderLabels({"Capture History"});
}

QStandardItemModel* HistoryViewModel::model() noexcept
{
    return &model_;
}

void HistoryViewModel::refresh()
{
    const auto result = service_.recentCaptures(100);
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    records_ = result.value();
    model_.clear();
    model_.setHorizontalHeaderLabels({"Capture History"});

    for (const auto& record : records_) {
        const QFileInfo file(record.filePath);
        auto* item = new QStandardItem(file.fileName());
        item->setData(record.id, Qt::UserRole + 1);
        item->setData(record.filePath, Qt::UserRole + 2);
        item->setToolTip(record.filePath);
        model_.appendRow(item);
    }
}

void HistoryViewModel::deleteByRow(int row)
{
    if (row < 0 || row >= records_.size()) {
        return;
    }

    const auto result = service_.deleteCapture(records_.at(row).id);
    if (result.isError()) {
        emit errorOccurred(result.error());
        return;
    }

    refresh();
}

} // namespace snappaste
