#ifndef TOURNAMENTLEADERBOARDDIALOG_H
#define TOURNAMENTLEADERBOARDDIALOG_H

#include <QDialog>
#include <QSqlDatabase> // To receive the database connection name
#include <QPushButton>
#include <QVBoxLayout> // For layout management
#include <QHBoxLayout> // For layout management
#include <QTabWidget>  // Include QTabWidget for multiple leaderboards

// Include the new custom widget headers
#include "tournamentleaderboardwidget.h"
#include "dailyleaderboardwidget.h"
// Forward declare other leaderboard widgets we will create
//class TeamLeaderboardWidget;

// Include QPainter related headers for the export functionality (now in the widget)
// #include <QPainter>
// #include <QPixmap>
// #include <QImage>
#include <QFileDialog> // Still needed for the dialog to get the file path
#include <QMessageBox> // Still needed for messages
#include <QDir>     // Still needed for paths
// #include <QSet>     // Now used in the model/widget


class TournamentLeaderboardDialog : public QDialog
{
    Q_OBJECT // Required for signals and slots

public:
    // Constructor takes the database connection name
    explicit TournamentLeaderboardDialog(const QString &connectionName, QWidget *parent = nullptr);
    ~TournamentLeaderboardDialog();

public slots:
    // Slot to refresh all leaderboard data
    void refreshLeaderboards();

private slots:
    // Slot to export the currently visible leaderboard tab as an image
    void exportCurrentImage();

private:
    QString m_connectionName; // Stores the database connection name

    QTabWidget *tabWidget; // The main tab widget for different leaderboards

    // Custom widgets for different leaderboards
    TournamentLeaderboardWidget *tournamentLeaderboardWidget; // Overall tournament widget
    DailyLeaderboardWidget *day1LeaderboardWidget;            // Day 1 widget
    DailyLeaderboardWidget *day2LeaderboardWidget;            // Day 2 widget
    DailyLeaderboardWidget *day3LeaderboardWidget;            // Day 3 widget
    //TeamLeaderboardWidget *teamLeaderboardWidget;             // Team widget (will be implemented later)

    QPushButton *refreshButton; // Button to refresh all leaderboards
    QPushButton *closeButton;   // Button to close the dialog
    QPushButton *exportImageButton; // Button to export the current image

    // Helper method to get database connection
    QSqlDatabase database() const;

    // Helper to configure common table view settings
    // This helper is no longer needed in the dialog as configuration is in the widgets
    // void configureTableView(QTableView *view);

    // Helper to update column visibility for daily leaderboards based on scores
    // This logic is now primarily in the TournamentLeaderboardWidget
    // void updateDailyLeaderboardColumnVisibility();
};

#endif // TOURNAMENTLEADERBOARDDIALOG_H