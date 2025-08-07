/**
 * @file DailyLeaderboardModel.cpp
 * @brief Implements the DailyLeaderboardModel class.
 */

#include "DailyLeaderboardModel.h"
#include "utils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>
#include <algorithm>

DailyLeaderboardModel::DailyLeaderboardModel(const QString &connectionName, int dayNum, QObject *parent)
    : QAbstractTableModel(parent)
    , m_connectionName(connectionName)
    , m_dayNum(dayNum)
{
    // Data is loaded on demand via refreshData()
}

DailyLeaderboardModel::~DailyLeaderboardModel()
{
}

QSqlDatabase DailyLeaderboardModel::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

int DailyLeaderboardModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_leaderboardData.size();
}

int DailyLeaderboardModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4; // Rank, Player Name, Daily Total Points, Daily Net Points
}

QVariant DailyLeaderboardModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= columnCount()) {
        return QVariant();
    }

    const DailyLeaderboardRow* rowData = getLeaderboardRow(index.row());
    if (!rowData) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        if (index.column() == getColumnForRank()) return rowData->rank;
        if (index.column() == getColumnForPlayerName()) return rowData->playerName;
        if (index.column() == getColumnForDailyTotalPoints()) return rowData->dailyTotalPoints;
        if (index.column() == getColumnForDailyNetPoints()) return rowData->dailyNetPoints;
    }

    if (role == Qt::TextAlignmentRole) {
        return static_cast<int>(Qt::AlignCenter);
    }

    return QVariant();
}

QVariant DailyLeaderboardModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section == getColumnForRank()) return "Rank";
        if (section == getColumnForPlayerName()) return "Player";
        if (section == getColumnForDailyTotalPoints()) return QString("Day %1 Total").arg(m_dayNum);
        if (section == getColumnForDailyNetPoints()) return QString("Day %1 Net").arg(m_dayNum);
    }

    if (role == Qt::TextAlignmentRole && orientation == Qt::Horizontal) {
        return static_cast<int>(Qt::AlignCenter);
    }

    return QVariant();
}

void DailyLeaderboardModel::refreshData()
{
    beginResetModel();

    m_allPlayers.clear();
    m_allHoleDetails.clear();
    m_dailyScores.clear();
    m_leaderboardData.clear();

    fetchAllPlayers();
    fetchAllHoleDetails();
    fetchDailyScores();

    calculateLeaderboard();

    endResetModel();
}

/**
 * @brief Fetches all active players from the database.
 */
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

/**
 * @brief Fetches details (par, handicap) for all holes on all courses.
 */
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

/**
 * @brief Fetches scores for active players for the specific day of this model.
 */
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
    QString queryString = QString("SELECT player_id, course_id, hole_num, score FROM scores WHERE player_id IN (%1) AND day_num = %2 ORDER BY player_id, hole_num;")
                          .arg(activePlayerIds.join(",")).arg(m_dayNum);

    if (query.exec(queryString)) {
        while (query.next()) {
            int playerId = query.value("player_id").toInt();
            int courseId = query.value("course_id").toInt();
            int holeNum = query.value("hole_num").toInt();
            int score = query.value("score").toInt();

            m_dailyScores[playerId][holeNum] = qMakePair(score, courseId);
        }
    } else {
        qDebug() << "DailyLeaderboardModel::fetchDailyScores: ERROR executing query:" << query.lastError().text();
    }
}

/**
 * @brief Calculates the Stableford points and ranks the players for the day.
 */
void DailyLeaderboardModel::calculateLeaderboard()
{
    m_leaderboardData.clear();

    for (auto const& [playerId, playerInfo] : m_allPlayers.asKeyValueRange()) {
        if (m_dailyScores.contains(playerId)) {
            DailyLeaderboardRow row;
            row.playerId = playerId;
            row.playerName = playerInfo.name;
            row.dailyTotalPoints = 0;
            row.dailyNetPoints = 0;

            for (auto const& [holeNum, scoreAndCourse] : m_dailyScores[playerId].asKeyValueRange()) {
                int score = scoreAndCourse.first;
                int courseIdForScore = scoreAndCourse.second;

                if (m_allHoleDetails.contains(qMakePair(courseIdForScore, holeNum))) {
                    int par = m_allHoleDetails[qMakePair(courseIdForScore, holeNum)].first;
                    if (stableford_conversion.contains(score - par)) {
                        row.dailyTotalPoints += stableford_conversion.at(score - par);
                    } else {
                        qDebug() << "Invalid score " << score << "on par " << par << "for " << playerInfo.name;
                    }
                } else {
                     qDebug() << QString("DailyLeaderboardModel::calculateLeaderboard (Day %1): Warning: Hole details not found for Course %2 Hole %3").arg(m_dayNum).arg(courseIdForScore).arg(holeNum);
                }
            }

            row.dailyNetPoints = row.dailyTotalPoints - playerInfo.handicap;
            m_leaderboardData.append(row);
        }
    }

    std::sort(m_leaderboardData.begin(), m_leaderboardData.end(),
              [](const DailyLeaderboardRow& a, const DailyLeaderboardRow& b) {
                  return a.dailyNetPoints > b.dailyNetPoints;
              });

    if (!m_leaderboardData.isEmpty()) {
        m_leaderboardData[0].rank = 1;
        for (int i = 1; i < m_leaderboardData.size(); ++i) {
            if (m_leaderboardData[i].dailyNetPoints == m_leaderboardData[i-1].dailyNetPoints) {
                m_leaderboardData[i].rank = m_leaderboardData[i-1].rank;
            } else {
                m_leaderboardData[i].rank = i + 1;
            }
        }
    }
}

const DailyLeaderboardRow* DailyLeaderboardModel::getLeaderboardRow(int row) const
{
    if (row >= 0 && row < m_leaderboardData.size()) {
        return &m_leaderboardData.at(row);
    }
    return nullptr;
}
