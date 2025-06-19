#include "TournamentLeaderboardModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>
#include <algorithm>
#include <cmath> // For std::floor

TournamentLeaderboardModel::TournamentLeaderboardModel(const QString &connectionName, QObject *parent)
    : QAbstractTableModel(parent), m_connectionName(connectionName) // Store the passed connection name
      ,
      m_tournamentContext(TwistedCreek) // Default context, dialog will override
      ,
      m_cutLineScore(0), m_isCutApplied(false)
{
}

TournamentLeaderboardModel::~TournamentLeaderboardModel() {}

// This is the critical method for getting the database connection.
QSqlDatabase TournamentLeaderboardModel::database() const
{
    // Attempt to retrieve the database connection by the stored name.
    // The 'false' argument means: do NOT add a new connection if one by this name isn't found.
    // This helps ensure we are checking the status of the *intended* connection.
    QSqlDatabase db_check = QSqlDatabase::database(m_connectionName, false);

    if (!db_check.isValid())
    {
        qWarning() << "TournamentLeaderboardModel instance" << this
                   << "DB connection name '" << m_connectionName << "' is NOT VALID (not found in Qt's list). "
                   << "Available connections:" << QSqlDatabase::connectionNames();
    }
    else
    {
        if (!db_check.isOpen())
        {
            qWarning() << "TournamentLeaderboardModel instance" << this
                       << "DB connection '" << m_connectionName << "' is VALID but NOT OPEN. Last error:" << db_check.lastError().text();
        }
    }
    return db_check; // Return the retrieved (and possibly not open) connection.
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
    // Rank, Player, ActualHcp, D1 Gross, D1 Net, D2 Gross, D2 Net, D3 Gross, D3 Net, Overall Net Pts
    return 10;
}

QVariant TournamentLeaderboardModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= columnCount())
    {
        return QVariant();
    }

    const LeaderboardRow &rowData = m_leaderboardData.at(index.row());

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case 0:
            return rowData.rank > 0 ? QVariant(rowData.rank) : QVariant("-");
        case 1:
            return rowData.playerName;
        case 2:
            return rowData.playerActualDbHandicap;
        case 3:
            return rowData.dailyGrossStablefordPoints.contains(1) ? QVariant(rowData.dailyGrossStablefordPoints.value(1)) : QVariant();
        case 4:
            return rowData.dailyNetStablefordPoints.contains(1) ? QVariant(rowData.dailyNetStablefordPoints.value(1)) : QVariant();
        case 5:
            return rowData.dailyGrossStablefordPoints.contains(2) ? QVariant(rowData.dailyGrossStablefordPoints.value(2)) : QVariant();
        case 6:
            return rowData.dailyNetStablefordPoints.contains(2) ? QVariant(rowData.dailyNetStablefordPoints.value(2)) : QVariant();
        case 7:
            return rowData.dailyGrossStablefordPoints.contains(3) ? QVariant(rowData.dailyGrossStablefordPoints.value(3)) : QVariant();
        case 8:
            return rowData.dailyNetStablefordPoints.contains(3) ? QVariant(rowData.dailyNetStablefordPoints.value(3)) : QVariant();
        case 9:
            return rowData.totalNetStablefordPoints;
        default:
            return QVariant();
        }
    }
    else if (role == Qt::TextAlignmentRole)
    {
        return QVariant(Qt::AlignCenter);
    }
    return QVariant();
}

QVariant TournamentLeaderboardModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return "Rank";
        case 1:
            return "Player";
        case 2:
            return "Point Target";
        case 3:
            return "Day 1 Gross";
        case 4:
            return "Day 1 Net";
        case 5:
            return "Day 2 Gross";
        case 6:
            return "Day 2 Net";
        case 7:
            return "Day 3 Gross";
        case 8:
            return "Day 3 Net";
        case 9:
            return "Overall Net";
        default:
            return QVariant();
        }
    }
    if (role == Qt::TextAlignmentRole && orientation == Qt::Horizontal)
    {
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

    fetchAllPlayers(); // This will call database() and check if it's open
    if (m_allPlayers.isEmpty())
    { // If DB wasn't open, fetchAllPlayers might return early
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllPlayers resulted in empty m_allPlayers. Further calculations might be skipped or incorrect.";
    }

    fetchAllHoleDetails();
    fetchAllScores();

    calculateAllPlayerTwoDayMosleyNetScores();
    calculateLeaderboard();

    endResetModel();
    if (m_leaderboardData.isEmpty() && !m_allPlayers.isEmpty())
    {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- WARNING: m_leaderboardData is empty but m_allPlayers is not. Check filtering logic in calculateLeaderboard.";
    }
    else if (m_allPlayers.isEmpty() && (m_tournamentContext == MosleyOpen || m_tournamentContext == TwistedCreek))
    { // Expect players if context is set
        qDebug() << "TournamentLeaderboardModel instance" << this << "- WARNING: m_allPlayers is empty. No players fetched. Leaderboard will be empty.";
    }
}

QSet<int> TournamentLeaderboardModel::getDaysWithScores() const
{
    return m_daysWithScores;
}

void TournamentLeaderboardModel::fetchAllPlayers()
{
    QSqlDatabase db = database(); // Calls the updated database() method
    if (!db.isValid() || !db.isOpen())
    { // Check is now more critical based on the updated database()
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllPlayers: DB connection from database() is not valid or not open. Aborting fetch.";
        return; // Critical to return if DB is not usable
    }
    QSqlQuery query(db);
    if (query.exec("SELECT id, name, handicap FROM players WHERE active = 1"))
    {
        while (query.next())
        {
            PlayerInfo player;
            player.id = query.value("id").toInt();
            player.name = query.value("name").toString();
            player.handicap = query.value("handicap").toInt();
            m_allPlayers[player.id] = player;
        }
    }
    else
    {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllPlayers: SQL ERROR:" << query.lastError().text();
    }
}

void TournamentLeaderboardModel::fetchAllHoleDetails()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
    {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllHoleDetails: DB not open. Skipping.";
        return;
    }
    QSqlQuery query(db);
    if (query.exec("SELECT course_id, hole_num, par, handicap FROM holes"))
    {
        while (query.next())
        {
            m_holeParAndHandicapIndex[qMakePair(query.value("course_id").toInt(), query.value("hole_num").toInt())] =
                qMakePair(query.value("par").toInt(), query.value("handicap").toInt());
        }
    }
    else
    {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllHoleDetails: SQL ERROR:" << query.lastError().text();
    }
}

void TournamentLeaderboardModel::fetchAllScores()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
    {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllScores: DB not open. Skipping.";
        return;
    }
    QSqlQuery query(db);
    if (query.exec("SELECT player_id, course_id, hole_num, day_num, score FROM scores"))
    {
        while (query.next())
        {
            int playerId = query.value("player_id").toInt();
            int dayNum = query.value("day_num").toInt();
            int holeNum = query.value("hole_num").toInt();
            int scoreVal = query.value("score").toInt();
            int courseId = query.value("course_id").toInt();
            m_allScores[playerId][dayNum][holeNum] = qMakePair(scoreVal, courseId);
            m_daysWithScores.insert(dayNum);
        }
    }
    else
    {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- fetchAllScores: SQL ERROR:" << query.lastError().text();
    }
}

int TournamentLeaderboardModel::calculateNetStablefordPointsForHole(
    int grossScore, int par, int playerActualDatabaseHandicap,
    int holeHandicapIndex, TournamentContext contextForCalc) const
{
    if (grossScore <= 0 || par <= 0)
        return 0;

    int handicapForThisCalc = playerActualDatabaseHandicap;

    if (contextForCalc == MosleyOpen)
    {
        if (playerActualDatabaseHandicap >= 6 && playerActualDatabaseHandicap <= 15)
        {
            handicapForThisCalc = 16;
        }
    }

    int strokesReceivedOnThisHole = 0;
    if (handicapForThisCalc <= 36)
    {
        int effectiveStrokesForRound = 36 - handicapForThisCalc;
        if (effectiveStrokesForRound >= holeHandicapIndex)
            strokesReceivedOnThisHole++;
        if (effectiveStrokesForRound >= (18 + holeHandicapIndex))
            strokesReceivedOnThisHole++;
        if (effectiveStrokesForRound >= (36 + holeHandicapIndex))
            strokesReceivedOnThisHole++;
    }
    else
    {
        double strokesToGiveBackTotalDouble = (static_cast<double>(handicapForThisCalc) - 36.0) / 2.0;
        int strokesToGiveBackTotal = static_cast<int>(std::floor(strokesToGiveBackTotalDouble));
        if (holeHandicapIndex > (18 - strokesToGiveBackTotal))
            strokesReceivedOnThisHole = -1;
    }
    int netScore = grossScore - strokesReceivedOnThisHole;
    int diff = grossScore - par;

    if (diff <= -3)
        return 8;
    if (diff == -2)
        return 6;
    if (diff == -1)
        return 4;
    if (diff == 0)
        return 2;
    if (diff == 1)
        return 1;
    if (diff == 2)
        return 0;
    if (diff >= 3)
        return -1;
    return 0;
}

void TournamentLeaderboardModel::calculateAllPlayerTwoDayMosleyNetScores()
{
    m_playerTwoDayMosleyNetScoreForCut.clear();
    if (m_allPlayers.isEmpty())
    {
        qDebug() << "TournamentLeaderboardModel instance" << this << "- calculateAllPlayerTwoDayMosleyNetScores: m_allPlayers is empty. Skipping.";
        return;
    }
    for (auto const &[playerId, playerInfo] : m_allPlayers.asKeyValueRange())
    {
        int twoDayTotalNetForPlayer = 0;
        for (int dayNum = 1; dayNum <= 2; ++dayNum)
        {
            if (m_allScores.contains(playerId) && m_allScores[playerId].contains(dayNum))
            {
                for (auto const &[holeNum, scoreAndCourse] : m_allScores[playerId][dayNum].asKeyValueRange())
                {
                    int grossScore = scoreAndCourse.first;
                    int courseIdForScore = scoreAndCourse.second;
                    QPair<int, int> holeDetailsKey = qMakePair(courseIdForScore, holeNum);
                    if (m_holeParAndHandicapIndex.contains(holeDetailsKey))
                    {
                        int par = m_holeParAndHandicapIndex[holeDetailsKey].first;
                        int holeHcIdx = m_holeParAndHandicapIndex[holeDetailsKey].second;
                        twoDayTotalNetForPlayer += calculateNetStablefordPointsForHole(
                            grossScore, par, playerInfo.handicap, holeHcIdx, MosleyOpen);
                    }
                }
                twoDayTotalNetForPlayer -= playerInfo.handicap;
            }
        }
        m_playerTwoDayMosleyNetScoreForCut[playerId] = twoDayTotalNetForPlayer;
    }
}

void TournamentLeaderboardModel::calculateLeaderboard()
{
    m_leaderboardData.clear();

    for (auto const &[playerId, playerInfo] : m_allPlayers.asKeyValueRange())
    {
        bool includePlayer = false;
        int playerTwoDayMosleyScore = m_playerTwoDayMosleyNetScoreForCut.value(playerId, 0);

        if (m_isCutApplied)
        {
            if (m_tournamentContext == MosleyOpen)
            {
                if (playerTwoDayMosleyScore >= m_cutLineScore)
                    includePlayer = true;
            }
            else if (m_tournamentContext == TwistedCreek)
            {
                if (playerTwoDayMosleyScore < m_cutLineScore)
                    includePlayer = true;
            }
        }
        else
            includePlayer = true;

        if (includePlayer)
        {
            LeaderboardRow row;
            row.playerId = playerId;
            row.playerName = playerInfo.name;
            row.playerActualDbHandicap = playerInfo.handicap;
            row.totalNetStablefordPoints = 0;
            row.twoDayMosleyNetScoreForCut = playerTwoDayMosleyScore;

            for (int dayNum = 1; dayNum <= 3; ++dayNum)
            {
                int dailyGrossPts = 0;
                bool scoresExistForDay = false;

                if (m_allScores.contains(playerId) && m_allScores[playerId].contains(dayNum))
                {
                    scoresExistForDay = true;
                    for (auto const &[holeNum, scoreAndCourse] : m_allScores[playerId][dayNum].asKeyValueRange())
                    {
                        int grossScore = scoreAndCourse.first;
                        int courseIdForScore = scoreAndCourse.second;
                        QPair<int, int> holeDetailsKey = qMakePair(courseIdForScore, holeNum);
                        if (m_holeParAndHandicapIndex.contains(holeDetailsKey))
                        {
                            int par = m_holeParAndHandicapIndex[holeDetailsKey].first;
                            int holeHcIdx = m_holeParAndHandicapIndex[holeDetailsKey].second;

                            int grossStablefordDiff = grossScore - par;
                            if (grossStablefordDiff <= -3)
                                dailyGrossPts += 8;
                            else if (grossStablefordDiff == -2)
                                dailyGrossPts += 6;
                            else if (grossStablefordDiff == -1)
                                dailyGrossPts += 4;
                            else if (grossStablefordDiff == 0)
                                dailyGrossPts += 2;
                            else if (grossStablefordDiff == 1)
                                dailyGrossPts += 1;
                            else if (grossStablefordDiff == 2)
                                dailyGrossPts += 0;
                            else if (grossStablefordDiff >= 3)
                                dailyGrossPts += -1;
                        }
                    }
                }
                if (scoresExistForDay)
                {
                    row.dailyGrossStablefordPoints[dayNum] = dailyGrossPts;
                    row.dailyNetStablefordPoints[dayNum] = dailyGrossPts - playerInfo.handicap;
                    row.totalNetStablefordPoints += row.dailyNetStablefordPoints[dayNum];
                }
            }
            m_leaderboardData.append(row);
        }
    }

    std::sort(m_leaderboardData.begin(), m_leaderboardData.end(),
              [](const LeaderboardRow &a, const LeaderboardRow &b)
              {
                  return a.totalNetStablefordPoints > b.totalNetStablefordPoints;
              });

    if (!m_leaderboardData.isEmpty())
    {
        m_leaderboardData[0].rank = 1;
        for (int i = 1; i < m_leaderboardData.size(); ++i)
        {
            if (m_leaderboardData[i].totalNetStablefordPoints == m_leaderboardData[i - 1].totalNetStablefordPoints)
                m_leaderboardData[i].rank = m_leaderboardData[i - 1].rank;
            else
                m_leaderboardData[i].rank = i + 1;
        }
    }
}

int TournamentLeaderboardModel::getColumnForDailyGrossPoints(int dayNum) const
{
    if (dayNum == 1)
        return 3;
    if (dayNum == 2)
        return 5;
    if (dayNum == 3)
        return 7;
    return -1;
}

int TournamentLeaderboardModel::getColumnForDailyNetPoints(int dayNum) const
{
    if (dayNum == 1)
        return 4;
    if (dayNum == 2)
        return 6;
    if (dayNum == 3)
        return 8;
    return -1;
}
