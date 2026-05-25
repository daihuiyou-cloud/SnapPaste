#include "presentation/history/HistoryWidget.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace snappaste {

HistoryWidget::HistoryWidget(HistoryViewModel& viewModel, QWidget* parent)
    : QWidget(parent)
    , viewModel_(viewModel)
    , listView_(new QListView(this))
{
    auto* refreshButton = new QPushButton("Refresh", this);
    auto* openButton = new QPushButton("Open", this);
    auto* deleteButton = new QPushButton("Delete", this);

    listView_->setModel(viewModel_.model());

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(buttonLayout);
    layout->addWidget(listView_);
    setLayout(layout);

    connect(refreshButton, &QPushButton::clicked, &viewModel_, &HistoryViewModel::refresh);
    connect(deleteButton, &QPushButton::clicked, this, [this] {
        const auto index = listView_->currentIndex();
        if (!index.isValid()) {
            QMessageBox::information(this, "SnapPaste", "No capture selected.");
            return;
        }
        viewModel_.deleteByRow(index.row());
    });
    connect(openButton, &QPushButton::clicked, this, [this] {
        const auto index = listView_->currentIndex();
        if (!index.isValid()) {
            return;
        }
        const auto filePath = index.data(Qt::UserRole + 2).toString();
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    });
    connect(&viewModel_, &HistoryViewModel::errorOccurred, this, [this](const QString& message) {
        QMessageBox::warning(this, "SnapPaste", message);
    });

    viewModel_.refresh();
}

} // namespace snappaste
