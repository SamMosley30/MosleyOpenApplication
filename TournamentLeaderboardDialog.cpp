#include "tournamentleaderboarddialog.h"
// Include headers for the new custom widgets
#include "tournamentleaderboardwidget.h"
#include "dailyleaderboardwidget.h"
#include "teamleaderboardwidget.h" // Include header for TeamLeaderboardWidget

#include <QSqlDatabase>
#include <QDebug>
#include <QSet>
#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QAbstractTableModel> // Include for qobject_cast


// Constructor
TournamentLeaderboardDialog::TournamentLeaderboardDialog(const QString &connectionName, QWidget *parent)
    : QDialog(parent)
    , m_connectionName(connectionName)
    , tabWidget(new QTabWidget(this)) // Initialize the tab widget

    // Initialize custom widgets (Daily and Team widgets will be implemented later)
    , tournamentLeaderboardWidget(new TournamentLeaderboardWidget(m_connectionName, this))
    , day1LeaderboardWidget(new DailyLeaderboardWidget(m_connectionName, 1, this)) // Pass day number 1
    , day2LeaderboardWidget(new DailyLeaderboardWidget(m_connectionName, 2, this)) // Pass day number 2
    , day3LeaderboardWidget(new DailyLeaderboardWidget(m_connectionName, 3, this)) // Pass day number 3
    , teamLeaderboardWidget(new TeamLeaderboardWidget(m_connectionName, this))     // Team widget

    // Initialize buttons
    , refreshButton(new QPushButton(tr("Refresh All"), this)) // Refresh button refreshes all tabs
    , closeButton(new QPushButton(tr("Close"), this))
    , exportImageButton(new QPushButton(tr("Export Current Tab"), this)) // Export button exports current tab
{
    // --- Setup Database Connection ---
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
         qDebug() << "TournamentLeaderboardDialog: ERROR: Invalid or closed database connection passed to constructor.";
         refreshButton->setEnabled(false);
         exportImageButton->setEnabled(false);
         // Consider showing a message box here as well
    }

    // --- Setup Tab Widget ---
    // Add the custom widgets as tabs
    tabWidget->addTab(tournamentLeaderboardWidget, tr("Overall Tournament"));
    tabWidget->addTab(day1LeaderboardWidget, tr("Day 1 Leaderboard"));
    tabWidget->addTab(day2LeaderboardWidget, tr("Day 2 Leaderboard"));
    tabWidget->addTab(day3LeaderboardWidget, tr("Day 3 Leaderboard"));
    tabWidget->addTab(teamLeaderboardWidget, tr("Team Leaderboard")); // Add team tab


    // --- Setup Layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget); // Add the tab widget

    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(exportImageButton);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    setWindowTitle(tr("Tournament Leaderboards")); // Update window title
    resize(900, 600); // Adjust default size


    // --- Connect Signals and Slots ---
    connect(refreshButton, &QPushButton::clicked, this, &TournamentLeaderboardDialog::refreshLeaderboards);
    connect(exportImageButton, &QPushButton::clicked, this, &TournamentLeaderboardDialog::exportCurrentImage); // Connect to export current tab
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    // Connect tab changes to potentially update export button text or logic
    // connect(tabWidget, &QTabWidget::currentChanged, this, &TournamentLeaderboardDialog::updateExportButton);


    // --- Initial Data Load ---
    // Refresh all leaderboards when the dialog is first created/shown
    refreshLeaderboards();
}

// Destructor
TournamentLeaderboardDialog::~TournamentLeaderboardDialog()
{
    // Widgets are parented to 'this', so they will be deleted automatically.
}

// Helper to get database connection
QSqlDatabase TournamentLeaderboardDialog::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

// Helper to configure common table view settings
// This helper is no longer needed in the dialog as configuration is in the widgets
/*
void TournamentLeaderboardDialog::configureTableView(QTableView *view)
{
    if (!view) return;
    view->verticalHeader()->setVisible(false); // Hide vertical header (row numbers)
    view->setSelectionBehavior(QAbstractItemView::SelectRows); // Select entire rows
    view->setSelectionMode(QAbstractItemView::NoSelection);    // Make view read-only (no selection)
    view->setEditTriggers(QAbstractItemView::NoEditTriggers); // Make view non-editable
    view->horizontalHeader()->setStretchLastSection(true); // Stretch the last section
}
*/


// Slot to refresh all leaderboard data
void TournamentLeaderboardDialog::refreshLeaderboards()
{
    qDebug() << "TournamentLeaderboardDialog: Refreshing all leaderboards...";
    // Call refreshData() on each of the custom widgets
    tournamentLeaderboardWidget->refreshData();
    day1LeaderboardWidget->refreshData();
    day2LeaderboardWidget->refreshData();
    day3LeaderboardWidget->refreshData();
    teamLeaderboardWidget->refreshData(); // Call refresh for team widget (will be implemented)

    // Column visibility is now handled within the TournamentLeaderboardWidget after its refresh
    // updateDailyLeaderboardColumnVisibility(); // Not needed here anymore

    qDebug() << "TournamentLeaderboardDialog: All leaderboards refreshed.";
}

// Helper to update column visibility for daily leaderboards based on scores
// This logic is now primarily in the TournamentLeaderboardWidget
/*
void TournamentLeaderboardDialog::updateDailyLeaderboardColumnVisibility()
{
    // This logic is now handled within the TournamentLeaderboardWidget
}
*/


// Slot to export the currently visible leaderboard tab as an image
void TournamentLeaderboardDialog::exportCurrentImage()
{
    // Get the currently visible widget
    QWidget *currentWidget = tabWidget->currentWidget();

    // Try to cast the current widget to a known leaderboard widget type
    QImage exportedImage;

    if (TournamentLeaderboardWidget *overallWidget = qobject_cast<TournamentLeaderboardWidget*>(currentWidget)) {
        exportedImage = overallWidget->exportToImage();
    } else if (DailyLeaderboardWidget *dailyWidget = qobject_cast<DailyLeaderboardWidget*>(currentWidget)) {
        exportedImage = dailyWidget->exportToImage(); // Call export on daily widget
    } else if (TeamLeaderboardWidget *teamWidget = qobject_cast<TeamLeaderboardWidget*>(currentWidget)) {
        exportedImage = teamWidget->exportToImage(); // Call export on team widget (will implement)
    } else {
        QMessageBox::warning(this, tr("Export Failed"), tr("Cannot export the current tab type."));
        return;
    }


    // Check if the export was successful (returned a valid image)
    if (exportedImage.isNull()) {
         // The exportToImage method in the widget should ideally show an error message
         // if it fails internally. This check is a fallback.
         qDebug() << "TournamentLeaderboardDialog::exportCurrentImage: ExportToImage returned a null image.";
         // QMessageBox::critical(this, tr("Export Failed"), tr("Failed to generate image for export."));
         return;
    }


    // Get the file path from the user
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    tr("Save Leaderboard Image"),
                                                    QDir::homePath(),
                                                    tr("PNG Files (*.png);;JPEG Files (*.jpg *.jpeg);;BMP Files (*.bmp)")
                                                    );

    // Check if the user cancelled the dialog
    if (filePath.isEmpty()) {
        return;
    }

    // Save the created image
    if (exportedImage.save(filePath)) {
        QMessageBox::information(this, tr("Export Successful"), tr("Leaderboard image saved to:\n%1").arg(QDir::toNativeSeparators(filePath)));
    } else {
        QMessageBox::critical(this, tr("Export Failed"), tr("Could not save image to:\n%1").arg(QDir::toNativeSeparators(filePath)));
    }
}