/**
 * @file TeamLeaderboardModel.h
 * @brief Contains the declaration of the TeamLeaderboardModel class.
 */

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
#include "CommonStructs.h"

/**
 * @struct TeamLeaderboardRow
 * @brief Holds calculated data for each team on the leaderboard.
 */
struct TeamLeaderboardRow {
    int teamId;                             ///< The unique identifier for the team.
    QString teamName;                       ///< The name of the team.
    int rank;                               ///< The rank of the team.
    QMap<int, int> dailyTeamStablefordPoints; ///< Map of DayNum to total points for the team on that day.
    int overallTeamStablefordPoints;        ///< The overall total Stableford points for the team.
    QVector<PlayerInfo> teamMembers;        ///< The players who are members of the team.
};

/**
 * @class TeamLeaderboardModel
 * @brief A model for calculating and displaying a team leaderboard.
 *
 * This model calculates team scores based on the individual scores of team members
 * and provides the data to a view.
 */
class TeamLeaderboardModel : public QAbstractTableModel {
    Q_OBJECT

public:
    /**
     * @brief Constructs a TeamLeaderboardModel object.
     * @param connectionName The name of the database connection to use.
     * @param parent The parent object.
     */
    explicit TeamLeaderboardModel(const QString &connectionName, QObject *parent = nullptr);

    /**
     * @brief Destroys the TeamLeaderboardModel object.
     */
    ~TeamLeaderboardModel();

    // QAbstractTableModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Refreshes the data and recalculates the leaderboard.
     */
    void refreshData();

    /**
     * @brief Gets the set of days that have scores recorded.
     * @return A QSet of day numbers.
     */
    QSet<int> getDaysWithScores() const;

private:
    QString m_connectionName;
    QVector<TeamLeaderboardRow> m_leaderboardData;
    QSet<int> m_daysWithScores;

    QMap<int, PlayerInfo> m_allPlayers;
    QMap<int, int> m_playerTeamAssignments;
    QMap<QPair<int, int>, QPair<int, int>> m_allHoleDetails;
    QMap<int, QMap<int, QMap<int, QPair<int, int>>>> m_allScores;

    QSqlDatabase database() const;
    void fetchAllPlayersAndAssignments();
    void fetchAllHoleDetails();
    void fetchAllScores();
    std::optional<int> getPlayerNetStablefordForHole(const PlayerInfo& member, int dayNum, int holeNum) const;
    int calculateTeamScoreForHole(TeamLeaderboardRow & teamRow, int dayNum, int holeNum, int numScoresToTake) const;
    void calculateTeamLeaderboard();
};

#endif // TEAMLEADERBOARDMODEL_H
