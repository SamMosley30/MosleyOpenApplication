/**
 * @file DailyLeaderboardModel.h
 * @brief Contains the declaration of the DailyLeaderboardModel class.
 */

#ifndef DAILYLEADERBOARDMODEL_H
#define DAILYLEADERBOARDMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QMap>
#include <QPair>
#include <QDebug>

#include "CommonStructs.h"

/**
 * @struct DailyLeaderboardRow
 * @brief Holds a row of calculated daily leaderboard data.
 */
struct DailyLeaderboardRow {
    int playerId;           ///< The unique identifier for the player.
    QString playerName;     ///< The name of the player.
    int dailyTotalPoints;   ///< Total Stableford points for the day.
    int dailyNetPoints;     ///< Net Stableford points for the day (Total - Handicap).
    int rank;               ///< The player's rank on the daily leaderboard.
};

/**
 * @class DailyLeaderboardModel
 * @brief A model for calculating and displaying a daily leaderboard.
 *
 * This model fetches player scores for a specific day, calculates Stableford points,
 * ranks the players, and provides the data to a view.
 */
class DailyLeaderboardModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a DailyLeaderboardModel object.
     * @param connectionName The name of the database connection to use.
     * @param dayNum The day number (1, 2, or 3) this model represents.
     * @param parent The parent object.
     */
    explicit DailyLeaderboardModel(const QString &connectionName, int dayNum, QObject *parent = nullptr);

    /**
     * @brief Destroys the DailyLeaderboardModel object.
     */
    ~DailyLeaderboardModel();

    // QAbstractTableModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Refreshes the data and recalculates the leaderboard.
     *
     * This method fetches the latest data from the database and recalculates the
     * entire leaderboard.
     */
    void refreshData();

    /**
     * @brief Gets the day number this model represents.
     * @return The day number.
     */
    int getDayNum() const { return m_dayNum; }

    /** @brief Gets the column index for the rank. */
    int getColumnForRank() const { return 0; }
    /** @brief Gets the column index for the player name. */
    int getColumnForPlayerName() const { return 1; }
    /** @brief Gets the column index for the daily total points. */
    int getColumnForDailyTotalPoints() const { return 2; }
    /** @brief Gets the column index for the daily net points. */
    int getColumnForDailyNetPoints() const { return 3; }

private:
    QString m_connectionName; ///< The name of the database connection.
    int m_dayNum;             ///< The day number this model represents (1, 2, or 3).
    QVector<DailyLeaderboardRow> m_leaderboardData; ///< Stores the calculated leaderboard rows.

    // Internal data structures for calculations
    QMap<int, PlayerInfo> m_allPlayers; ///< Map of PlayerId to PlayerInfo for all active players.
    QMap<QPair<int, int>, QPair<int, int>> m_allHoleDetails; ///< Map of <CourseId, HoleNum> to <Par, Handicap>.
    QMap<int, QMap<int, QPair<int, int>>> m_dailyScores; ///< Map of PlayerId to a map of <HoleNum, <Score, CourseId>>.

    /**
     * @brief Gets the database connection by name.
     * @return The QSqlDatabase object.
     */
    QSqlDatabase database() const;

    // Private helper methods for data fetching and calculation
    void fetchAllPlayers();
    void fetchAllHoleDetails();
    void fetchDailyScores();
    void calculateLeaderboard();

    /**
     * @brief Gets the leaderboard row data for a given row index.
     * @param row The row index.
     * @return A pointer to the DailyLeaderboardRow, or nullptr if the index is invalid.
     */
    const DailyLeaderboardRow* getLeaderboardRow(int row) const;
};

#endif // DAILYLEADERBOARDMODEL_H
