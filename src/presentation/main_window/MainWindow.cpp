#include "presentation/main_window/MainWindow.h"
#include "presentation/history/HistoryWidget.h"
#include "presentation/settings/SettingsWidget.h"

#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace snappaste {

MainWindow::MainWindow(HistoryViewModel& historyViewModel,
                       SettingsViewModel& settingsViewModel,
                       QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("SnapPaste");
    resize(860, 560);

    auto* tabs = new QTabWidget(this);

    auto* home = new QWidget(this);
    auto* captureButton = new QPushButton("Start Capture", home);
    captureButton->setMinimumHeight(42);

    auto* homeLayout = new QVBoxLayout(home);
    homeLayout->addWidget(captureButton, 0, Qt::AlignTop);
    homeLayout->addStretch();
    home->setLayout(homeLayout);

    tabs->addTab(home, "Capture");
    {
        auto* historyWidget = new HistoryWidget(historyViewModel, tabs);
        connect(historyWidget, &HistoryWidget::repinRequested, this, &MainWindow::repinRequested);
        tabs->addTab(historyWidget, "History");
    }
    tabs->addTab(new SettingsWidget(settingsViewModel, tabs), "Settings");

    setCentralWidget(tabs);

    connect(captureButton, &QPushButton::clicked, this, &MainWindow::captureRequested);
}

} // namespace snappaste
