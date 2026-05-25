#pragma once

#include "presentation/viewmodels/HistoryViewModel.h"

#include <QWidget>

class QListView;

namespace nanosnap {

class HistoryWidget final : public QWidget {
    Q_OBJECT

public:
    explicit HistoryWidget(HistoryViewModel& viewModel, QWidget* parent = nullptr);

private:
    HistoryViewModel& viewModel_;
    QListView* listView_ = nullptr;
};

} // namespace nanosnap
