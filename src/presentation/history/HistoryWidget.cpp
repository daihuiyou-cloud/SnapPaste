#include "presentation/history/HistoryWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QSet>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace snappaste {

namespace {

QString filePathFromIndex(const QModelIndex& proxyIndex, QSortFilterProxyModel* proxy)
{
    if (!proxyIndex.isValid()) return {};
    return proxy->mapToSource(proxyIndex).data(Qt::UserRole + 2).toString();
}

QVector<QString> selectedFilePaths(const QListView* view, QSortFilterProxyModel* proxy)
{
    const auto selected = view->selectionModel()->selectedIndexes();
    QVector<QString> paths;
    paths.reserve(selected.size());
    for (const auto& idx : selected) {
        auto p = filePathFromIndex(idx, proxy);
        if (!p.isEmpty() && !paths.contains(p)) {
            paths.append(p);
        }
    }
    return paths;
}

} // namespace

HistoryWidget::HistoryWidget(HistoryViewModel& viewModel, QWidget* parent)
    : QWidget(parent)
    , viewModel_(viewModel)
    , listView_(new QListView(this))
    , searchBox_(new QLineEdit(this))
    , proxyModel_(new QSortFilterProxyModel(this))
{
    searchBox_->setPlaceholderText(tr("Search by filename..."));
    searchBox_->setClearButtonEnabled(true);
    searchBox_->setStyleSheet(
        "QLineEdit {"
        " padding: 4px 8px;"
        " border: 1px solid #3a3a3c;"
        " border-radius: 4px;"
        " font: 10pt 'Segoe UI';"
        "}");

    auto* refreshButton = new QPushButton(tr("Refresh"), this);
    auto* pinButton = new QPushButton(tr("Pin"), this);
    auto* copyButton = new QPushButton(tr("Copy"), this);
    auto* openButton = new QPushButton(tr("Open"), this);
    auto* showInExplorerButton = new QPushButton(tr("Show in Explorer"), this);
    auto* deleteButton = new QPushButton(tr("Delete"), this);

    proxyModel_->setSourceModel(viewModel_.model());
    proxyModel_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel_->setFilterRole(Qt::DisplayRole);
    listView_->setModel(proxyModel_);
    listView_->setIconSize(QSize(48, 48));
    listView_->setSpacing(2);
    listView_->setUniformItemSizes(true);
    listView_->setContextMenuPolicy(Qt::CustomContextMenu);
    listView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    listView_->installEventFilter(this);

    auto* searchLayout = new QHBoxLayout();
    auto* searchLabel = new QLabel(tr("Search:"), this);
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
    auto emitRepinFirst = [this] {
        const auto paths = selectedFilePaths(listView_, proxyModel_);
        if (paths.isEmpty()) {
            QMessageBox::information(this, tr("SnapPaste"), tr("No captures selected."));
            return;
        }
        emit repinRequested(paths.first());
    };
    connect(pinButton, &QPushButton::clicked, this, emitRepinFirst);
    connect(listView_, &QListView::doubleClicked, this, emitRepinFirst);
    connect(deleteButton, &QPushButton::clicked, this, [this] {
        const auto paths = selectedFilePaths(listView_, proxyModel_);
        if (paths.isEmpty()) {
            QMessageBox::information(this, tr("SnapPaste"), tr("No captures selected."));
            return;
        }
        const QString msg = paths.size() == 1
            ? tr("Are you sure you want to delete this capture?")
            : tr("Are you sure you want to delete %1 captures?").arg(paths.size());
        auto ret = QMessageBox::question(this, tr("Delete Capture"), msg,
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            const auto selected = listView_->selectionModel()->selectedIndexes();
            QSet<int> processedRows;
            QVector<int> rowsToDelete;
            for (const auto& idx : selected) {
                const auto row = proxyModel_->mapToSource(idx).row();
                if (!processedRows.contains(row)) {
                    processedRows.insert(row);
                    rowsToDelete.append(row);
                }
            }
            std::sort(rowsToDelete.begin(), rowsToDelete.end(), std::greater<int>());
            for (const auto row : rowsToDelete) {
                viewModel_.deleteByRow(row);
            }
            listView_->selectionModel()->clear();
        }
    });
    connect(copyButton, &QPushButton::clicked, this, [this] {
        const auto paths = selectedFilePaths(listView_, proxyModel_);
        if (paths.isEmpty()) {
            QMessageBox::information(this, tr("SnapPaste"), tr("No captures selected."));
            return;
        }
        if (paths.size() == 1) {
            QImage img(paths.first());
            if (img.isNull()) {
                QMessageBox::warning(this, tr("SnapPaste"), tr("Failed to load image."));
                return;
            }
            const QSignalBlocker blocker(QApplication::clipboard());
            QApplication::clipboard()->setImage(img);
        } else {
            QStringList fileList;
            fileList.reserve(paths.size());
            for (const auto& p : paths) {
                fileList << QDir::toNativeSeparators(p);
            }
            const QSignalBlocker blocker(QApplication::clipboard());
            QApplication::clipboard()->setText(fileList.join("\n"));
        }
    });
    connect(openButton, &QPushButton::clicked, this, [this] {
        const auto paths = selectedFilePaths(listView_, proxyModel_);
        if (paths.isEmpty()) return;
        for (const auto& path : paths) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    });
    connect(showInExplorerButton, &QPushButton::clicked, this, [this] {
        const auto paths = selectedFilePaths(listView_, proxyModel_);
        if (paths.isEmpty()) return;
        const QString firstPath = paths.first();
        const QFileInfo fi(firstPath);
#ifdef Q_OS_WIN
        QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(firstPath)});
#else
        QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
#endif
    });
    connect(&viewModel_, &HistoryViewModel::errorOccurred, this, [this](const QString& message) {
        QMessageBox::warning(this, tr("SnapPaste"), message);
    });

    connect(listView_, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
        const auto paths = selectedFilePaths(listView_, proxyModel_);
        if (paths.isEmpty()) return;

        QMenu menu(this);
        auto* pinAction = paths.size() == 1 ? menu.addAction(tr("Pin")) : nullptr;
        auto* copyAction = menu.addAction(paths.size() == 1 ? tr("Copy") : tr("Copy All"));
        auto* openAction = menu.addAction(tr("Open"));
        auto* exploreAction = menu.addAction(tr("Show in Explorer"));
        menu.addSeparator();
        auto* deleteAction = menu.addAction(paths.size() == 1 ? tr("Delete") : tr("Delete All"));

        const auto* action = menu.exec(listView_->mapToGlobal(pos));
        if (action == pinAction && pinAction) {
            emit repinRequested(paths.first());
        } else if (action == copyAction) {
            if (paths.size() == 1) {
                QImage img(paths.first());
                if (img.isNull()) {
                    QMessageBox::warning(this, tr("SnapPaste"), tr("Failed to load image."));
                } else {
                    const QSignalBlocker blocker(QApplication::clipboard());
                    QApplication::clipboard()->setImage(img);
                }
            } else {
                QStringList fileList;
                for (const auto& p : paths) fileList << QDir::toNativeSeparators(p);
                const QSignalBlocker blocker(QApplication::clipboard());
                QApplication::clipboard()->setText(fileList.join("\n"));
            }
        } else if (action == openAction) {
            for (const auto& path : paths)
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        } else if (action == exploreAction) {
            const QFileInfo fi(paths.first());
#ifdef Q_OS_WIN
            QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(paths.first())});
#else
            QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
#endif
        } else if (action == deleteAction) {
            const QString msg = paths.size() == 1
                ? tr("Are you sure you want to delete this capture?")
                : tr("Delete %1 captures?").arg(paths.size());
            auto ret = QMessageBox::question(this, tr("Delete"), msg,
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                const auto selected = listView_->selectionModel()->selectedIndexes();
                QSet<int> processedRows;
                QVector<int> rowsToDelete;
                for (const auto& idx : selected) {
                    const auto row = proxyModel_->mapToSource(idx).row();
                    if (!processedRows.contains(row)) {
                        processedRows.insert(row);
                        rowsToDelete.append(row);
                    }
                }
                std::sort(rowsToDelete.begin(), rowsToDelete.end(), std::greater<int>());
                for (const auto row : rowsToDelete) {
                    viewModel_.deleteByRow(row);
                }
                listView_->selectionModel()->clear();
            }
        }
    });

    viewModel_.refresh();
}

void HistoryWidget::deleteSelected()
{
    const auto paths = selectedFilePaths(listView_, proxyModel_);
    if (paths.isEmpty()) return;
    const QString msg = paths.size() == 1
        ? tr("Are you sure you want to delete this capture?")
        : tr("Delete %1 captures?").arg(paths.size());
    auto ret = QMessageBox::question(this, tr("Delete"), msg,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    const auto selected = listView_->selectionModel()->selectedIndexes();
    QSet<int> processedRows;
    QVector<int> rowsToDelete;
    for (const auto& idx : selected) {
        const auto row = proxyModel_->mapToSource(idx).row();
        if (!processedRows.contains(row)) {
            processedRows.insert(row);
            rowsToDelete.append(row);
        }
    }
    std::sort(rowsToDelete.begin(), rowsToDelete.end(), std::greater<int>());
    for (const auto row : rowsToDelete) {
        viewModel_.deleteByRow(row);
    }
    listView_->selectionModel()->clear();
}

bool HistoryWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == listView_ && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
            deleteSelected();
            return true;
        }
        if (keyEvent->key() == Qt::Key_A && keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
            listView_->selectAll();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

} // namespace snappaste