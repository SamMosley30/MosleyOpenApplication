#ifndef DAILYLEADERBOARDMODEL_H
#define DAILYLEADERBOARDMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QMap>
#include <QPair>
#include <QDebug> // For debugging

#include "CommonStructs.h"

// Structure to hold a row of calculated daily leaderboard data
struct DailyLeaderboardRow {
    int playerId;
    QString playerName;
    int dailyTotalPoints; // Total points for this day
    int dailyNetPoints;   // Net points for this day (Total - Handicap)
    int rank;             // Player's rank on this daily leaderboard
    // Add other calculated metrics here if needed (e.g., Daily Gross Score)
};

class DailyLeaderboardModel : public QAbstractTableModel
{
    Q_OBJECT // Required for signals and slots

public:
    // Constructor takes the database connection name and the day number
    explicit DailyLeaderboardModel(const QString &connectionName, int dayNum, QObject *parent = nullptr);
    ~DailyLeaderboardModel();

    // === Required QAbstractTableModel methods ===
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // === Public method to refresh data and recalculate leaderboard ===
    void refreshData();

    // Public method to get the day number this model represents
    int getDayNum() const { return m_dayNum; }

    // Helper to get the model column index for a specific data type
    int getColumnForRank() const { return 0; }
    int getColumnForPlayerName() const { return 1; }
    int getColumnForDailyTotalPoints() const { return 2; }
    int getColumnForDailyNetPoints() const { return 3; }

private:
    QString m_connectionName; // Stores the name of the database connection
    int m_dayNum;             // The day number this model represents (1, 2, or 3)
    QVector<DailyLeaderboardRow> m_leaderboardData; // Stores the calculated leaderboard rows

    // Internal data structures to hold raw data fetched from DB for calculations
    QMap<int, PlayerInfo> m_allPlayers; // Map<PlayerId, PlayerInfo> for all active players
    QMap<QPair<int, int>, QPair<int, int>> m_allHoleDetails; // Map<Pair<CourseId, HoleNum>, Pair<Par, Handicap>>
    // Stores scores for this day: Map<PlayerId, Map<HoleNum, Pair<Score, CourseId>>>
    QMap<int, QMap<int, QPair<int, int>>> m_dailyScores;

    // === Private helper methods ===
    QSqlDatabase database() const; // Helper to get database connection by name

    // Methods to fetch raw data from the database
    void fetchAllPlayers();
    void fetchAllHoleDetails();
    void fetchDailyScores(); // Fetches scores only for the specific day

    // Method to perform Stableford calculations and ranking for the day
    void calculateLeaderboard();

    // Helper to get DailyLeaderboardRow by row index in m_leaderboardData
    const DailyLeaderboardRow* getLeaderboardRow(int row) const;
    // Add helpers for other columns if added to DailyLeaderboardRow
};

#endif // DAILYLEADERBOARDMODEL_H
