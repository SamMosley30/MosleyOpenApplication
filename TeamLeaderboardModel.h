#ifndef TEAMLEADERBOARDMODEL_H
#define TEAMLEADERBOARDMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QMap>
#include <QSet>
#include <QDebug>
#include "CommonStructs.h" // For PlayerInfo

// Structure to hold calculated data for each team on the leaderboard
struct TeamLeaderboardRow {
    int teamId;                 // 1, 2, 3, or 4
    QString teamName;           // e.g., "Team Jake"
    int rank;
    QMap<int, int> dailyTeamStablefordPoints; // Map<DayNum, TotalPointsForTeamOnThatDay>
    int overallTeamStablefordPoints;
    QVector<PlayerInfo> teamMembers; // Store players in this team
};

class TeamLeaderboardModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit TeamLeaderboardModel(const QString &connectionName, QObject *parent = nullptr);
    ~TeamLeaderboardModel();

    // Required QAbstractTableModel methods
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Public method to refresh data
    void refreshData();
    QSet<int> getDaysWithScores() const; // To know which daily columns to show

private:
    QString m_connectionName;
    QVector<TeamLeaderboardRow> m_leaderboardData; // Stores calculated rows for 4 teams
    QSet<int> m_daysWithScores; // Days (1,2,3) that have any scores recorded

    // Raw data fetched from DB
    QMap<int, PlayerInfo> m_allPlayers; // Map<PlayerId, PlayerInfo> (includes name and handicap)
    QMap<int, int> m_playerTeamAssignments; // Map<PlayerId, TeamId>
    QMap<QPair<int, int>, QPair<int, int>> m_allHoleDetails; // Map<Pair<CourseId, HoleNum>, Pair<Par, Handicap>>
    // Map<PlayerId, Map<DayNum, Map<HoleNum, ScoreInfo(score, course_id)>>>
    QMap<int, QMap<int, QMap<int, QPair<int, int>>>> m_allScores; 

    // Helper methods
    QSqlDatabase database() const;
    void fetchAllPlayersAndAssignments(); // Will now also help populate teamMembers
    void fetchAllHoleDetails();
    void fetchAllScores();
    void determineTeamNames(); // New method to set team names based on captain
    std::optional<int> getPlayerNetStablefordForHole(const PlayerInfo& member, int dayNum, int holeNum) const;
    int calculateTeamScoreForHole(TeamLeaderboardRow & teamRow, int dayNum, int holeNum) const;
    void calculateTeamLeaderboard();
    int calculateStablefordPoints(int score, int par) const; // Assuming this is the basic one, not the complex per-player one
};

#endif // TEAMLEADERBOARDMODEL_H
