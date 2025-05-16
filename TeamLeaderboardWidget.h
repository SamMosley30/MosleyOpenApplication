#ifndef TEAMLEADERBOARDWIDGET_H
#define TEAMLEADERBOARDWIDGET_H

#include <QWidget>
#include <QSqlDatabase>
#include <QTableView>
#include <QVBoxLayout>
#include <QHeaderView>
#include "TeamLeaderboardModel.h" // Include the model header

// Forward declarations for QPainter related classes if used for export
class QPainter;
class QImage;

class TeamLeaderboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit TeamLeaderboardWidget(const QString &connectionName, QWidget *parent = nullptr);
    ~TeamLeaderboardWidget();

    void refreshData();
    QImage exportToImage() const; // Similar to other leaderboard widgets

private:
    QString m_connectionName;
    TeamLeaderboardModel *leaderboardModel;
    QTableView *leaderboardView;

    QSqlDatabase database() const;
    void configureTableView();
    void updateColumnVisibility(); // If daily columns need to be hidden
};

#endif // TEAMLEADERBOARDWIDGET_H
