#ifndef TOURNAMENTLEADERBOARDMODEL_H
#define TOURNAMENTLEADERBOARDMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QMap>
#include <QPair>
#include <QDebug> // For debugging

#include "CommonStructs.h"

// Structure to hold a row of calculated leaderboard data
struct LeaderboardRow {
    int playerId;
    QString playerName;
    int playerHandicap;
    int totalStablefordPoints; // Total points including handicap
    int rank;                  // Player's rank on the leaderboard

    // Daily points
    QMap<int, int> dailyTotalPoints; // Map<DayNum, TotalPointsForThatDay>
    QMap<int, int> dailyNetPoints;   // Map<DayNum, NetPointsForThatDay (Total - Handicap)>

    // Add other calculated metrics here if needed (e.g., Total Gross Score, Total Net Score)
};

class TournamentLeaderboardModel : public QAbstractTableModel
{
    Q_OBJECT // Required for signals and slots

public:
    // Constructor takes the database connection name
    explicit TournamentLeaderboardModel(const QString &connectionName, QObject *parent = nullptr);
    ~TournamentLeaderboardModel();

    // === Required QAbstractTableModel methods ===
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // === Public method to refresh data and recalculate leaderboard ===
    void refreshData();

    // === Public method to get days with scores ===
    QSet<int> getDaysWithScores() const;

    // Helper to get the column index for a specific daily points column
    int getColumnForDailyTotalPoints(int dayNum) const;
    int getColumnForDailyNetPoints(int dayNum) const;
    
    // Helper to get the day number from a daily points column index
    int getDayNumForColumn(int column) const;

    // Helper to get PlayerInfo by row index in m_leaderboardData
    const LeaderboardRow* getLeaderboardRow(int row) const;

private:
    QString m_connectionName; // Stores the name of the database connection
    QVector<LeaderboardRow> m_leaderboardData; // Stores the calculated leaderboard rows
    QSet<int> m_daysWithScores; // Stores the set of day numbers (1, 2, 3) that have scores

    // Internal data structures to hold raw data fetched from DB for calculations
    QMap<int, PlayerInfo> m_allPlayers; // Map<PlayerId, PlayerInfo> for all active players
    QMap<QPair<int, int>, QPair<int, int>> m_allHoleDetails; // Map<Pair<CourseId, HoleNum>, Pair<Par, Handicap>>
    QMap<int, QMap<int, QMap<int, QPair<int, int>>>> m_allScores; // Map<PlayerId, Map<DayNum, Map<HoleNum, Score>>>

    // === Private helper methods ===
    QSqlDatabase database() const; // Helper to get database connection by name

    // Methods to fetch raw data from the database
    void fetchAllPlayers();
    void fetchAllHoleDetails();
    void fetchAllScores();

    // Method to perform Stableford calculations and ranking
    void calculateLeaderboard();

    // Helper to calculate Stableford points for a single hole score
    int calculateStablefordPoints(int score, int par) const;
};

#endif // TOURNAMENTLEADERBOARDMODEL_H