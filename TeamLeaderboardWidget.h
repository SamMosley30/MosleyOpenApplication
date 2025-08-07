/**
 * @file TeamLeaderboardWidget.h
 * @brief Contains the declaration of the TeamLeaderboardWidget class.
 */

#ifndef TEAMLEADERBOARDWIDGET_H
#define TEAMLEADERBOARDWIDGET_H

#include <QWidget>
#include <QSqlDatabase>
#include <QTableView>
#include <QVBoxLayout>
#include <QHeaderView>
#include "TeamLeaderboardModel.h"

class QPainter;
class QImage;

/**
 * @class TeamLeaderboardWidget
 * @brief A widget for displaying a team leaderboard.
 *
 * This widget contains a table view that displays the data from a
 * TeamLeaderboardModel. It also provides functionality to refresh the data
 * and export the leaderboard as an image.
 */
class TeamLeaderboardWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs a TeamLeaderboardWidget object.
     * @param connectionName The name of the database connection to use.
     * @param parent The parent widget.
     */
    explicit TeamLeaderboardWidget(const QString &connectionName, QWidget *parent = nullptr);

    /**
     * @brief Destroys the TeamLeaderboardWidget object.
     */
    ~TeamLeaderboardWidget();

    /**
     * @brief Refreshes the leaderboard data.
     */
    void refreshData();

    /**
     * @brief Exports the leaderboard as an image.
     * @return The leaderboard rendered as a QImage.
     */
    QImage exportToImage() const;

private:
    QString m_connectionName;
    TeamLeaderboardModel *leaderboardModel;
    QTableView *leaderboardView;

    QSqlDatabase database() const;
    void configureTableView();
    void updateColumnVisibility();
};

#endif // TEAMLEADERBOARDWIDGET_H
