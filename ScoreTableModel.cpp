/**
 * @file ScoreTableModel.cpp
 * @brief Implements the ScoreTableModel class.
 */

#include "ScoreTableModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>

ScoreTableModel::ScoreTableModel(const QString &connectionName, int dayNum, QObject *parent)
    : QAbstractTableModel(parent), m_connectionName(connectionName), m_dayNum(dayNum), m_currentCourseId(-1)
{
    loadActivePlayers();
}

ScoreTableModel::~ScoreTableModel()
{
}

QSqlDatabase ScoreTableModel::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

int ScoreTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_activePlayers.size();
}

int ScoreTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1 + 18; // Player Name + 18 Holes
}

QVariant ScoreTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= columnCount()) {
        return QVariant();
    }

    const PlayerInfo *player = getPlayerInfo(index.row());
    if (!player) {
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        if (index.column() == 0) {
            return player->name;
        }

        if (index.column() >= 1 && index.column() <= 18) {
            int holeNum = getHoleForColumn(index.column());
            if (m_scores.contains(player->id) && m_scores[player->id].contains(holeNum)) {
                int score = m_scores[player->id][holeNum];
                if (score <= 0) {
                    return QVariant();
                }
                return score;
            } else {
                return QVariant();
            }
        }
    }

    if (role == Qt::TextAlignmentRole && index.column() >= 1 && index.column() <= 18) {
        return Qt::AlignCenter;
    }

    return QVariant();
}

QVariant ScoreTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }

    if (section == 0) {
        return "Player";
    } else if (section >= 1 && section <= 18) {
        int holeNum = getHoleForColumn(section);
        if (!m_holeDetails.contains(holeNum)) {
            return QString("Hole %1").arg(holeNum);
        }
        return QString("Hole %1\n(Par %2)").arg(holeNum).arg(m_holeDetails[holeNum].first);
    } else if (section == 19) {
        return "Total";
    }

    return QVariant();
}

Qt::ItemFlags ScoreTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);
    if (index.isValid() && index.row() >= 0 && index.row() < rowCount() &&
        index.column() >= 0 && index.column() < columnCount()) {
        if (index.column() >= 1 && index.column() <= 18) {
            return defaultFlags | Qt::ItemIsEditable;
        }
    }
    return defaultFlags;
}

bool ScoreTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 1 || index.column() > 18) {
        return false;
    }

    const PlayerInfo *player = getPlayerInfo(index.row());
    if (!player) {
        qDebug() << "ScoreTableModel::setData: ERROR: Could not get player info for row" << index.row();
        return false;
    }

    int holeNum = getHoleForColumn(index.column());
    bool ok;
    int newScore = value.toInt(&ok);

    if (!ok || newScore <= 0) {
        qDebug() << "ScoreTableModel::setData: Invalid score value entered:" << value.toString();
        return false;
    }

    int oldScore = m_scores[player->id][holeNum];
    m_scores[player->id][holeNum] = newScore;

    if (oldScore == newScore) {
        return true;
    }

    bool saveSuccess = saveScore(player->id, holeNum, newScore);
    if (!saveSuccess) {
        m_scores[player->id][holeNum] = oldScore;
        emit dataChanged(index, index, {role});
        qDebug() << "ScoreTableModel::setData: ERROR: Database save failed for Player" << player->id << "Hole" << holeNum;
        return false;
    }

    emit dataChanged(index, index, {role});
    return true;
}

void ScoreTableModel::setCourseId(int courseId)
{
    if (m_currentCourseId == courseId || courseId <= 0) {
        if (m_currentCourseId > 0) {
            beginResetModel();
            m_currentCourseId = -1;
            m_holeDetails.clear();
            m_scores.clear();
            endResetModel();
        } else {
            m_currentCourseId = courseId;
        }
        return;
    }

    m_currentCourseId = courseId;
    beginResetModel();
    m_holeDetails.clear();
    m_scores.clear();
    loadHoleDetails(m_currentCourseId);
    loadScores(m_currentCourseId);
    endResetModel();
}

/**
 * @brief Loads the list of active players from the database.
 */
void ScoreTableModel::loadActivePlayers()
{
    m_activePlayers.clear();
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "ScoreTableModel::loadActivePlayers: ERROR: Invalid or closed database connection.";
        return;
    }

    QSqlQuery query(db);
    if (query.exec("SELECT id, name, handicap FROM players WHERE active = 1 ORDER BY name")) {
        while (query.next()) {
            PlayerInfo player;
            player.id = query.value("id").toInt();
            player.name = query.value("name").toString();
            player.handicap = query.value("handicap").toInt();
            m_activePlayers.append(player);
        }
    } else {
        qDebug() << "ScoreTableModel::loadActivePlayers: ERROR executing query:" << query.lastError().text();
    }
}

/**
 * @brief Loads hole details (par, handicap) for a given course.
 * @param courseId The ID of the course to load.
 */
void ScoreTableModel::loadHoleDetails(int courseId)
{
    m_holeDetails.clear();
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "ScoreTableModel::loadHoleDetails: ERROR: Invalid or closed database connection.";
        return;
    }

    QSqlQuery query(db);
    query.prepare("SELECT hole_num, par, handicap FROM holes WHERE course_id = :cid ORDER BY hole_num;");
    query.bindValue(":cid", courseId);

    if (query.exec()) {
        while (query.next()) {
            int holeNum = query.value("hole_num").toInt();
            int par = query.value("par").toInt();
            int handicap = query.value("handicap").toInt();
            m_holeDetails[holeNum] = qMakePair(par, handicap);
        }
    } else {
        qDebug() << "ScoreTableModel::loadHoleDetails: ERROR executing query:" << query.lastError().text();
    }
}

/**
 * @brief Loads existing scores for the current day, course, and active players.
 * @param courseId The ID of the course to load.
 */
void ScoreTableModel::loadScores(int courseId)
{
    m_scores.clear();
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "ScoreTableModel::loadScores: ERROR: Invalid or closed database connection.";
        return;
    }

    QStringList activePlayerIds;
    for (const auto &player : m_activePlayers) {
        activePlayerIds << QString::number(player.id);
    }

    if (activePlayerIds.isEmpty()) {
        qDebug() << "ScoreTableModel::loadScores: No active players to load scores for.";
        return;
    }

    QSqlQuery query(db);
    QString queryString = QString("SELECT player_id, hole_num, score FROM scores WHERE day_num = %1 AND course_id = %2 AND player_id IN (%3);")
                              .arg(m_dayNum)
                              .arg(courseId)
                              .arg(activePlayerIds.join(","));

    if (query.exec(queryString)) {
        while (query.next()) {
            int playerId = query.value("player_id").toInt();
            int holeNum = query.value("hole_num").toInt();
            int score = query.value("score").toInt();
            m_scores[playerId][holeNum] = score;
        }
    } else {
        qDebug() << "ScoreTableModel::loadScores: ERROR executing query:" << query.lastError().text();
    }
}

const PlayerInfo *ScoreTableModel::getPlayerInfo(int row) const
{
    if (row >= 0 && row < m_activePlayers.size()) {
        return &m_activePlayers.at(row);
    }
    return nullptr;
}

int ScoreTableModel::getColumnForHole(int holeNum) const
{
    if (holeNum >= 1 && holeNum <= 18) {
        return holeNum;
    }
    return -1;
}

int ScoreTableModel::getHoleForColumn(int column) const
{
    if (column >= 1 && column <= 18) {
        return column;
    }
    return -1;
}

/**
 * @brief Saves a single score to the database.
 * @param playerId The ID of the player.
 * @param holeNum The hole number.
 * @param score The score to save.
 * @return True if the score was saved successfully, false otherwise.
 */
bool ScoreTableModel::saveScore(int playerId, int holeNum, int score)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "ScoreTableModel::saveScore: ERROR: Invalid or closed database connection.";
        return false;
    }

    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO scores (player_id, course_id, hole_num, day_num, score) "
                  "VALUES (:pid, :cid, :hnum, :dnum, :score);");
    query.bindValue(":pid", playerId);
    query.bindValue(":cid", m_currentCourseId);
    query.bindValue(":hnum", holeNum);
    query.bindValue(":dnum", m_dayNum);
    query.bindValue(":score", score);

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "ScoreTableModel::saveScore: ERROR executing query:" << query.lastError().text();
        return false;
    }
}
