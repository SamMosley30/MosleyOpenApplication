/**
 * @file DailyLeaderboardWidget.h
 * @brief Contains the declaration of the DailyLeaderboardWidget class.
 */

#ifndef DAILYLEADERBOARDWIDGET_H
#define DAILYLEADERBOARDWIDGET_H

#include <QWidget>
#include <QSqlDatabase>
#include <QTableView>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

#include "dailyleaderboardmodel.h"

/**
 * @class DailyLeaderboardWidget
 * @brief A widget for displaying a daily leaderboard.
 *
 * This widget contains a table view that displays the data from a
 * DailyLeaderboardModel. It also provides functionality to refresh the data
 * and export the leaderboard as an image.
 */
class DailyLeaderboardWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a DailyLeaderboardWidget object.
     * @param connectionName The name of the database connection to use.
     * @param dayNum The day number (1, 2, or 3) this widget represents.
     * @param parent The parent widget.
     */
    explicit DailyLeaderboardWidget(const QString &connectionName, int dayNum, QWidget *parent = nullptr);

    /**
     * @brief Destroys the DailyLeaderboardWidget object.
     */
    ~DailyLeaderboardWidget();

    /**
     * @brief Refreshes the leaderboard data.
     *
     * This method calls the model's refreshData() method to update the leaderboard.
     */
    void refreshData();

    /**
     * @brief Exports the leaderboard as an image.
     * @return The leaderboard rendered as a QImage.
     */
    QImage exportToImage() const;

private:
    QString m_connectionName; ///< The name of the database connection.
    int m_dayNum;             ///< The day number this widget represents.

    DailyLeaderboardModel *leaderboardModel; ///< The model for this leaderboard.
    QTableView *leaderboardView; ///< The view to display this leaderboard.

    /**
     * @brief Gets the database connection by name.
     * @return The QSqlDatabase object.
     */
    QSqlDatabase database() const;

    /**
     * @brief Configures the settings for the table view.
     */
    void configureTableView();
};

#endif // DAILYLEADERBOARDWIDGET_H
