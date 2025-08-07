/**
 * @file TournamentLeaderboardModel.h
 * @brief Contains the declaration of the TournamentLeaderboardModel class.
 */

#ifndef TOURNAMENTLEADERBOARDMODEL_H
#define TOURNAMENTLEADERBOARDMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QMap>
#include <QPair>
#include <QDebug>
#include <QSet>

#include "CommonStructs.h"

/**
 * @struct LeaderboardRow
 * @brief Holds calculated data for a single player on the leaderboard.
 */
struct LeaderboardRow {
    int playerId;
    QString playerName;
    int playerActualDbHandicap;
    int totalNetStablefordPoints;
    int rank;
    QMap<int, int> dailyGrossStablefordPoints;
    QMap<int, int> dailyNetStablefordPoints;
    int twoDayMosleyNetScoreForCut;
};

/**
 * @class TournamentLeaderboardModel
 * @brief A model for calculating and displaying tournament leaderboards.
 *
 * This model can calculate leaderboards for different tournament contexts,
 * such as the Mosley Open or Twisted Creek, and can apply a cut line.
 */
class TournamentLeaderboardModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /**
     * @enum TournamentContext
     * @brief Defines the context for leaderboard calculations.
     */
    enum TournamentContext {
        MosleyOpen,
        TwistedCreek
    };

    explicit TournamentLeaderboardModel(const QString &connectionName, QObject *parent = nullptr);
    ~TournamentLeaderboardModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void refreshData();
    QSet<int> getDaysWithScores() const;

    void setTournamentContext(TournamentContext context);
    TournamentContext getTournamentContext() const { return m_tournamentContext; }
    void setCutLineScore(int score);
    void setIsCutApplied(bool applied);

    int getColumnForDailyGrossPoints(int dayNum) const;
    int getColumnForDailyNetPoints(int dayNum) const;

private:
    QString m_connectionName;
    QVector<LeaderboardRow> m_leaderboardData;
    QSet<int> m_daysWithScores;

    TournamentContext m_tournamentContext;
    int m_cutLineScore;
    bool m_isCutApplied;

    QMap<int, PlayerInfo> m_allPlayers;
    QMap<QPair<int, int>, QPair<int, int>> m_holeParAndHandicapIndex;
    QMap<int, QMap<int, QMap<int, QPair<int, int>>>> m_allScores;
    QMap<int, int> m_playerTwoDayMosleyNetScoreForCut;

    QSqlDatabase database() const;

    void fetchAllPlayers();
    void fetchAllHoleDetails();
    void fetchAllScores();
    void calculateAllPlayerTwoDayMosleyNetScores();
    void calculateLeaderboard();

    int calculateNetStablefordPointsForHole(int grossScore, int par) const;
};

#endif // TOURNAMENTLEADERBOARDMODEL_H
