#include "TournamentLeaderboardModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>
#include <algorithm> // For std::sort

// Constructor
TournamentLeaderboardModel::TournamentLeaderboardModel(const QString &connectionName, QObject *parent)
    : QAbstractTableModel(parent)
    , m_connectionName(connectionName)
{
    // Data is loaded on demand via refreshData()
}

// Destructor
TournamentLeaderboardModel::~TournamentLeaderboardModel()
{
    // No dynamic allocation of model members needed
}

// Helper to get the database connection by name
QSqlDatabase TournamentLeaderboardModel::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

// === Required QAbstractTableModel methods implementation ===

// Returns the number of rows (active players on the leaderboard)
int TournamentLeaderboardModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_leaderboardData.size(); // One row per player in the calculated data
}

// Returns the number of columns for the leaderboard display
int TournamentLeaderboardModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    // Columns:
    // Rank (1)
    // Player Name (1)
    // Handicap (1)
    // Day 1 Total Points (1), Day 1 Net Points (1)
    // Day 2 Total Points (1), Day 2 Net Points (1)
    // Day 3 Total Points (1), Day 3 Net Points (1)
    // Overall Net Stableford Points (1)
    // Total = 1 + 1 + 1 + 1 + (2 * 3) = 4 + 6 = 10 columns
    return 10;
}

// Returns the data for a specific cell
QVariant TournamentLeaderboardModel::data(const QModelIndex &index, int role) const
{
    // Check if the index is valid and within the model's bounds
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= columnCount())
    {
        return QVariant();
    }

    // Get the calculated leaderboard row for the current index
    const LeaderboardRow* rowData = getLeaderboardRow(index.row());
    if (!rowData) {
        return QVariant(); // Should not happen if index is valid
    }

    // Handle DisplayRole
    if (role == Qt::DisplayRole)
    {
        switch (index.column()) {
            case 0: return rowData->rank;                   // Rank column
            case 1: return rowData->playerName;             // Player Name column
            case 2: return rowData->playerHandicap;         // Handicap column// Daily points columns
            case 3: // Day 1 Total Points
            case 4: // Day 1 Net Points
            case 5: // Day 2 Total Points
            case 6: // Day 2 Net Points
            case 7: // Day 3 Total Points
            case 8: // Day 3 Net Points
            {
                int dayNum = getDayNumForColumn(index.column());
                if (dayNum != -1 && rowData->dailyTotalPoints.contains(dayNum)) { // Check if this player has scores for this day
                     if (index.column() % 2 == 0) { // Even columns (4, 6, 8) are Total Points
                         return rowData->dailyNetPoints[dayNum];
                     } else { // Odd columns (3, 5, 7) are Total Points
                         return rowData->dailyTotalPoints[dayNum];
                     }
                }
                return QVariant(); // Return empty if no scores for this day
            }
            case 9: return rowData->totalStablefordPoints; // Total Stableford Points column
            // Add cases for other columns here
            default: return QVariant();
        }
    }

    // Handle other roles like TextAlignmentRole
    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
            case 0: // Rank
            case 2: // Handicap
            case 3: // Total Stableford Points
            case 4: case 5: // Day 1 Total/Net
            case 6: case 7: // Day 2 Total/Net
            case 8: case 9: // Day 3 Total/Net
                return Qt::AlignCenter; // Center align numeric columns
            case 1: // Player Name
                return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter); // Left align name
            default: return QVariant();
        }
    }
    // Add other roles like BackgroundRole, ForegroundRole, etc.

    return QVariant(); // Return an invalid QVariant for roles we don't handle
}

// Returns the header data for rows and columns
QVariant TournamentLeaderboardModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // We only handle DisplayRole for headers
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) // Column headers
        {
            switch (section) {
                case 0: return "Rank";
                case 1: return "Player";
                case 2: return "Point Target";

                // Daily points headers
                case 3: return "Day 1 Total";
                case 4: return "Day 1 Net";
                case 5: return "Day 2 Total";
                case 6: return "Day 2 Net";
                case 7: return "Day 3 Total";
                case 8: return "Day 3 Net";

                case 9: return "Net Points";
                // Add headers for other columns here
                default: return QVariant();
            }
        }
        // Vertical headers are usually not needed for leaderboards
    }

    // Add other roles like TextAlignmentRole for headers
    if (role == Qt::TextAlignmentRole) {
         if (orientation == Qt::Horizontal) {
            switch (section) {
                case 0: // Rank
                case 2: // Handicap
                case 3: // Total Stableford Points
                case 4: case 5: // Day 1 Total/Net
                case 6: case 7: // Day 2 Total/Net
                case 8: case 9: // Day 3 Total/Net
                    return Qt::AlignCenter; // Center align numeric column headers
                case 1: // Player
                    return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter); // Left align name header
                default: return QVariant();
            }
         }
    }


    return QVariant(); // Return an invalid QVariant for roles/orientations we don't handle
}

// === Public method to refresh data and recalculate leaderboard ===
void TournamentLeaderboardModel::refreshData()
{
    // Notify the view that the model's data is about to be reset
    beginResetModel();

    // Clear previous data
    m_allPlayers.clear();
    m_allHoleDetails.clear();
    m_allScores.clear();
    m_leaderboardData.clear(); // Clear calculated data

    // Fetch raw data from the database
    fetchAllPlayers();
    fetchAllHoleDetails();
    fetchAllScores();

    // Perform calculations and ranking
    calculateLeaderboard();

    // Notify the view that the model's data has been reset
    endResetModel();

    qDebug() << "TournamentLeaderboardModel: Data refreshed. Leaderboard rows:" << m_leaderboardData.size();
}

// === Public method to get days with scores ===
QSet<int> TournamentLeaderboardModel::getDaysWithScores() const
{
    return m_daysWithScores;
}

// === Private helper methods ===

// Fetches all active players
void TournamentLeaderboardModel::fetchAllPlayers()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "TournamentLeaderboardModel::fetchAllPlayers: ERROR: Invalid or closed database connection.";
        return;
    }

    QSqlQuery query(db);
    // Select players who are marked as active
    if (query.exec("SELECT id, name, handicap FROM players WHERE active = 1")) {
        while (query.next()) {
            PlayerInfo player;
            player.id = query.value("id").toInt();
            player.name = query.value("name").toString();
            player.handicap = query.value("handicap").toInt();
            m_allPlayers[player.id] = player; // Store by player ID
        }
        qDebug() << "TournamentLeaderboardModel::fetchAllPlayers: Fetched" << m_allPlayers.size() << "active players.";
    } else {
        qDebug() << "TournamentLeaderboardModel::fetchAllPlayers: ERROR executing query:" << query.lastError().text();
    }
}

// Fetches details (Par, Handicap) for all holes on all courses
void TournamentLeaderboardModel::fetchAllHoleDetails()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "TournamentLeaderboardModel::fetchAllHoleDetails: ERROR: Invalid or closed database connection.";
        return;
    }

    QSqlQuery query(db);
    // Select hole details for all courses and holes
    if (query.exec("SELECT course_id, hole_num, par, handicap FROM holes ORDER BY course_id, hole_num")) {
        while (query.next()) {
            int courseId = query.value("course_id").toInt();
            int holeNum = query.value("hole_num").toInt();
            int par = query.value("par").toInt();
            int handicap = query.value("handicap").toInt();
            m_allHoleDetails[qMakePair(courseId, holeNum)] = qMakePair(par, handicap); // Store by CourseId and HoleNum
        }
        qDebug() << "TournamentLeaderboardModel::fetchAllHoleDetails: Fetched details for" << m_allHoleDetails.size() << "holes.";
    } else {
        qDebug() << "TournamentLeaderboardModel::fetchAllHoleDetails: ERROR executing query:" << query.lastError().text();
    }
}

// Fetches all scores for active players across all days and courses
void TournamentLeaderboardModel::fetchAllScores()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "TournamentLeaderboardModel::fetchAllScores: ERROR: Invalid or closed database connection.";
        return;
    }

    // Build a list of active player IDs
    QStringList activePlayerIds;
    for (auto const& [playerId, playerInfo] : m_allPlayers.asKeyValueRange()) {
        activePlayerIds << QString::number(playerId);
    }

     if (activePlayerIds.isEmpty()) {
        qDebug() << "TournamentLeaderboardModel::fetchAllScores: No active players to fetch scores for.";
        return;
    }

    QSqlQuery query(db);
    // Select scores for active players across all days and courses
    // Use an IN clause for player_id
    QString queryString = QString("SELECT player_id, course_id, hole_num, day_num, score FROM scores WHERE player_id IN (%1) ORDER BY player_id, day_num, hole_num;")
                          .arg(activePlayerIds.join(","));

    qDebug() << "TournamentLeaderboardModel::fetchAllScores: Executing query:" << queryString;

    if (query.exec(queryString)) {
        while (query.next()) {
            int playerId = query.value("player_id").toInt();
            int courseId = query.value("course_id").toInt();
            int holeNum = query.value("hole_num").toInt();
            int dayNum = query.value("day_num").toInt();
            int score = query.value("score").toInt();

            // Store the score and course_id in the nested map: m_allScores[playerId][dayNum][holeNum] = Pair<Score, CourseId>
            m_allScores[playerId][dayNum][holeNum] = qMakePair(score, courseId);
        }
        qDebug() << "TournamentLeaderboardModel::fetchAllScores: Fetched scores for" << m_allScores.size() << "players.";
    } else {
        qDebug() << "TournamentLeaderboardModel::fetchAllScores: ERROR executing query:" << query.lastError().text();
    }
}


// Method to perform Stableford calculations and ranking
void TournamentLeaderboardModel::calculateLeaderboard()
{
    m_leaderboardData.clear(); // Clear previous calculated data

    // Iterate through all active players
    for (auto const& [playerId, playerInfo] : m_allPlayers.asKeyValueRange()) {
        LeaderboardRow row;
        row.playerId = playerId;
        row.playerName = playerInfo.name;
        row.playerHandicap = playerInfo.handicap;
        row.totalStablefordPoints = 0; // Initialize overall total

        // Calculate daily points for this player
        for (int dayNum = 1; dayNum <= 3; ++dayNum) {
            int dailyTotal = 0;
            bool hasScoresForDay = false;

            if (m_allScores.contains(playerId) && m_allScores[playerId].contains(dayNum)) {
                 hasScoresForDay = true;
                 for (auto const& [holeNum, scoreAndCourse] : m_allScores[playerId][dayNum].asKeyValueRange()) {
                       int score = scoreAndCourse.first;
                       int courseIdForScore = scoreAndCourse.second;

                       // Find the par for this hole on this course
                       if (m_allHoleDetails.contains(qMakePair(courseIdForScore, holeNum))) {
                           int par = m_allHoleDetails[qMakePair(courseIdForScore, holeNum)].first; // Get Par from Pair<Par, Handicap>

                           // Calculate Stableford points for this hole
                           int points = calculateStablefordPoints(score, par);
                           dailyTotal += points; // Add to daily total
                       } else {
                            qDebug() << "TournamentLeaderboardModel::calculateLeaderboard: Warning: Hole details not found for Course" << courseIdForScore << "Hole" << holeNum;
                       }
                 }
            }

            if (hasScoresForDay) {
                 row.dailyTotalPoints[dayNum] = dailyTotal;
                 row.dailyNetPoints[dayNum] = dailyTotal - playerInfo.handicap; // Calculate daily net points
                 row.totalStablefordPoints += row.dailyNetPoints[dayNum]; // Add daily total to overall total
                 m_daysWithScores.insert(dayNum);
            }
            // If no scores for the day, the entry in dailyTotalPoints/dailyNetPoints map will be missing,
            // and totalTournamentStablefordPoints remains unchanged for that day.
        }

        m_leaderboardData.append(row); // Add the calculated row to the list
    }

    // --- Rank the players ---
    // Sort m_leaderboardData by totalTournamentStablefordPoints in descending order
    std::sort(m_leaderboardData.begin(), m_leaderboardData.end(),
              [](const LeaderboardRow& a, const LeaderboardRow& b) {
                  return a.totalStablefordPoints > b.totalStablefordPoints; // Sort descending
              });

    // Assign ranks based on the sorted order
    if (!m_leaderboardData.isEmpty()) {
        m_leaderboardData[0].rank = 1; // First player is rank 1
        for (int i = 1; i < m_leaderboardData.size(); ++i) {
            // Assign same rank if points are tied
            if (m_leaderboardData[i].totalStablefordPoints == m_leaderboardData[i-1].totalStablefordPoints) {
                m_leaderboardData[i].rank = m_leaderboardData[i-1].rank;
            } else {
                m_leaderboardData[i].rank = i + 1; // Otherwise, rank is based on position
            }
        }
    }
}

// Helper to calculate Stableford points for a single hole score relative to par
// Rules: Triple Bogey = -1, Double Bogey = 0, Bogey = 1, Par = 2, Birdie = 4, Eagle = 6, Double Eagle = 8
// Net score relative to par determines points. Handicap is applied as starting points.
int TournamentLeaderboardModel::calculateStablefordPoints(int score, int par) const
{
    if (score <= 0 || par <= 0) {
        return 0; // Cannot calculate points for unentered score or invalid par
    }

    // Calculate difference relative to par
    int diff = score - par;

    // Determine points based on the difference
    if (diff <= -3) return 8; // Double Eagle or better
    if (diff == -2) return 6; // Eagle
    if (diff == -1) return 4; // Birdie
    if (diff == 0) return 2;  // Par
    if (diff == 1) return 1;  // Bogey
    if (diff == 2) return 0;  // Double Bogey
    if (diff >= 3) return -1; // Triple Bogey or worse

    return 0; // Should not be reached if score/par are valid
}


// Helper to get LeaderboardRow by row index in m_leaderboardData
const LeaderboardRow* TournamentLeaderboardModel::getLeaderboardRow(int row) const
{
    if (row >= 0 && row < m_leaderboardData.size()) {
        return &m_leaderboardData.at(row);
    }
    return nullptr; // Invalid row
}

// Helper to get the column index for a specific daily points column
// Columns 3-8 are daily points: Day 1 Total (3), Day 1 Net (4), Day 2 Total (5), Day 2 Net (6), Day 3 Total (7), Day 3 Net (8)
int TournamentLeaderboardModel::getColumnForDailyTotalPoints(int dayNum) const
{
    if (dayNum >= 1 && dayNum <= 3) {
        return 3 + (dayNum - 1) * 2;
    }
    return -1; // Invalid day number
}

int TournamentLeaderboardModel::getColumnForDailyNetPoints(int dayNum) const
{
     if (dayNum >= 1 && dayNum <= 3) {
        return 3 + (dayNum - 1) * 2 + 1;
    }
    return -1; // Invalid day number
}

// Helper to get the day number from a daily points column index
int TournamentLeaderboardModel::getDayNumForColumn(int column) const
{
    if (column >= 3 && column <= 8) {
        return 1 + (column - 3) / 2;
    }
    return -1; // Not a daily points column
}
