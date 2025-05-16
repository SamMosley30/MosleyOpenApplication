#include "TeamLeaderboardModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <algorithm> // For std::sort, std::greater
#include <cmath>     // For std::floor (changed from std::ceil)

TeamLeaderboardModel::TeamLeaderboardModel(const QString &connectionName, QObject *parent)
    : QAbstractTableModel(parent), m_connectionName(connectionName) {
    // Initialize leaderboard data for 4 teams
    m_leaderboardData.resize(4);
    for (int i = 0; i < 4; ++i) {
        m_leaderboardData[i].teamId = i + 1;
        m_leaderboardData[i].teamName = QString("Team %1").arg(i + 1);
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
    return m_leaderboardData.size(); // Always 4 teams
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
            case 1: return teamRow.teamName;
            case 2: // Day 1 Points
                return teamRow.dailyTeamStablefordPoints.value(1, 0); // Default to 0 if no score
            case 3: // Day 2 Points
                return teamRow.dailyTeamStablefordPoints.value(2, 0);
            case 4: // Day 3 Points
                return teamRow.dailyTeamStablefordPoints.value(3, 0);
            case 5: return teamRow.overallTeamStablefordPoints;
            default: return QVariant();
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == 1) return QVariant(Qt::AlignLeft | Qt::AlignVCenter); // Team Name
        return Qt::AlignCenter; // Rank and points
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
        if (section == 1) return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        return Qt::AlignCenter;
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

    // Reset team scores before recalculating
    for (TeamLeaderboardRow &teamRow : m_leaderboardData) {
        teamRow.dailyTeamStablefordPoints.clear();
        teamRow.overallTeamStablefordPoints = 0;
        teamRow.rank = 0;
    }

    fetchAllPlayersAndAssignments();
    fetchAllHoleDetails();
    fetchAllScores();
    calculateTeamLeaderboard();

    endResetModel();
    qDebug() << "TeamLeaderboardModel: Data refreshed.";
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
            player.handicap = query.value("handicap").toInt(); // This is the playerDatabaseHandicap
            m_allPlayers[player.id] = player;

            QVariant teamIdVariant = query.value("team_id");
            if (!teamIdVariant.isNull()) {
                m_playerTeamAssignments[player.id] = teamIdVariant.toInt();
            }
        }
        qDebug() << "TeamLeaderboardModel: Fetched" << m_allPlayers.size() << "active players and" << m_playerTeamAssignments.size() << "team assignments.";
    } else {
        qDebug() << "TeamLeaderboardModel::fetchAllPlayersAndAssignments: ERROR executing query:" << query.lastError().text();
    }
}

void TeamLeaderboardModel::fetchAllHoleDetails() {
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) return;
    QSqlQuery query(db);
    // Fetches course_id, hole_num, par, and hole_handicap_index
    if (query.exec("SELECT course_id, hole_num, par, handicap FROM holes")) {
        while (query.next()) {
            m_allHoleDetails[qMakePair(query.value("course_id").toInt(), query.value("hole_num").toInt())] =
                qMakePair(query.value("par").toInt(), query.value("handicap").toInt()); // par, hole_handicap_index
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
            m_allScores[playerId][dayNum][holeNum] = qMakePair(scoreVal, courseId); // score, course_id
            m_daysWithScores.insert(dayNum);
        }
    } else {
        qDebug() << "TeamLeaderboardModel::fetchAllScores: ERROR:" << query.lastError().text();
    }
}

// Calculates Stableford points for a player on a single hole, applying the new handicap logic.
int TeamLeaderboardModel::calculateStablefordPoints(
    int grossScore,             // Actual strokes taken by the player
    int par,                    // Par for the hole
    int playerDatabaseHandicap, // Player's handicap from DB (e.g., 0-54)
    int holeHandicapIndex       // Course's handicap index for this hole (1-18, 1 is hardest)
) const {
    if (grossScore <= 0 || par <= 0) { // No score entered or invalid par
        return 0; 
    }

    int strokesReceivedOnThisHole = 0;

    if (playerDatabaseHandicap <= 36) {
        // Player's handicap value is 36 or less.
        int effectiveStrokesForRound = 36 - playerDatabaseHandicap;

        // Distribute strokes based on hole handicap index
        if (effectiveStrokesForRound >= holeHandicapIndex) {
            strokesReceivedOnThisHole++;
        }
        if (effectiveStrokesForRound >= (18 + holeHandicapIndex)) {
            strokesReceivedOnThisHole++;
        }
        if (effectiveStrokesForRound >= (36 + holeHandicapIndex)) {
            strokesReceivedOnThisHole++;
        }
    } else {
        // Player's handicap value is greater than 36.
        // They give back strokes.
        // Using std::floor to align with "41 handicap -> -2 strokes (gives back 2)" example.
        double strokesToGiveBackTotalDouble = (static_cast<double>(playerDatabaseHandicap) - 36.0) / 2.0;
        int strokesToGiveBackTotal = static_cast<int>(std::floor(strokesToGiveBackTotalDouble)); // Changed from ceil to floor

        if (holeHandicapIndex > (18 - strokesToGiveBackTotal)) {
            strokesReceivedOnThisHole = -1; 
        }
    }

    int netScore = grossScore - strokesReceivedOnThisHole;

    // Calculate Stableford points based on netScore relative to par
    int diff = netScore - par;
    if (diff <= -3) return 8; 
    if (diff == -2) return 6; 
    if (diff == -1) return 4; 
    if (diff == 0) return 2;  
    if (diff == 1) return 1;  
    if (diff == 2) return 0;  
    if (diff >= 3) return -1; 

    return 0; 
}

void TeamLeaderboardModel::calculateTeamLeaderboard() {
    if (m_allPlayers.isEmpty() || m_allHoleDetails.isEmpty()) {
        qDebug() << "TeamLeaderboardModel: Not enough data to calculate (players or hole details missing).";
        return;
    }

    // Iterate over each day (1, 2, 3)
    for (int dayNum = 1; dayNum <= 3; ++dayNum) {
        // Iterate over each team (Team IDs 1, 2, 3, 4)
        for (int teamIdLoop = 1; teamIdLoop <= 4; ++teamIdLoop) { 
            int teamDailyTotalStablefordPoints = 0;
            int teamIndex = teamIdLoop - 1; // 0-indexed for m_leaderboardData

            // Iterate over each hole (1 to 18)
            for (int holeNum = 1; holeNum <= 18; ++holeNum) {
                QVector<int> teamPlayerStablefordScoresForHole;

                // Find players in the current team
                for (auto it = m_playerTeamAssignments.constBegin(); it != m_playerTeamAssignments.constEnd(); ++it) {
                    if (it.value() == teamIdLoop) { // Player is in the current team
                        int playerId = it.key();
                        // Ensure player exists in m_allPlayers (should always be true if data is consistent)
                        if (!m_allPlayers.contains(playerId)) {
                            qWarning() << "TeamLeaderboardModel: Player ID" << playerId << "found in assignments but not in m_allPlayers. Skipping.";
                            continue;
                        }
                        const PlayerInfo& currentPlayer = m_allPlayers.value(playerId);

                        // Check if this player has a score for this day and hole
                        if (m_allScores.contains(playerId) &&
                            m_allScores[playerId].contains(dayNum) &&
                            m_allScores[playerId][dayNum].contains(holeNum)) {
                            
                            QPair<int, int> scoreInfo = m_allScores[playerId][dayNum][holeNum];
                            int playerScoreGross = scoreInfo.first;
                            int courseIdForScore = scoreInfo.second;

                            QPair<int,int> holeDetailsKey = qMakePair(courseIdForScore, holeNum);
                            if (m_allHoleDetails.contains(holeDetailsKey)) {
                                int par = m_allHoleDetails[holeDetailsKey].first;
                                int holeHcIndex = m_allHoleDetails[holeDetailsKey].second; 
                                
                                int points = calculateStablefordPoints(playerScoreGross, par, currentPlayer.handicap, holeHcIndex);
                                teamPlayerStablefordScoresForHole.append(points);
                            } else {
                                qDebug() << "TeamLeaderboardModel: Missing hole details for course" << courseIdForScore << "hole" << holeNum;
                            }
                        }
                    }
                }

                std::sort(teamPlayerStablefordScoresForHole.begin(), teamPlayerStablefordScoresForHole.end(), std::greater<int>());

                int teamHoleScore = 0;
                for (int k = 0; k < qMin(4, teamPlayerStablefordScoresForHole.size()); ++k) {
                    teamHoleScore += teamPlayerStablefordScoresForHole[k];
                }
                teamDailyTotalStablefordPoints += teamHoleScore;
            } 

            m_leaderboardData[teamIndex].dailyTeamStablefordPoints[dayNum] = teamDailyTotalStablefordPoints;
            // Overall points will be summed up after all days are processed for a team
        } 
    } 

    // Calculate overall points by summing the daily points for each team
    for(TeamLeaderboardRow &teamRow : m_leaderboardData) {
        teamRow.overallTeamStablefordPoints = 0; // Reset before summing
        for(int dayPoints : teamRow.dailyTeamStablefordPoints) {
            teamRow.overallTeamStablefordPoints += dayPoints;
        }
    }

    QVector<TeamLeaderboardRow*> teamRowsToSort;
    for(int i=0; i < m_leaderboardData.size(); ++i) teamRowsToSort.append(&m_leaderboardData[i]);

    std::sort(teamRowsToSort.begin(), teamRowsToSort.end(), [](const TeamLeaderboardRow* a, const TeamLeaderboardRow* b) {
        return a->overallTeamStablefordPoints > b->overallTeamStablefordPoints;
    });

    if (!teamRowsToSort.isEmpty()) {
        for (int i = 0; i < teamRowsToSort.size(); ++i) {
            if (i > 0 && teamRowsToSort[i]->overallTeamStablefordPoints == teamRowsToSort[i-1]->overallTeamStablefordPoints) {
                teamRowsToSort[i]->rank = teamRowsToSort[i-1]->rank; 
            } else {
                teamRowsToSort[i]->rank = i + 1;
            }
        }
    }
     qDebug() << "Team leaderboard calculation complete.";
     for(const auto& teamRow : m_leaderboardData) { 
         qDebug() << teamRow.teamName << "Overall Pts:" << teamRow.overallTeamStablefordPoints << "Rank:" << teamRow.rank
                  << "Day1:" << teamRow.dailyTeamStablefordPoints.value(1)
                  << "Day2:" << teamRow.dailyTeamStablefordPoints.value(2)
                  << "Day3:" << teamRow.dailyTeamStablefordPoints.value(3);
     }
}
