#pragma once

#include "presentation/viewmodels/HistoryViewModel.h"

#include <QSortFilterProxyModel>
#include <QWidget>

class QLineEdit;
class QListView;

namespace snappaste {

class HistoryWidget final : public QWidget {
    Q_OBJECT

public:
    explicit HistoryWidget(HistoryViewModel& viewModel, QWidget* parent = nullptr);

signals:
    void repinRequested(const QString& filePath);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void deleteSelected();

    HistoryViewModel& viewModel_;
    QListView* listView_ = nullptr;
    QLineEdit* searchBox_ = nullptr;
    QSortFilterProxyModel* proxyModel_ = nullptr;
};

} // namespace snappaste
