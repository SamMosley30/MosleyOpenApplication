#ifndef DAILYLEADERBOARDWIDGET_H
#define DAILYLEADERBOARDWIDGET_H

#include <QWidget>
#include <QSqlDatabase>
#include <QTableView>
#include <QVBoxLayout>
#include <QHeaderView>

#include "dailyleaderboardmodel.h" // Include the model header

// Include QPainter related headers for the export functionality
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>


class DailyLeaderboardWidget : public QWidget
{
    Q_OBJECT // Required for signals and slots

public:
    // Constructor takes the database connection name and the day number
    explicit DailyLeaderboardWidget(const QString &connectionName, int dayNum, QWidget *parent = nullptr);
    ~DailyLeaderboardWidget();

    // Public method to refresh the leaderboard data
    void refreshData();

    // Public method to export the leaderboard as an image
    QImage exportToImage() const;

private:
    QString m_connectionName; // Stores the database connection name
    int m_dayNum;             // The day number this widget represents

    DailyLeaderboardModel *leaderboardModel; // The model for this leaderboard
    QTableView *leaderboardView; // The view to display this leaderboard

    // Helper method to get database connection
    QSqlDatabase database() const;

    // Helper to configure the table view settings
    void configureTableView();
};

#endif // DAILYLEADERBOARDWIDGET_H
