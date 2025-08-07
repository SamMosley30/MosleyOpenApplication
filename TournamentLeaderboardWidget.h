/**
 * @file TournamentLeaderboardWidget.h
 * @brief Contains the declaration of the TournamentLeaderboardWidget class.
 */

#ifndef TOURNAMENTLEADERBOARDWIDGET_H
#define TOURNAMENTLEADERBOARDWIDGET_H

#include <QWidget>
#include <QSqlDatabase>
#include <QTableView>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QSet>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

#include "tournamentleaderboardmodel.h"

/**
 * @class TournamentLeaderboardWidget
 * @brief A widget for displaying a tournament leaderboard.
 *
 * This widget contains a table view that displays the data from a
 * TournamentLeaderboardModel. It also provides functionality to refresh the data
 * and export the leaderboard as an image.
 */
class TournamentLeaderboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TournamentLeaderboardWidget(const QString &connectionName, QWidget *parent = nullptr);
    ~TournamentLeaderboardWidget();

    void refreshData();
    QImage exportToImage() const;
    TournamentLeaderboardModel *leaderboardModel;

private:
    QString m_connectionName;
    QTableView *leaderboardView;

    QSqlDatabase database() const;
    void configureTableView();
    void updateColumnVisibility();
};

#endif // TOURNAMENTLEADERBOARDWIDGET_H