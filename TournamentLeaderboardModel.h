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

struct LeaderboardRow {
    int playerId;
    QString playerName;
    int playerActualDbHandicap; // Player's actual handicap from the database
    int totalNetStablefordPoints; // Overall net Stableford points for THIS specific leaderboard context
    int rank;                  

    QMap<int, int> dailyGrossStablefordPoints; // Gross points for the day
    QMap<int, int> dailyNetStablefordPoints;   // Net points for the day (calculated based on context)
    
    // This is the 2-day total calculated using ONLY MOSLEY OPEN handicap rules, used for cut decision.
    // It might not be directly displayed in every leaderboard row but is essential for filtering.
    int twoDayMosleyNetScoreForCut; 
};

class TournamentLeaderboardModel : public QAbstractTableModel
{
    Q_OBJECT 

public:
    enum TournamentContext {
        MosleyOpen,  // Calculates scores with Mosley handicap rules
        TwistedCreek // Calculates scores with actual handicaps
        // Removed 'Combined' as per new requirements
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
    void setCutLineScore(int score);
    void setIsCutApplied(bool applied);
    
    int getColumnForDailyGrossPoints(int dayNum) const; 
    int getColumnForDailyNetPoints(int dayNum) const;   

private:
    QString m_connectionName; 
    QVector<LeaderboardRow> m_leaderboardData; 
    QSet<int> m_daysWithScores; 

    TournamentContext m_tournamentContext; // Is this model for Mosley or Twisted Creek?
    int m_cutLineScore;
    bool m_isCutApplied;

    QMap<int, PlayerInfo> m_allPlayers; 
    QMap<QPair<int, int>, QPair<int, int>> m_holeParAndHandicapIndex; // Key: <CourseId, HoleNum>, Value: <Par, HoleIndex>
    QMap<int, QMap<int, QMap<int, QPair<int, int>>>> m_allScores; // PlayerId -> DayNum -> HoleNum -> <Score, CourseId>
    
    // Stores the 2-day net Stableford score for each player, calculated *using Mosley Open handicap rules*.
    // This is used for the cut decision.
    QMap<int, int> m_playerTwoDayMosleyNetScoreForCut; 

    QSqlDatabase database() const; 

    void fetchAllPlayers();
    void fetchAllHoleDetails(); 
    void fetchAllScores();
    void calculateAllPlayerTwoDayMosleyNetScores(); // Calculates m_playerTwoDayMosleyNetScoreForCut
    void calculateLeaderboard(); 

    int calculateNetStablefordPointsForHole(
        int grossScore, 
        int par, 
        int playerActualDatabaseHandicap, 
        int holeHandicapIndex,
        TournamentContext contextForCalc // Which handicap rule to apply for THIS calculation
    ) const;
};

#endif // TOURNAMENTLEADERBOARDMODEL_H
