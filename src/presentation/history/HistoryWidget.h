#pragma once

#include "presentation/viewmodels/HistoryViewModel.h"

#include <QWidget>

class QListView;

namespace snappaste {

class HistoryWidget final : public QWidget {
    Q_OBJECT

public:
    explicit HistoryWidget(HistoryViewModel& viewModel, QWidget* parent = nullptr);

signals:
    void repinRequested(const QString& filePath);

private:
    HistoryViewModel& viewModel_;
    QListView* listView_ = nullptr;
};

} // namespace snappaste
