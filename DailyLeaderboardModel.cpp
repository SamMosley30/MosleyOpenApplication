#include "dailyleaderboardmodel.h"
#include "utils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>
#include <algorithm> // For std::sort

// Constructor
DailyLeaderboardModel::DailyLeaderboardModel(const QString &connectionName, int dayNum, QObject *parent)
    : QAbstractTableModel(parent)
    , m_connectionName(connectionName)
    , m_dayNum(dayNum) // Store the day number
{
    // Data is loaded on demand via refreshData()
}

// Destructor
DailyLeaderboardModel::~DailyLeaderboardModel()
{
    // No dynamic allocation of model members needed
}

// Helper to get the database connection by name
QSqlDatabase DailyLeaderboardModel::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

// === Required QAbstractTableModel methods implementation ===

// Returns the number of rows (active players on the daily leaderboard)
int DailyLeaderboardModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_leaderboardData.size(); // One row per player in the calculated data
}

// Returns the number of columns for the daily leaderboard display
int DailyLeaderboardModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    // Columns: Rank (1), Player Name (1), Daily Total Points (1), Daily Net Points (1)
    return 4;
}

// Returns the data for a specific cell
QVariant DailyLeaderboardModel::data(const QModelIndex &index, int role) const
{
    // Check if the index is valid and within the model's bounds
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= columnCount())
    {
        return QVariant();
    }

    // Get the calculated leaderboard row for the current index
    const DailyLeaderboardRow* rowData = getLeaderboardRow(index.row());
    if (!rowData) {
        return QVariant(); // Should not happen if index is valid
    }

    // Handle DisplayRole
    if (role == Qt::DisplayRole)
    {
        if (index.column() == getColumnForRank()) return rowData->rank;
        if (index.column() == getColumnForPlayerName()) return rowData->playerName;
        if (index.column() == getColumnForDailyTotalPoints()) return rowData->dailyTotalPoints;
        if (index.column() == getColumnForDailyNetPoints()) return rowData->dailyNetPoints;
    }

    // Handle other roles like TextAlignmentRole
    if (role == Qt::TextAlignmentRole)
        return static_cast<int>(Qt::AlignCenter); // Center align numeric columns

    return QVariant(); // Return an invalid QVariant for roles we don't handle
}

// Returns the header data for rows and columns
QVariant DailyLeaderboardModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // We only handle DisplayRole for headers
    if (role == Qt::DisplayRole and orientation == Qt::Horizontal)
    {
        if (section == getColumnForRank()) return "Rank";
        if (section == getColumnForPlayerName()) return "Player";
        if (section == getColumnForDailyTotalPoints()) return QString("Day %1 Total").arg(m_dayNum);
        if (section == getColumnForDailyNetPoints()) return QString("Day %1 Net").arg(m_dayNum);
    }

    // Handle other roles like TextAlignmentRole for headers
    if (role == Qt::TextAlignmentRole and orientation == Qt::Horizontal)
            return static_cast<int>(Qt::AlignCenter); // Center align numeric column headers

    return QVariant(); // Return an invalid QVariant for roles/orientations we don't handle
}

// === Public method to refresh data and recalculate leaderboard ===
void DailyLeaderboardModel::refreshData()
{
    // Notify the view that the model's data is about to be reset
    beginResetModel();

    // Clear previous data
    m_allPlayers.clear();
    m_allHoleDetails.clear();
    m_dailyScores.clear();
    m_leaderboardData.clear(); // Clear calculated data

    // Fetch raw data from the database
    fetchAllPlayers();
    fetchAllHoleDetails();
    fetchDailyScores(); // Fetch scores only for this day

    // Perform calculations and ranking
    calculateLeaderboard();

    // Notify the view that the model's data has been reset
    endResetModel();
}

// === Private helper methods ===

// Fetches all active players (same as TournamentModel)
void DailyLeaderboardModel::fetchAllPlayers()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "DailyLeaderboardModel::fetchAllPlayers: ERROR: Invalid or closed database connection.";
        return;
    }

    QSqlQuery query(db);
    if (query.exec("SELECT id, name, handicap FROM players WHERE active = 1")) {
        while (query.next()) {
            PlayerInfo player;
            player.id = query.value("id").toInt();
            player.name = query.value("name").toString();
            player.handicap = query.value("handicap").toInt();
            m_allPlayers[player.id] = player;
        }
    } else {
        qDebug() << "DailyLeaderboardModel::fetchAllPlayers: ERROR executing query:" << query.lastError().text();
    }
}

// Fetches details (Par, Handicap) for all holes on all courses (same as TournamentModel)
void DailyLeaderboardModel::fetchAllHoleDetails()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "DailyLeaderboardModel::fetchAllHoleDetails: ERROR: Invalid or closed database connection.";
        return;
    }

    QSqlQuery query(db);
    if (query.exec("SELECT course_id, hole_num, par, handicap FROM holes ORDER BY course_id, hole_num")) {
        while (query.next()) {
            int courseId = query.value("course_id").toInt();
            int holeNum = query.value("hole_num").toInt();
            int par = query.value("par").toInt();
            int handicap = query.value("handicap").toInt();
            m_allHoleDetails[qMakePair(courseId, holeNum)] = qMakePair(par, handicap);
        }
    } else {
        qDebug() << "DailyLeaderboardModel::fetchAllHoleDetails: ERROR executing query:" << query.lastError().text();
    }
}

// Fetches scores for active players for the specific day of this model
void DailyLeaderboardModel::fetchDailyScores()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "DailyLeaderboardModel::fetchDailyScores: ERROR: Invalid or closed database connection.";
        return;
    }

    QStringList activePlayerIds;
    for (auto const& [playerId, playerInfo] : m_allPlayers.asKeyValueRange()) {
        activePlayerIds << QString::number(playerId);
    }

     if (activePlayerIds.isEmpty()) {
        qDebug() << "DailyLeaderboardModel::fetchDailyScores: No active players to fetch scores for.";
        return;
    }

    QSqlQuery query(db);
    // Select scores for active players for the specific day (m_dayNum)
    QString queryString = QString("SELECT player_id, course_id, hole_num, score FROM scores WHERE player_id IN (%1) AND day_num = %2 ORDER BY player_id, hole_num;")
                          .arg(activePlayerIds.join(",")).arg(m_dayNum);

    if (query.exec(queryString)) {
        while (query.next()) {
            int playerId = query.value("player_id").toInt();
            int courseId = query.value("course_id").toInt();
            int holeNum = query.value("hole_num").toInt();
            int score = query.value("score").toInt();

            // Store the score and course_id: m_dailyScores[playerId][holeNum] = Pair<Score, CourseId>
            m_dailyScores[playerId][holeNum] = qMakePair(score, courseId);
        }
    } else {
        qDebug() << "DailyLeaderboardModel::fetchDailyScores: ERROR executing query:" << query.lastError().text();
    }
}


// Method to perform Stableford calculations and ranking for the day
void DailyLeaderboardModel::calculateLeaderboard()
{
    m_leaderboardData.clear(); // Clear previous calculated data

    // Iterate through all active players
    for (auto const& [playerId, playerInfo] : m_allPlayers.asKeyValueRange()) {
        // Only include players who have scores for this specific day
        if (m_dailyScores.contains(playerId)) {
            DailyLeaderboardRow row;
            row.playerId = playerId;
            row.playerName = playerInfo.name;
            row.dailyTotalPoints = 0; // Initialize daily total
            row.dailyNetPoints = 0;   // Initialize daily net

            // Iterate through scores for this player for this day
            for (auto const& [holeNum, scoreAndCourse] : m_dailyScores[playerId].asKeyValueRange()) {
                int score = scoreAndCourse.first;
                int courseIdForScore = scoreAndCourse.second;

                // Find the par for this hole on this course
                if (m_allHoleDetails.contains(qMakePair(courseIdForScore, holeNum))) {
                    int par = m_allHoleDetails[qMakePair(courseIdForScore, holeNum)].first; // Get Par

                    // Calculate Stableford points for this hole
                    if (stableford_conversion.contains(score - par))
                        row.dailyTotalPoints += stableford_conversion.at(score - par);
                    else
                        qDebug() << "Invalid score " << score << "on par " << par << "for " << playerInfo.name;
                } else {
                     qDebug() << QString("DailyLeaderboardModel::calculateLeaderboard (Day %1): Warning: Hole details not found for Course %2 Hole %3").arg(m_dayNum).arg(courseIdForScore).arg(holeNum);
                }
            }

            // Calculate daily net points after summing daily total points
            row.dailyNetPoints = row.dailyTotalPoints - playerInfo.handicap;

            m_leaderboardData.append(row); // Add the calculated row to the list
        }
    }

    // --- Rank the players ---
    // Sort m_leaderboardData by dailyTotalPoints in descending order
    std::sort(m_leaderboardData.begin(), m_leaderboardData.end(),
              [](const DailyLeaderboardRow& a, const DailyLeaderboardRow& b) {
                  return a.dailyNetPoints > b.dailyNetPoints; // Sort descending by daily total
              });

    // Assign ranks based on the sorted order
    if (!m_leaderboardData.isEmpty()) {
        m_leaderboardData[0].rank = 1; // First player is rank 1
        for (int i = 1; i < m_leaderboardData.size(); ++i) {
            // Assign same rank if points are tied
            if (m_leaderboardData[i].dailyNetPoints == m_leaderboardData[i-1].dailyNetPoints) {
                m_leaderboardData[i].rank = m_leaderboardData[i-1].rank;
            } else {
                m_leaderboardData[i].rank = i + 1; // Otherwise, rank is based on position
            }
        }
    }
}

// Helper to get DailyLeaderboardRow by row index in m_leaderboardData
const DailyLeaderboardRow* DailyLeaderboardModel::getLeaderboardRow(int row) const
{
    if (row >= 0 && row < m_leaderboardData.size()) {
        return &m_leaderboardData.at(row);
    }
    return nullptr; // Invalid row
}
