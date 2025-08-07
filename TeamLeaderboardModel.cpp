/**
 * @file TeamLeaderboardModel.cpp
 * @brief Implements the TeamLeaderboardModel class.
 */

#include "TeamLeaderboardModel.h"
#include "utils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <algorithm>
#include <cmath>
#include <ranges>
#include <functional>
#include <numeric>

/**
 * @brief Calculates the number of strokes a player receives on a hole.
 * @param handicapForCalc The player's handicap.
 * @param holeHcIndex The handicap index of the hole.
 * @return The number of strokes received.
 */
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
    : QAbstractTableModel(parent), m_connectionName(connectionName) {}

TeamLeaderboardModel::~TeamLeaderboardModel() {}

QSqlDatabase TeamLeaderboardModel::database() const {
    return QSqlDatabase::database(m_connectionName);
}

int TeamLeaderboardModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_leaderboardData.size();
}

int TeamLeaderboardModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return 6; // Rank, Team Name, Day 1 Pts, Day 2 Pts, Day 3 Pts, Overall Pts
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
            case 1: return teamRow.teamName;
            case 2: return teamRow.dailyTeamStablefordPoints.value(1, 0);
            case 3: return teamRow.dailyTeamStablefordPoints.value(2, 0);
            case 4: return teamRow.dailyTeamStablefordPoints.value(3, 0);
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
    m_leaderboardData.clear();

    fetchAllPlayersAndAssignments();
    fetchAllHoleDetails();
    fetchAllScores();
    calculateTeamLeaderboard();

    endResetModel();
}

QSet<int> TeamLeaderboardModel::getDaysWithScores() const {
    return m_daysWithScores;
}

/**
 * @brief Fetches all players and their team assignments from the database.
 */
void TeamLeaderboardModel::fetchAllPlayersAndAssignments() {
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "TeamLeaderboardModel::fetchAllPlayersAndAssignments: ERROR: Invalid or closed database connection.";
        return;
    }

    QSqlQuery teamNameQuery(db);
    if (teamNameQuery.exec("SELECT id, name FROM teams ORDER BY id")) {
        while (teamNameQuery.next()) {
            TeamLeaderboardRow teamRow;
            teamRow.teamId = teamNameQuery.value("id").toInt();
            teamRow.teamName = teamNameQuery.value("name").toString();
            teamRow.overallTeamStablefordPoints = 0;
            teamRow.rank = 0;
            m_leaderboardData.append(teamRow);
        }
    } else {
        qDebug() << "TeamLeaderboardModel::fetchAllPlayersAndAssignments: ERROR fetching team names:" << teamNameQuery.lastError().text();
    }

    QSqlQuery query(db);
    if (query.exec("SELECT id, name, handicap, team_id FROM players WHERE active = 1")) {
        while (query.next()) {
            PlayerInfo player;
            player.id = query.value("id").toInt();
            player.name = query.value("name").toString();
            player.handicap = query.value("handicap").toInt();
            m_allPlayers[player.id] = player;

            QVariant teamIdVariant = query.value("team_id");
            if (!teamIdVariant.isNull()) {
                int teamId = teamIdVariant.toInt();
                m_playerTeamAssignments[player.id] = teamId;
                auto it = std::ranges::find_if(m_leaderboardData,
                                       [teamId](const TeamLeaderboardRow& row) {
                                           return row.teamId == teamId;
                                       });
                if (it != m_leaderboardData.end()) {
                    m_leaderboardData[teamId - 1].teamMembers.append(player);
                }
            }
        }
    } else {
        qDebug() << "TeamLeaderboardModel::fetchAllPlayersAndAssignments: ERROR executing query:" << query.lastError().text();
    }
}

/**
 * @brief Fetches details for all holes from the database.
 */
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

/**
 * @brief Fetches all scores from the database.
 */
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

/**
 * @brief Gets a single player's net stableford score for a specific hole.
 * @param member The player.
 * @param dayNum The day number.
 * @param holeNum The hole number.
 * @return The player's net stableford score, or an empty optional if not available.
 */
std::optional<int> TeamLeaderboardModel::getPlayerNetStablefordForHole(
    const PlayerInfo& member, int dayNum, int holeNum) const
{
    if (!m_allScores.contains(member.id) ||
        !m_allScores[member.id].contains(dayNum) ||
        !m_allScores[member.id][dayNum].contains(holeNum)) {
        return std::nullopt;
    }

    const auto& scoreInfo = m_allScores[member.id][dayNum][holeNum];
    int playerScoreGross = scoreInfo.first;
    int courseIdForScore = scoreInfo.second;

    const auto& holeDetailsKey = qMakePair(courseIdForScore, holeNum);
    if (!m_allHoleDetails.contains(holeDetailsKey)) {
        return std::nullopt;
    }

    const auto& holeDetails = m_allHoleDetails[holeDetailsKey];
    int par = holeDetails.first;
    int holeHcIndex = holeDetails.second;

    int strokesReceived = calculateStrokesReceived(member.handicap, holeHcIndex);
    int netPlayerScore = playerScoreGross - strokesReceived;
    
    if (stableford_conversion.contains(netPlayerScore - par)) {
        return stableford_conversion.at(netPlayerScore - par);
    } else {
        qDebug() << "Invalid score " << netPlayerScore << "on par " << par;
        return std::nullopt;
    }
}

/**
 * @brief Calculates the team score for a single hole.
 * @param team The team.
 * @param dayNum The day number.
 * @param holeNum The hole number.
 * @param numScoresToTake The number of top scores to take.
 * @return The team's score for the hole.
 */
int TeamLeaderboardModel::calculateTeamScoreForHole(TeamLeaderboardRow &team, int dayNum, int holeNum, int numScoresToTake) const
{
    auto scores_view = team.teamMembers
                       | std::views::transform([&](const PlayerInfo &member)
                                               { return getPlayerNetStablefordForHole(member, dayNum, holeNum); })
                       | std::views::filter([](const std::optional<int> &score)
                                            { return score.has_value(); })
                       | std::views::transform([](const std::optional<int> &score)
                                               { return score.value(); });

    std::vector<int> scores_vec;
    std::ranges::copy(scores_view, std::back_inserter(scores_vec));
    std::ranges::sort(scores_vec, std::greater{});

    return std::ranges::fold_left(
        scores_vec | std::views::take(numScoresToTake),
        0,
        std::plus<>{});
}

/**
 * @brief Calculates the team leaderboard.
 */
void TeamLeaderboardModel::calculateTeamLeaderboard()
{
    if (m_allPlayers.isEmpty() || m_allHoleDetails.isEmpty()) {
        qDebug() << "TeamLeaderboardModel: Not enough data to calculate (players or hole details missing).";
        return;
    }

    auto largestTeam = std::ranges::max(m_leaderboardData, [&](const TeamLeaderboardRow &a, const TeamLeaderboardRow &b)
                     { return a.teamMembers.size() < b.teamMembers.size(); });

    int numScoresToTake = (largestTeam.teamMembers.size() > 1) ? largestTeam.teamMembers.size() - 1 : 1;

    std::ranges::for_each(std::views::iota(1, 4), [&](int dayNum) {
        std::ranges::for_each(m_leaderboardData, [&](TeamLeaderboardRow &teamRow) {
            int teamDailyTotalStablefordPoints = 0;
            std::ranges::for_each(std::views::iota(1, 19), [&](int holeNum)
                                  { teamDailyTotalStablefordPoints += calculateTeamScoreForHole(teamRow, dayNum, holeNum, numScoresToTake); });
            teamRow.dailyTeamStablefordPoints[dayNum] = teamDailyTotalStablefordPoints;
        }); 
    });

    std::ranges::for_each(m_leaderboardData, [](TeamLeaderboardRow &teamRow)
                          { teamRow.overallTeamStablefordPoints = std::ranges::fold_left(teamRow.dailyTeamStablefordPoints, 0, std::plus<>{}); });

    std::ranges::sort(m_leaderboardData, [](const TeamLeaderboardRow &a, const TeamLeaderboardRow &b)
                      { return a.overallTeamStablefordPoints > b.overallTeamStablefordPoints; });

    for (int i = 0; i < m_leaderboardData.size(); ++i) {
        if (i > 0 && m_leaderboardData[i].overallTeamStablefordPoints == m_leaderboardData[i - 1].overallTeamStablefordPoints) {
            m_leaderboardData[i].rank = m_leaderboardData[i - 1].rank;
        } else {
            m_leaderboardData[i].rank = i + 1;
        }
    }
}
