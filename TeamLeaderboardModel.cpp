#include "TeamLeaderboardModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <algorithm> // For std::sort, std::greater, std::max_element
#include <cmath>     // For std::ceil

TeamLeaderboardModel::TeamLeaderboardModel(const QString &connectionName, QObject *parent)
    : QAbstractTableModel(parent), m_connectionName(connectionName) {
    // Initialize leaderboard data for 4 teams
    m_leaderboardData.resize(4);
    for (int i = 0; i < 4; ++i) {
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
        if (index.column() == 1) return QVariant(Qt::AlignLeft | Qt::AlignVCenter); 
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
        if (section == 1) return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
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
        qDebug() << "TeamLeaderboardModel: Fetched" << m_allPlayers.size() << "active players and" << m_playerTeamAssignments.size() << "team assignments.";
        for(const auto& teamRow : m_leaderboardData) {
            qDebug() << "Team" << teamRow.teamId << "has" << teamRow.teamMembers.size() << "members initially populated.";
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
        auto captainIt = std::max_element(teamRow.teamMembers.begin(), teamRow.teamMembers.end(),
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
        qDebug() << "Determined name for Team ID" << teamRow.teamId << ":" << teamRow.teamName;
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

// This is the basic Stableford calculation, not applying the complex player-specific handicap rules here.
// The complex handicap should be applied to get a NET score first, then that net score used here.
// However, for the team "sum of 4 best Stableford points", we need individual player Stableford points
// calculated using their *Twisted Creek* (actual) handicaps.
int TeamLeaderboardModel::calculateStablefordPoints(int score, int par) const {
    // This function might need to be the more complex one if individual player handicaps
    // for Twisted Creek rules are to be applied before summing for the team.
    // For now, let's assume 'score' is already a NET score for Twisted Creek rules.
    // OR, this is a GROSS stableford calculation, and net is handled elsewhere.
    // Based on "sum the stableford points", it implies individual stableford points are calculated first.
    // Let's assume this is a GROSS Stableford for now, and the team logic will fetch NET.
    // No, the problem says "sum the stableford points" which are inherently net.
    // This method needs to be the one from TournamentLeaderboardModel (the complex one)
    // or a similar one that takes playerActualDbHandicap and holeHandicapIndex.
    // For simplicity in this step, I'll keep it basic, assuming NET score is fed.
    // THIS WILL NEED REVISITING if individual player handicaps are applied before summing team points.

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

void TeamLeaderboardModel::calculateTeamLeaderboard() {
    if (m_allPlayers.isEmpty() || m_allHoleDetails.isEmpty()) {
        qDebug() << "TeamLeaderboardModel: Not enough data to calculate (players or hole details missing).";
        return;
    }

    // Iterate over each day (1, 2, 3)
    for (int dayNum = 1; dayNum <= 3; ++dayNum) {
        // Iterate over each team (using m_leaderboardData which now has teamMembers)
        for (TeamLeaderboardRow &teamRow : m_leaderboardData) {
            int teamDailyTotalStablefordPoints = 0;

            // Iterate over each hole (1 to 18)
            for (int holeNum = 1; holeNum <= 18; ++holeNum) {
                QVector<int> teamPlayerNetStablefordScoresForHole;

                // Iterate through members of the current team
                for (const PlayerInfo &member : teamRow.teamMembers) {
                    int playerId = member.id;
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
                            
                            // Calculate NET Stableford points for this player on this hole
                            // using their actual DB handicap (Twisted Creek rules for team comp)
                            // We need the complex calculateNetStablefordPointsForHole from TournamentLeaderboardModel here.
                            // For now, I'll call the basic one, assuming it's been adapted or score is pre-netted.
                            // THIS IS A PLACEHOLDER FOR THE CORRECT, COMPLEX HANDICAP CALCULATION
                            // For team scores, use the player's ACTUAL handicap (Twisted Creek rules)Blake Mozley
                            int points = 0; // Placeholder
                            // Correct call would be something like:
                            // points = TournamentLeaderboardModel::calculateNetStablefordPointsForHole(
                            //     playerScoreGross, par, member.handicap, holeHcIndex, TournamentLeaderboardModel::TwistedCreek
                            // );
                            // Since this model doesn't have that directly, we'll use the basic one for now.
                            // This means the handicap logic for team scores needs to be correctly integrated.
                            // Let's use the existing simple calculateStablefordPoints and assume score is somehow net.
                            // To do it properly, we need the full handicap calculation from the other model.
                            // For now, let's just calculate gross stableford to see structure.
                            // int gross_diff = playerScoreGross - par;
                            // if (gross_diff <= -3) points = 8; else if (gross_diff == -2) points = 6; else if (gross_diff == -1) points = 4;
                            // else if (gross_diff == 0) points = 2; else if (gross_diff == 1) points = 1; else if (gross_diff == 2) points = 0;
                            // else if (gross_diff >= 3) points = -1;

                            // Re-inserting the proper call from TournamentLeaderboardModel logic:
                            // This requires TeamLeaderboardModel to have access to that specific calculation logic.
                            // Let's assume we'll adapt this model's calculateStablefordPoints to be the complex one.
                            // For now, I'll call the simple one and it will be incorrect for net.
                            // The user stated "sum the stableford points" implying they are calculated per player first.
                            // The Team Leaderboard uses Twisted Creek handicaps (player's actual handicap).

                            // We need a method similar to TournamentLeaderboardModel::calculateNetStablefordPointsForHole
                            // Let's temporarily define a simplified one here or assume the existing one is for NET.
                            // For now, the existing calculateStablefordPoints will be used, assuming it gets a NET score.
                            // This is the main point of complexity to resolve.
                            // Let's assume calculateStablefordPoints should be the complex one.
                            // To avoid circular dependencies or large code duplication, this is tricky.
                            // A shared utility function for handicap calculation would be best.
                            // For now, I'll duplicate the logic from TournamentLeaderboardModel's calculateNetStablefordPointsForHole
                            // and ensure it uses the player's actual handicap for team scoring.

                            int handicapForCalc = member.handicap; // Use actual handicap for team scoring
                            int strokesReceived = 0;
                            if (handicapForCalc <= 36) {
                                int effective = 36 - handicapForCalc;
                                if (effective >= holeHcIndex) strokesReceived++;
                                if (effective >= (18 + holeHcIndex)) strokesReceived++;
                                if (effective >= (36 + holeHcIndex)) strokesReceived++;
                            } else {
                                int toGiveBack = static_cast<int>(std::floor((static_cast<double>(handicapForCalc) - 36.0) / 2.0));
                                if (holeHcIndex > (18 - toGiveBack)) strokesReceived = -1;
                            }
                            int netPlayerScore = playerScoreGross - strokesReceived;
                            points = calculateStablefordPoints(netPlayerScore, par); // Call with net score

                            teamPlayerNetStablefordScoresForHole.append(points);
                        }
                    }
                }

                std::sort(teamPlayerNetStablefordScoresForHole.begin(), teamPlayerNetStablefordScoresForHole.end(), std::greater<int>());

                int teamHoleScore = 0;
                for (int k = 0; k < qMin(4, teamPlayerNetStablefordScoresForHole.size()); ++k) {
                    teamHoleScore += teamPlayerNetStablefordScoresForHole[k];
                }
                teamDailyTotalStablefordPoints += teamHoleScore;
            } 
            teamRow.dailyTeamStablefordPoints[dayNum] = teamDailyTotalStablefordPoints;
        } 
    } 

    for(TeamLeaderboardRow &teamRow : m_leaderboardData) {
        teamRow.overallTeamStablefordPoints = 0; 
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
