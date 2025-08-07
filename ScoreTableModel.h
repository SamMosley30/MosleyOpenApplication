/**
 * @file ScoreTableModel.h
 * @brief Contains the declaration of the ScoreTableModel class.
 */

#ifndef SCORETABLEMODEL_H
#define SCORETABLEMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QMap>
#include <QPair>
#include <QDebug>

#include "CommonStructs.h"

/**
 * @class ScoreTableModel
 * @brief A model for entering and displaying scores for a single day.
 *
 * This model provides a table view with players as rows and holes as columns
 * for score entry. It fetches data from the database and handles saving
 * scores as they are entered.
 */
class ScoreTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a ScoreTableModel object.
     * @param connectionName The name of the database connection to use.
     * @param dayNum The day number (1, 2, or 3) this model represents.
     * @param parent The parent object.
     */
    explicit ScoreTableModel(const QString &connectionName, int dayNum, QObject *parent = nullptr);

    /**
     * @brief Destroys the ScoreTableModel object.
     */
    ~ScoreTableModel();

    // QAbstractTableModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    /**
     * @brief Sets the course ID to load data for.
     *
     * This method triggers loading players, hole details, and scores for the given course.
     *
     * @param courseId The ID of the course to load.
     */
    void setCourseId(int courseId);

private:
    QString m_connectionName; ///< The name of the database connection.
    int m_dayNum;             ///< The tournament day number (1, 2, or 3).
    int m_currentCourseId;    ///< The ID of the currently selected course.

    QVector<PlayerInfo> m_activePlayers; ///< The list of active players (rows).
    QMap<int, QPair<int, int>> m_holeDetails; ///< Map of HoleNum to <Par, Handicap>.
    QMap<int, QMap<int, int>> m_scores; ///< Map of PlayerId to a map of <HoleNum, Score>.

    QSqlDatabase database() const;
    void loadActivePlayers();
    void loadHoleDetails(int courseId);
    void loadScores(int courseId);
    const PlayerInfo* getPlayerInfo(int row) const;
    int getColumnForHole(int holeNum) const;
    int getHoleForColumn(int column) const;
    bool saveScore(int playerId, int holeNum, int score);
};

#endif // SCORETABLEMODEL_H