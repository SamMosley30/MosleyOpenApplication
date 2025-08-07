/**
 * @file TournamentLeaderboardModel.cpp
 * @brief Implements the TournamentLeaderboardModel class.
 */

#include "utils.h"
#include "TournamentLeaderboardModel.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>
#include <algorithm>
#include <cmath>
#include <ranges>

TournamentLeaderboardModel::TournamentLeaderboardModel(const QString &connectionName, QObject *parent)
    : QAbstractTableModel(parent), m_connectionName(connectionName),
      m_tournamentContext(TwistedCreek),
      m_cutLineScore(0), m_isCutApplied(false)
{
}

TournamentLeaderboardModel::~TournamentLeaderboardModel() {}

QSqlDatabase TournamentLeaderboardModel::database() const
{
    QSqlDatabase db_check = QSqlDatabase::database(m_connectionName, false);
    if (!db_check.isValid()) {
        qWarning() << "TournamentLeaderboardModel instance" << this
                   << "DB connection name '" << m_connectionName << "' is NOT VALID (not found in Qt's list). "
                   << "Available connections:" << QSqlDatabase::connectionNames();
    } else if (!db_check.isOpen()) {
        qWarning() << "TournamentLeaderboardModel instance" << this
                   << "DB connection '" << m_connectionName << "' is VALID but NOT OPEN. Last error:" << db_check.lastError().text();
    }
    return db_check;
}

void TournamentLeaderboardModel::setTournamentContext(TournamentContext context)
{
    m_tournamentContext = context;
}

void TournamentLeaderboardModel::setCutLineScore(int score)
{
    m_cutLineScore = score;
}

void TournamentLeaderboardModel::setIsCutApplied(bool applied)
{
    m_isCutApplied = applied;
}

int TournamentLeaderboardModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_leaderboardData.size();
}

int TournamentLeaderboardModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 10;
}

QVariant TournamentLeaderboardModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= columnCount()) {
        return QVariant();
    }

    const LeaderboardRow &rowData = m_leaderboardData.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return rowData.rank > 0 ? QVariant(rowData.rank) : QVariant("-");
        case 1: return rowData.playerName;
        case 2: return rowData.playerActualDbHandicap;
        case 3: return rowData.dailyGrossStablefordPoints.contains(1) ? QVariant(rowData.dailyGrossStablefordPoints.value(1)) : QVariant();
        case 4: return rowData.dailyNetStablefordPoints.contains(1) ? QVariant(rowData.dailyNetStablefordPoints.value(1)) : QVariant();
        case 5: return rowData.dailyGrossStablefordPoints.contains(2) ? QVariant(rowData.dailyGrossStablefordPoints.value(2)) : QVariant();
        case 6: return rowData.dailyNetStablefordPoints.contains(2) ? QVariant(rowData.dailyNetStablefordPoints.value(2)) : QVariant();
        case 7: return rowData.dailyGrossStablefordPoints.contains(3) ? QVariant(rowData.dailyGrossStablefordPoints.value(3)) : QVariant();
        case 8: return rowData.dailyNetStablefordPoints.contains(3) ? QVariant(rowData.dailyNetStablefordPoints.value(3)) : QVariant();
        case 9: return rowData.totalNetStablefordPoints;
        default: return QVariant();
        }
    } else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignCenter);
    }
    return QVariant();
}

QVariant TournamentLeaderboardModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    const std::unordered_map<int, QString> columnNames = {
        {0, "Rank"}, {1, "Player"}, {2, "Point Target"}, {3, "Day 1 Gross"}, {4, "Day 1 Net"},
        {5, "Day 2 Gross"}, {6, "Day 2 Net"}, {7, "Day 3 Gross"}, {8, "Day 3 Net"}, {9, "Overall Net"},
    };

    if (role == Qt::DisplayRole && orientation == Qt::Horizontal && columnNames.contains(section)) {
        return columnNames.at(section);
    } else if (role == Qt::TextAlignmentRole && orientation == Qt::Horizontal) {
        return QVariant(Qt::AlignCenter);
    }
    
    return QVariant();
}

void TournamentLeaderboardModel::refreshData()
{
    beginResetModel();

    m_allPlayers.clear();
    m_holeParAndHandicapIndex.clear();
    m_allScores.clear();
    m_leaderboardData.clear();
    m_daysWithScores.clear();
    m_playerTwoDayMosleyNetScoreForCut.clear();

    fetchAllPlayers();
    if (m_allPlayers.isEmpty()) {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllPlayers resulted in empty m_allPlayers. Further calculations might be skipped or incorrect.";
    }

    fetchAllHoleDetails();
    fetchAllScores();

    calculateAllPlayerTwoDayMosleyNetScores();
    calculateLeaderboard();

    endResetModel();
    if (m_leaderboardData.isEmpty() && !m_allPlayers.isEmpty()) {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- WARNING: m_leaderboardData is empty but m_allPlayers is not. Check filtering logic in calculateLeaderboard.";
    } else if (m_allPlayers.isEmpty() && (m_tournamentContext == MosleyOpen || m_tournamentContext == TwistedCreek)) {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- WARNING: m_allPlayers is empty. No players fetched. Leaderboard will be empty.";
    }
}

QSet<int> TournamentLeaderboardModel::getDaysWithScores() const
{
    return m_daysWithScores;
}

/**
 * @brief Fetches all active players from the database.
 */
void TournamentLeaderboardModel::fetchAllPlayers()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllPlayers: DB connection from database() is not valid or not open. Aborting fetch.";
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
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllPlayers: SQL ERROR:" << query.lastError().text();
    }
}

/**
 * @brief Fetches details for all holes from the database.
 */
void TournamentLeaderboardModel::fetchAllHoleDetails()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllHoleDetails: DB not open. Skipping.";
        return;
    }
    QSqlQuery query(db);
    if (query.exec("SELECT course_id, hole_num, par, handicap FROM holes")) {
        while (query.next()) {
            m_holeParAndHandicapIndex[qMakePair(query.value("course_id").toInt(), query.value("hole_num").toInt())] =
                qMakePair(query.value("par").toInt(), query.value("handicap").toInt());
        }
    } else {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllHoleDetails: SQL ERROR:" << query.lastError().text();
    }
}

/**
 * @brief Fetches all scores from the database.
 */
void TournamentLeaderboardModel::fetchAllScores()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllScores: DB not open. Skipping.";
        return;
    }
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
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllScores: SQL ERROR:" << query.lastError().text();
    }
}

int TournamentLeaderboardModel::calculateNetStablefordPointsForHole(
    int grossScore, int par) const
{
    if (grossScore <= 0 || par <= 0) {
        return 0;
    }
        
    int diff = grossScore - par;

    return stableford_conversion.at(diff);
}

/**
 * @brief Calculates the two-day Mosley net scores for all players, used for the cut.
 */
void TournamentLeaderboardModel::calculateAllPlayerTwoDayMosleyNetScores()
{
    m_playerTwoDayMosleyNetScoreForCut.clear();
    if (m_allPlayers.isEmpty()) {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- calculateAllPlayerTwoDayMosleyNetScores: m_allPlayers is empty. Skipping.";
        return;
    }

    for (auto const &[playerId, playerInfo] : m_allPlayers.asKeyValueRange()) {
        int twoDayTotalNetForPlayer = 0;
        for (int dayNum = 1; dayNum <= 2; ++dayNum) {
            if (!m_allScores.contains(playerId) || !m_allScores[playerId].contains(dayNum)) {
                continue;
            }

            for (auto const &[holeNum, scoreAndCourse] : m_allScores[playerId][dayNum].asKeyValueRange()) {
                int grossScore = scoreAndCourse.first;
                int courseIdForScore = scoreAndCourse.second;
                QPair<int, int> holeDetailsKey = qMakePair(courseIdForScore, holeNum);
                if (!m_holeParAndHandicapIndex.contains(holeDetailsKey)) {
                    continue;
                }
                
                int par = m_holeParAndHandicapIndex[holeDetailsKey].first;
                if (stableford_conversion.contains(grossScore - par)) {
                    twoDayTotalNetForPlayer += stableford_conversion.at(grossScore - par);
                } else {
                    qDebug() << "Invalid score " << grossScore << "on par " << par << "for " << playerInfo.name;
                }
            }
            twoDayTotalNetForPlayer -= std::max(16, playerInfo.handicap);
        }
        m_playerTwoDayMosleyNetScoreForCut[playerId] = twoDayTotalNetForPlayer;
    }
}

/**
 * @brief Calculates the leaderboard based on the current context and cut line.
 */
void TournamentLeaderboardModel::calculateLeaderboard()
{
    m_leaderboardData.clear();

    for (auto const &[playerId, playerInfo] : m_allPlayers.asKeyValueRange()) {
        int playerTwoDayMosleyScore = m_playerTwoDayMosleyNetScoreForCut.value(playerId, 0);

        bool madeTheCut = m_isCutApplied && playerTwoDayMosleyScore >= m_cutLineScore;
        bool includePlayer = !m_isCutApplied ||
                             (madeTheCut && m_tournamentContext == MosleyOpen) ||
                             (!madeTheCut && m_tournamentContext == TwistedCreek);

        if (!includePlayer) continue;

        LeaderboardRow row;
        row.playerId = playerId;
        row.playerName = playerInfo.name;
        row.playerActualDbHandicap = playerInfo.handicap;
        row.totalNetStablefordPoints = 0;
        row.twoDayMosleyNetScoreForCut = playerTwoDayMosleyScore;

        for (int dayNum = 1; dayNum <= 3; ++dayNum) {
            int dailyGrossPts = 0;

            if (!m_allScores.contains(playerId) || !m_allScores[playerId].contains(dayNum)) {
                continue;
            }

            int total_score = 0;
            for (auto const &[holeNum, scoreAndCourse] : m_allScores[playerId][dayNum].asKeyValueRange()) {
                int grossScore = scoreAndCourse.first;
                total_score += grossScore;
                int courseIdForScore = scoreAndCourse.second;
                QPair<int, int> holeDetailsKey = qMakePair(courseIdForScore, holeNum);
                if (m_holeParAndHandicapIndex.contains(holeDetailsKey)) {
                    int par = m_holeParAndHandicapIndex[holeDetailsKey].first;
                    if (stableford_conversion.contains(grossScore - par)) {
                        dailyGrossPts += stableford_conversion.at(grossScore - par);
                    } else {
                        qDebug() << "Invalid score " << grossScore << "on par " << par << "for " << playerInfo.name;
                    }
                }
            }

            qDebug() << playerInfo.name << " shot " << total_score << " on day " << dayNum;

            row.dailyGrossStablefordPoints[dayNum] = dailyGrossPts;
            if (m_tournamentContext == MosleyOpen) {
                row.dailyNetStablefordPoints[dayNum] = dailyGrossPts - std::max(16, playerInfo.handicap);
            } else {
                row.dailyNetStablefordPoints[dayNum] = dailyGrossPts - playerInfo.handicap;
            }
            row.totalNetStablefordPoints += row.dailyNetStablefordPoints[dayNum];
        }
        m_leaderboardData.append(row);
    }

    std::ranges::sort(m_leaderboardData,
                      [](const auto &a, const auto &b) {
                          return a.totalNetStablefordPoints > b.totalNetStablefordPoints;
                      });

    if (m_leaderboardData.isEmpty()) return;
    
    m_leaderboardData[0].rank = 1;
    for (int i = 1; i < m_leaderboardData.size(); ++i) {
        if (m_leaderboardData[i].totalNetStablefordPoints == m_leaderboardData[i - 1].totalNetStablefordPoints) {
            m_leaderboardData[i].rank = m_leaderboardData[i - 1].rank;
        } else {
            m_leaderboardData[i].rank = i + 1;
        }
    }
}

int TournamentLeaderboardModel::getColumnForDailyGrossPoints(int dayNum) const
{
    const std::unordered_map<int, int> columnMap = {{1, 3}, {2, 5}, {3, 7}};
    return columnMap.contains(dayNum) ? columnMap.at(dayNum) : -1;
}

int TournamentLeaderboardModel::getColumnForDailyNetPoints(int dayNum) const
{
    const std::unordered_map<int, int> columnMap = {{1, 4}, {2, 6}, {3, 8}};
    return columnMap.contains(dayNum) ? columnMap.at(dayNum) : -1;
}
