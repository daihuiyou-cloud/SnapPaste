#include "presentation/history/HistoryWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace snappaste {

HistoryWidget::HistoryWidget(HistoryViewModel& viewModel, QWidget* parent)
    : QWidget(parent)
    , viewModel_(viewModel)
    , listView_(new QListView(this))
    , searchBox_(new QLineEdit(this))
    , proxyModel_(new QSortFilterProxyModel(this))
{
    searchBox_->setPlaceholderText("Search history...");
    searchBox_->setClearButtonEnabled(true);
    searchBox_->setStyleSheet(
        "QLineEdit {"
        " padding: 4px 8px;"
        " border: 1px solid #3a3a3c;"
        " border-radius: 4px;"
        " font: 10pt 'Segoe UI';"
        "}");

    auto* refreshButton = new QPushButton("Refresh", this);
    auto* pinButton = new QPushButton("Pin", this);
    auto* copyButton = new QPushButton("Copy", this);
    auto* openButton = new QPushButton("Open", this);
    auto* showInExplorerButton = new QPushButton("Explore", this);
    auto* deleteButton = new QPushButton("Delete", this);

    proxyModel_->setSourceModel(viewModel_.model());
    proxyModel_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel_->setFilterRole(Qt::DisplayRole);
    listView_->setModel(proxyModel_);
    listView_->setIconSize(QSize(48, 48));
    listView_->setSpacing(2);
    listView_->setContextMenuPolicy(Qt::CustomContextMenu);

    auto* searchLayout = new QHBoxLayout();
    auto* searchLabel = new QLabel("Search:", this);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(searchBox_, 1);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(pinButton);
    buttonLayout->addWidget(copyButton);
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(showInExplorerButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(searchLayout);
    layout->addLayout(buttonLayout);
    layout->addWidget(listView_);
    setLayout(layout);

    connect(searchBox_, &QLineEdit::textChanged, this, [this](const QString& text) {
        proxyModel_->setFilterFixedString(text);
    });

    connect(refreshButton, &QPushButton::clicked, &viewModel_, &HistoryViewModel::refresh);
    auto emitRepin = [this] {
        const auto proxyIndex = listView_->currentIndex();
        if (!proxyIndex.isValid()) {
            QMessageBox::information(this, "SnapPaste", "No capture selected.");
            return;
        }
        const auto sourceIndex = proxyModel_->mapToSource(proxyIndex);
        emit repinRequested(sourceIndex.data(Qt::UserRole + 2).toString());
    };
    connect(pinButton, &QPushButton::clicked, this, emitRepin);
    connect(listView_, &QListView::doubleClicked, this, emitRepin);
    connect(deleteButton, &QPushButton::clicked, this, [this] {
        const auto proxyIndex = listView_->currentIndex();
        if (!proxyIndex.isValid()) {
            QMessageBox::information(this, "SnapPaste", "No capture selected.");
            return;
        }
        auto ret = QMessageBox::question(this, "Delete Capture",
            "Are you sure you want to delete this capture?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            const auto sourceIndex = proxyModel_->mapToSource(proxyIndex);
            viewModel_.deleteByRow(sourceIndex.row());
        }
    });
    connect(copyButton, &QPushButton::clicked, this, [this] {
        const auto proxyIndex = listView_->currentIndex();
        if (!proxyIndex.isValid()) {
            QMessageBox::information(this, "SnapPaste", "No capture selected.");
            return;
        }
        const auto sourceIndex = proxyModel_->mapToSource(proxyIndex);
        const auto filePath = sourceIndex.data(Qt::UserRole + 2).toString();
        QImage img(filePath);
        if (img.isNull()) {
            QMessageBox::warning(this, "SnapPaste", "Failed to load image.");
            return;
        }
        QApplication::clipboard()->setImage(img);
    });
    connect(openButton, &QPushButton::clicked, this, [this] {
        const auto proxyIndex = listView_->currentIndex();
        if (!proxyIndex.isValid()) {
            return;
        }
        const auto sourceIndex = proxyModel_->mapToSource(proxyIndex);
        const auto filePath = sourceIndex.data(Qt::UserRole + 2).toString();
        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    });
    connect(showInExplorerButton, &QPushButton::clicked, this, [this] {
        const auto proxyIndex = listView_->currentIndex();
        if (!proxyIndex.isValid()) {
            return;
        }
        const auto sourceIndex = proxyModel_->mapToSource(proxyIndex);
        const auto filePath = sourceIndex.data(Qt::UserRole + 2).toString();
        const QFileInfo fi(filePath);
#ifdef Q_OS_WIN
        QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(filePath)});
#else
        QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
#endif
    });
    connect(&viewModel_, &HistoryViewModel::errorOccurred, this, [this](const QString& message) {
        QMessageBox::warning(this, "SnapPaste", message);
    });

    connect(listView_, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
        const auto proxyIndex = listView_->indexAt(pos);
        if (!proxyIndex.isValid()) {
            return;
        }
        const auto sourceIndex = proxyModel_->mapToSource(proxyIndex);
        const auto filePath = sourceIndex.data(Qt::UserRole + 2).toString();

        QMenu menu(this);
        auto* pinAction = menu.addAction("Pin");
        auto* copyAction = menu.addAction("Copy");
        auto* openAction = menu.addAction("Open");
        auto* exploreAction = menu.addAction("Show in Explorer");
        menu.addSeparator();
        auto* deleteAction = menu.addAction("Delete");

        const auto* action = menu.exec(listView_->mapToGlobal(pos));
        if (action == pinAction) {
            emit repinRequested(filePath);
        } else if (action == copyAction) {
            QImage img(filePath);
            if (!img.isNull()) {
                QApplication::clipboard()->setImage(img);
            }
        } else if (action == openAction) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        } else if (action == exploreAction) {
            const QFileInfo fi(filePath);
#ifdef Q_OS_WIN
            QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(filePath)});
#else
            QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
#endif
        } else if (action == deleteAction) {
            auto ret = QMessageBox::question(this, "Delete Capture",
                "Are you sure you want to delete this capture?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                viewModel_.deleteByRow(sourceIndex.row());
            }
        }
    });

    viewModel_.refresh();
}

} // namespace snappaste