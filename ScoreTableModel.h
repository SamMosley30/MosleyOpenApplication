#ifndef SCORETABLEMODEL_H
#define SCORETABLEMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QMap>
#include <QPair>
#include <QDebug> // For debugging

#include "CommonStructs.h"

// Structure to hold a specific score entry
// We'll store scores internally in a way that's easy to look up by player and hole
// For example, a map where key is player ID, value is another map
// where key is hole number, value is the score.

class ScoreTableModel : public QAbstractTableModel
{
    Q_OBJECT // Required for signals and slots

public:
    // Constructor takes the database connection name and the tournament day number
    explicit ScoreTableModel(const QString &connectionName, int dayNum, QObject *parent = nullptr);
    ~ScoreTableModel();

    // === Required QAbstractTableModel methods ===
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // === Methods for editing ===
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // === Public methods to load data ===
    // This method will trigger loading players, hole details, and scores for the given course
    void setCourseId(int courseId);

    // === Optional: Methods for saving/reverting changes manually ===
    // bool submitAll(); // If using manual submit strategy
    // void revertAll(); // If using manual submit strategy

private:
    QString m_connectionName; // Stores the name of the database connection
    int m_dayNum;             // Stores the tournament day number (1, 2, or 3)
    int m_currentCourseId;    // Stores the ID of the currently selected course

    QVector<PlayerInfo> m_activePlayers; // Stores the list of active players (rows)
    QMap<int, QPair<int, int>> m_holeDetails; // Stores hole details: Map<HoleNum, Pair<Par, Handicap>> (used for column headers/info)

    // Stores scores: Map<PlayerId, Map<HoleNum, Score>>
    // A nested map allows quick lookup: m_scores[playerId][holeNum]
    QMap<int, QMap<int, int>> m_scores; // Scores: Map<PlayerId, Map<HoleNum, Score>> (Score can be 0 or -1 for unentered)

    // === Private helper methods ===
    QSqlDatabase database() const; // Helper to get database connection by name

    // Methods to load data from the database
    void loadActivePlayers();
    void loadHoleDetails(int courseId);
    void loadScores(int courseId);

    // Helper to get PlayerInfo by row index
    const PlayerInfo* getPlayerInfo(int row) const;

    // Helper to get the column index for a specific hole number
    int getColumnForHole(int holeNum) const;
    // Helper to get the hole number for a specific column index
    int getHoleForColumn(int column) const;

    // Helper to save a single score to the database
    bool saveScore(int playerId, int holeNum, int score);
};

#endif // SCORETABLEMODEL_H