#include "TeamLeaderboardModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <algorithm> // For std::sort, std::greater, std::max_element
#include <cmath>     // For std::ceil
#include <ranges>
#include <functional>
#include <numeric>

// Helper function to calculate strokes. This isolates a piece of logic.
int calculateStrokesReceived(int handicapForCalc, int holeHcIndex) {
    if (handicapForCalc <= 36) {
        int effective = 36 - handicapForCalc;
        int strokes = 0;
        if (effective >= holeHcIndex) strokes++;
        if (effective >= (18 + holeHcIndex)) strokes++;
        if (effective >= (36 + holeHcIndex)) strokes++;
        return strokes;
    } else {
        int toGiveBack = static_cast<int>(std::floor((static_cast<double>(handicapForCalc) - 36.0) / 2.0));
        if (holeHcIndex > (18 - toGiveBack)) {
            return -1;
        }
    }
    return 0;
}

TeamLeaderboardModel::TeamLeaderboardModel(const QString &connectionName, QObject *parent)
    : QAbstractTableModel(parent), m_connectionName(connectionName) {
    // Initialize leaderboard data for 6 teams
    m_leaderboardData.resize(6);
    for (int i = 0; i < 6; ++i) {
        m_leaderboardData[i].teamId = i + 1;
        // Initial name, will be overwritten by determineTeamNames()
        m_leaderboardData[i].teamName = QString("Team %1 (Unassigned)").arg(i + 1); 
        m_leaderboardData[i].overallTeamStablefordPoints = 0;
        m_leaderboardData[i].rank = 0;
    }
}

TeamLeaderboardModel::~TeamLeaderboardModel() {}

QSqlDatabase TeamLeaderboardModel::database() const {
    return QSqlDatabase::database(m_connectionName);
}

int TeamLeaderboardModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_leaderboardData.size(); // Always 6 teams
}

int TeamLeaderboardModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    // Columns: Rank, Team Name, Day 1 Pts, Day 2 Pts, Day 3 Pts, Overall Pts
    return 6;
}

QVariant TeamLeaderboardModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= columnCount()) {
        return QVariant();
    }

    const TeamLeaderboardRow &teamRow = m_leaderboardData.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return teamRow.rank > 0 ? QVariant(teamRow.rank) : QVariant("-");
            case 1: return teamRow.teamName; // This will now be "Team <Captain>"
            case 2: // Day 1 Points
                return teamRow.dailyTeamStablefordPoints.value(1, 0); 
            case 3: // Day 2 Points
                return teamRow.dailyTeamStablefordPoints.value(2, 0);
            case 4: // Day 3 Points
                return teamRow.dailyTeamStablefordPoints.value(3, 0);
            case 5: return teamRow.overallTeamStablefordPoints;
            default: return QVariant();
        }
    } else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignCenter); 
    }
    return QVariant();
}

QVariant TeamLeaderboardModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return "Rank";
            case 1: return "Team";
            case 2: return "Day 1 Points";
            case 3: return "Day 2 Points";
            case 4: return "Day 3 Points";
            case 5: return "Overall Points";
            default: return QVariant();
        }
    }
    if (role == Qt::TextAlignmentRole && orientation == Qt::Horizontal) {
        return QVariant(Qt::AlignCenter);
    }
    return QVariant();
}

void TeamLeaderboardModel::refreshData() {
    beginResetModel();

    m_allPlayers.clear();
    m_playerTeamAssignments.clear();
    m_allHoleDetails.clear();
    m_allScores.clear();
    m_daysWithScores.clear();

    for (TeamLeaderboardRow &teamRow : m_leaderboardData) {
        teamRow.dailyTeamStablefordPoints.clear();
        teamRow.overallTeamStablefordPoints = 0;
        teamRow.rank = 0;
        teamRow.teamMembers.clear(); // Clear previous members
        // Reset initial name, determineTeamNames will set it properly
        teamRow.teamName = QString("Team %1 (No Captain)").arg(teamRow.teamId); 
    }

    fetchAllPlayersAndAssignments(); // This will populate m_allPlayers and m_playerTeamAssignments
                                     // And now also populate teamRow.teamMembers
    determineTeamNames();            // Set team names based on captains
    fetchAllHoleDetails();
    fetchAllScores();
    calculateTeamLeaderboard();      // Calculates scores using team members

    endResetModel();
}

QSet<int> TeamLeaderboardModel::getDaysWithScores() const {
    return m_daysWithScores;
}

void TeamLeaderboardModel::fetchAllPlayersAndAssignments() {
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "TeamLeaderboardModel::fetchAllPlayersAndAssignments: ERROR: Invalid or closed database connection.";
        return;
    }
    QSqlQuery query(db);
    if (query.exec("SELECT id, name, handicap, team_id FROM players WHERE active = 1")) {
        while (query.next()) {
            PlayerInfo player;
            player.id = query.value("id").toInt();
            player.name = query.value("name").toString();
            player.handicap = query.value("handicap").toInt(); // This is the "point target"
            m_allPlayers[player.id] = player;

            QVariant teamIdVariant = query.value("team_id");
            if (!teamIdVariant.isNull()) {
                int teamId = teamIdVariant.toInt();
                m_playerTeamAssignments[player.id] = teamId;
                // Add player to the correct team's member list
                if (teamId >= 1 && teamId <= m_leaderboardData.size()) {
                    m_leaderboardData[teamId - 1].teamMembers.append(player);
                }
            }
        }
    } else {
        qDebug() << "TeamLeaderboardModel::fetchAllPlayersAndAssignments: ERROR executing query:" << query.lastError().text();
    }
}

void TeamLeaderboardModel::determineTeamNames() {
    for (TeamLeaderboardRow &teamRow : m_leaderboardData) {
        if (teamRow.teamMembers.isEmpty()) {
            teamRow.teamName = QString("Team %1 (Empty)").arg(teamRow.teamId);
            continue;
        }

        // Find player with highest handicap (point target) in this team
        auto captainIt = std::ranges::max_element(teamRow.teamMembers,
            [](const PlayerInfo& a, const PlayerInfo& b) {
                return a.handicap < b.handicap; // Higher handicap is "greater"
            });
        
        // Check if max_element returned a valid iterator (not end())
        if (captainIt != teamRow.teamMembers.end()) {
            teamRow.teamName = QString("Team %1").arg(captainIt->name);
        } else {
            // Should not happen if teamMembers is not empty, but as a fallback:
            teamRow.teamName = QString("Team %1 (No Captain Found)").arg(teamRow.teamId);
        }
    }
}


void TeamLeaderboardModel::fetchAllHoleDetails() {
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) return;
    QSqlQuery query(db);
    if (query.exec("SELECT course_id, hole_num, par, handicap FROM holes")) {
        while (query.next()) {
            m_allHoleDetails[qMakePair(query.value("course_id").toInt(), query.value("hole_num").toInt())] =
                qMakePair(query.value("par").toInt(), query.value("handicap").toInt());
        }
    } else {
        qDebug() << "TeamLeaderboardModel::fetchAllHoleDetails: ERROR:" << query.lastError().text();
    }
}

void TeamLeaderboardModel::fetchAllScores() {
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) return;
    QSqlQuery query(db);
    if (query.exec("SELECT player_id, course_id, hole_num, day_num, score FROM scores")) {
        while (query.next()) {
            int playerId = query.value("player_id").toInt();
            int dayNum = query.value("day_num").toInt();
            int holeNum = query.value("hole_num").toInt();
            int scoreVal = query.value("score").toInt();
            int courseId = query.value("course_id").toInt();
            m_allScores[playerId][dayNum][holeNum] = qMakePair(scoreVal, courseId);
            m_daysWithScores.insert(dayNum);
        }
    } else {
        qDebug() << "TeamLeaderboardModel::fetchAllScores: ERROR:" << query.lastError().text();
    }
}

int TeamLeaderboardModel::calculateStablefordPoints(int score, int par) const {

    if (score <= 0 || par <= 0) return 0;
    int diff = score - par; // Assuming 'score' is already adjusted for handicap for this player
    if (diff <= -3) return 8; 
    if (diff == -2) return 6; 
    if (diff == -1) return 4; 
    if (diff == 0) return 2;  
    if (diff == 1) return 1;  
    if (diff == 2) return 0;  
    if (diff >= 3) return -1; 
    return 0;
}

// Helper to get a single player's net stableford score for a specific hole.
// Returns an empty optional if the player has no score for that hole.
std::optional<int> TeamLeaderboardModel::getPlayerNetStablefordForHole(
    const PlayerInfo& member, int dayNum, int holeNum) const
{
    // Check if the player has a score for this specific day and hole
    if (!m_allScores.contains(member.id) ||
        !m_allScores[member.id].contains(dayNum) ||
        !m_allScores[member.id][dayNum].contains(holeNum)) {
        return std::nullopt; // No score available
    }

    const auto& scoreInfo = m_allScores[member.id][dayNum][holeNum];
    int playerScoreGross = scoreInfo.first;
    int courseIdForScore = scoreInfo.second;

    const auto& holeDetailsKey = qMakePair(courseIdForScore, holeNum);
    if (!m_allHoleDetails.contains(holeDetailsKey)) {
        return std::nullopt; // Hole details missing
    }

    const auto& holeDetails = m_allHoleDetails[holeDetailsKey];
    int par = holeDetails.first;
    int holeHcIndex = holeDetails.second;

    // For team scoring, always use the player's actual handicap (Twisted Creek rules)
    int strokesReceived = calculateStrokesReceived(member.handicap, holeHcIndex);
    int netPlayerScore = playerScoreGross - strokesReceived;
    
    // Pass the calculated net score to the basic Stableford points function
    return calculateStablefordPoints(netPlayerScore, par);
}

int TeamLeaderboardModel::calculateTeamScoreForHole(TeamLeaderboardRow &team, int dayNum, int holeNum) const
{
    // --- Ranges Pipeline to get scores for this hole ---
    auto scores_view = team.teamMembers
                       // 1. Transform each team member into their potential score for this hole
                       | std::views::transform([&](const PlayerInfo &member)
                                               { return getPlayerNetStablefordForHole(member, dayNum, holeNum); })
                       // 2. Filter out any members who didn't have a score
                       | std::views::filter([](const std::optional<int> &score)
                                            { return score.has_value(); })
                       // 3. Transform the remaining optionals into their integer values
                       | std::views::transform([](const std::optional<int> &score)
                                               { return score.value(); });

    // Materialize the view into a temporary vector to sort it.
    std::vector<int> scores_vec;
    std::ranges::copy(scores_view, std::back_inserter(scores_vec));
    std::ranges::sort(scores_vec, std::greater{});

    // Sum the top 2 scores for the hole
    return std::ranges::fold_left(
        scores_vec | std::views::take(2),
        0,
        std::plus<>{});
}

void TeamLeaderboardModel::calculateTeamLeaderboard()
{
    if (m_allPlayers.isEmpty() || m_allHoleDetails.isEmpty())
    {
        qDebug() << "TeamLeaderboardModel: Not enough data to calculate (players or hole details missing).";
        return;
    }

    // Iterate over each day (1, 2, 3)
    std::ranges::for_each(std::views::iota(1, 4), [&](int dayNum)
                          {
        // Iterate over each team
        std::ranges::for_each(m_leaderboardData, [&](TeamLeaderboardRow &teamRow)
        {
            int teamDailyTotalStablefordPoints = 0;

            // Iterate over each hole (1 to 18)
            std::ranges::for_each(std::views::iota(1, 19), [&](int holeNum)
                                  { teamDailyTotalStablefordPoints += calculateTeamScoreForHole(teamRow, dayNum, holeNum); });
            teamRow.dailyTeamStablefordPoints[dayNum] = teamDailyTotalStablefordPoints;
        }); });

    // Calculate overall points by summing the daily points for each team
    std::ranges::for_each(m_leaderboardData, [](TeamLeaderboardRow &teamRow)
                          { teamRow.overallTeamStablefordPoints = std::ranges::fold_left(teamRow.dailyTeamStablefordPoints, 0, std::plus<>{}); });

    // --- Ranking logic (remains the same) ---
    std::ranges::sort(m_leaderboardData, [](const TeamLeaderboardRow &a, const TeamLeaderboardRow &b)
                      { return a.overallTeamStablefordPoints > b.overallTeamStablefordPoints; });

    for (int i = 0; i < m_leaderboardData.size(); ++i)
    {
        if (i > 0 && m_leaderboardData[i].overallTeamStablefordPoints == m_leaderboardData[i - 1].overallTeamStablefordPoints)
            m_leaderboardData[i].rank = m_leaderboardData[i - 1].rank;
        else
            m_leaderboardData[i].rank = i + 1;
    }
}
