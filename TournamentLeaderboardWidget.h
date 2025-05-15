#ifndef TOURNAMENTLEADERBOARDWIDGET_H
#define TOURNAMENTLEADERBOARDWIDGET_H

#include <QWidget>
#include <QSqlDatabase>
#include <QTableView>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QSet>

#include "tournamentleaderboardmodel.h" // Include the model header

// Include QPainter related headers for the export functionality
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>


class TournamentLeaderboardWidget : public QWidget
{
    Q_OBJECT // Required for signals and slots

public:
    // Constructor takes the database connection name
    explicit TournamentLeaderboardWidget(const QString &connectionName, QWidget *parent = nullptr);
    ~TournamentLeaderboardWidget();

    // Public method to refresh the leaderboard data
    void refreshData();

    // Public method to export the leaderboard as an image
    QImage exportToImage() const;

private:
    QString m_connectionName; // Stores the database connection name

    TournamentLeaderboardModel *leaderboardModel; // The model for this leaderboard
    QTableView *leaderboardView; // The view to display this leaderboard

    // Helper method to get database connection
    QSqlDatabase database() const;

    // Helper to configure the table view settings
    void configureTableView();

    // Helper to update column visibility based on scores (for daily columns in overall)
    void updateColumnVisibility();
};

#endif // TOURNAMENTLEADERBOARDWIDGET_H