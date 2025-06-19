#include "ScoreTableModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord> // To get field names from query
#include <QSqlField>  // To check field validity

// Constructor
ScoreTableModel::ScoreTableModel(const QString &connectionName, int dayNum, QObject *parent)
    : QAbstractTableModel(parent), m_connectionName(connectionName), m_dayNum(dayNum), m_currentCourseId(-1) // Initialize with an invalid ID
{
    // Load the list of active players when the model is created
    loadActivePlayers();
}

// Destructor
ScoreTableModel::~ScoreTableModel()
{
}

// Helper to get the database connection by name
QSqlDatabase ScoreTableModel::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

// === Required QAbstractTableModel methods implementation ===

// Returns the number of rows (active players)
int ScoreTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);              // Parent index is not used for a flat table model
    return m_activePlayers.size(); // One row per active player
}

// Returns the number of columns (Player Name + 18 Holes + potentially Total/VsPar)
int ScoreTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent); // Parent index is not used for a flat table model
    // Columns: Player Name (1) + 18 Holes (18) = 19
    // Add columns for Total Score (1) and Score vs Par (1) later if needed, making it 21
    return 1 + 18; // Player Name column + 18 Hole columns
}

// Returns the data for a specific cell
QVariant ScoreTableModel::data(const QModelIndex &index, int role) const
{
    // Check if the index is valid and within the model's bounds
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= columnCount())
        return QVariant(); // Return an invalid QVariant for invalid indices

    // Get the PlayerInfo for the current row
    const PlayerInfo *player = getPlayerInfo(index.row());
    if (!player)
        return QVariant(); // Should not happen if index is valid, but safety check

    // Handle DisplayRole and EditRole
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        // Column 0 is the Player Name
        if (index.column() == 0)
            return player->name; // Display player name

        // Columns 1-18 are for Hole Scores
        if (index.column() >= 1 && index.column() <= 18)
        {
            int holeNum = getHoleForColumn(index.column()); // Get the hole number for this column

            // Look up the score for this player and hole
            if (m_scores.contains(player->id) && m_scores[player->id].contains(holeNum))
            {
                int score = m_scores[player->id][holeNum];
                if (score <= 0)
                    return QVariant(); // Return empty for unentered scores
                
                return score; // Return the score
            }
            else
                return QVariant(); // Return empty if no score found for this player/hole
        }
    }

    // Handle other roles (e.g., TextAlignmentRole for score columns)
    if (role == Qt::TextAlignmentRole && index.column() >= 1 && index.column() <= 18)
        return Qt::AlignCenter; // Center align scores

    return QVariant(); // Return an invalid QVariant for roles we don't handle
}

// Returns the header data for rows and columns
QVariant ScoreTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // We only handle DisplayRole for headers
    if (role != Qt::DisplayRole or orientation != Qt::Horizontal)
        return QVariant(); // Return an invalid QVariant for roles/orientations we don't handle

    // Section is the column index (0 to columnCount() - 1)
    if (section == 0)
        return "Player"; // Header for the player name column
    else if (section >= 1 && section <= 18)
    {
        int holeNum = getHoleForColumn(section); // Get the hole number for this column
        if (!m_holeDetails.contains(holeNum))
            return QString("Hole %1").arg(holeNum); // Fallback if hole details not loaded

        return QString("Hole %1\n(Par %2)").arg(holeNum).arg(m_holeDetails[holeNum].first);
    }
    // Add headers for Total Score, Score vs Par columns later
    else if (section == 19)
        return "Total";

    return QVariant(); // Return an invalid QVariant for roles/orientations we don't handle
}

// === Methods for editing ===

// Returns flags for each item (cell)
Qt::ItemFlags ScoreTableModel::flags(const QModelIndex &index) const
{
    // Start with the default flags
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

    // Check if the index is valid and within the data area we want to make editable
    if (index.isValid() && index.row() >= 0 && index.row() < rowCount() &&
        index.column() >= 0 && index.column() < columnCount())
    {
        // Make the score columns (1-18) editable
        if (index.column() >= 1 && index.column() <= 18)
            return defaultFlags | Qt::ItemIsEditable; // Add the editable flag
    }

    return defaultFlags; // Return default flags for non-editable items (like Player Name column)
}

// Sets the data for a specific cell
bool ScoreTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // Check if the index is valid, the role is EditRole, and it's a score column
    if (!index.isValid() || role != Qt::EditRole || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 1 || index.column() > 18) // Only editable columns (scores)
        return false; // Data setting failed

    // Get the PlayerInfo for the current row
    const PlayerInfo *player = getPlayerInfo(index.row());
    if (!player)
    {
        qDebug() << "ScoreTableModel::setData: ERROR: Could not get player info for row" << index.row();
        return false; // Data setting failed
    }

    // Get the hole number for the current column
    int holeNum = getHoleForColumn(index.column());

    // Get the new score value as an integer
    bool ok;
    int newScore = value.toInt(&ok);

    // Check if the conversion to int was successful and the score is reasonable (e.g., >= 1)
    if (!ok || newScore <= 0)
    { // Assuming scores must be positive integers
        qDebug() << "ScoreTableModel::setData: Invalid score value entered:" << value.toString();
        return false; // Data setting failed due to invalid value
    }

    // Update the score in the internal data structure
    // Use [] operator for convenience, it will insert if keys don't exist
    int oldScore = m_scores[player->id][holeNum]; // Get old score before updating
    m_scores[player->id][holeNum] = newScore;

    // Check if the data actually changed to avoid unnecessary database writes and signals
    if (oldScore == newScore)
        return true; // Indicate data setting was successful (no change needed)

    // --- Save the score to the database ---
    bool saveSuccess = saveScore(player->id, holeNum, newScore);
    if (!saveSuccess)
    {
        // Database save failed. Revert the change in the internal model data.
        m_scores[player->id][holeNum] = oldScore; // Revert internal data
        // Notify the view to revert the displayed value
        emit dataChanged(index, index, {role});
        qDebug() << "ScoreTableModel::setData: ERROR: Database save failed for Player" << player->id << "Hole" << holeNum;
        // The saveScore function should print the database error.
        return false; // Indicate data setting failed due to database error
    }

    // Emit dataChanged signal to notify the view that the data at this index has changed
    emit dataChanged(index, index, {role}); // Notify view of the successful change
    return true;                            // Indicate data setting and saving was successful
}

// === Public method to load data ===

void ScoreTableModel::setCourseId(int courseId)
{
    // Only reload if the course ID has changed and is valid (> 0)
    if (m_currentCourseId == courseId || courseId <= 0)
    {
        // If the ID hasn't changed or is invalid, but we previously had data,
        // we might need to clear the model.
        if (m_currentCourseId > 0)
        {                           // Clear if we had a valid course selected before
            beginResetModel();      // Notify view data is changing
            m_currentCourseId = -1; // Reset ID
            m_holeDetails.clear();  // Clear hole details
            m_scores.clear();       // Clear scores
            endResetModel();        // Notify view data has changed
        }
        else
        {
            m_currentCourseId = courseId; // Store the invalid ID if needed for state tracking
        }
        return; // Nothing to do if ID is the same or invalid for loading
    }

    m_currentCourseId = courseId; // Update the current course ID

    // Notify the view that the model's data is about to be reset
    // This is important because column headers (Hole X, Par Y) might change
    beginResetModel();

    // Clear previous data
    m_holeDetails.clear();
    m_scores.clear(); // Clear old scores

    // Load new data for the selected course and day
    loadHoleDetails(m_currentCourseId); // Load details for the 18 holes
    loadScores(m_currentCourseId);      // Load existing scores for active players

    // Notify the view that the model's data has been reset
    endResetModel();

    // Optional: If only scores change (not players or hole details),
    // you could use beginInsertRows/endInsertRows, beginRemoveRows/endRemoveRows,
    // or begin/endDataChanged for more granular updates instead of reset.
    // But for simplicity and handling course changes, reset is fine.
}

// === Private helper methods ===

// Loads the list of active players from the database
void ScoreTableModel::loadActivePlayers()
{
    m_activePlayers.clear(); // Clear existing players

    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
    {
        qDebug() << "ScoreTableModel::loadActivePlayers: ERROR: Invalid or closed database connection.";
        return;
    }

    QSqlQuery query(db);
    // Select players who are marked as active, ordered by name
    if (query.exec("SELECT id, name, handicap FROM players WHERE active = 1 ORDER BY name"))
    {
        while (query.next())
        {
            PlayerInfo player;
            player.id = query.value("id").toInt();
            player.name = query.value("name").toString();
            player.handicap = query.value("handicap").toInt();
            m_activePlayers.append(player);
        }
    }
    else
    {
        qDebug() << "ScoreTableModel::loadActivePlayers: ERROR executing query:" << query.lastError().text();
    }
    // Note: This method is called in the constructor. If players change later,
    // you'd need a way to trigger a reload of active players and emit begin/endResetModel.
}

// Loads hole details (Par, Handicap) for a given course
void ScoreTableModel::loadHoleDetails(int courseId)
{
    m_holeDetails.clear(); // Clear existing hole details

    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
    {
        qDebug() << "ScoreTableModel::loadHoleDetails: ERROR: Invalid or closed database connection.";
        return;
    }

    QSqlQuery query(db);
    // Select hole details for the course, ordered by hole number
    query.prepare("SELECT hole_num, par, handicap FROM holes WHERE course_id = :cid ORDER BY hole_num;");
    query.bindValue(":cid", courseId);

    if (query.exec())
    {
        while (query.next())
        {
            int holeNum = query.value("hole_num").toInt();
            int par = query.value("par").toInt();
            int handicap = query.value("handicap").toInt();
            m_holeDetails[holeNum] = qMakePair(par, handicap); // Store par and handicap by hole number
        }
    }
    else
    {
        qDebug() << "ScoreTableModel::loadHoleDetails: ERROR executing query:" << query.lastError().text();
    }
}

// Loads existing scores for the current day, course, and active players
void ScoreTableModel::loadScores(int courseId)
{
    m_scores.clear(); // Clear existing scores

    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
    {
        qDebug() << "ScoreTableModel::loadScores: ERROR: Invalid or closed database connection.";
        return;
    }

    // Build a list of active player IDs to use in the query
    QStringList activePlayerIds;
    for (const auto &player : m_activePlayers)
    {
        activePlayerIds << QString::number(player.id);
    }

    if (activePlayerIds.isEmpty())
    {
        qDebug() << "ScoreTableModel::loadScores: No active players to load scores for.";
        return; // No active players, nothing to load
    }

    QSqlQuery query(db);
    QString queryString = QString("SELECT player_id, hole_num, score FROM scores WHERE day_num = %1 AND course_id = %2 AND player_id IN (%3);")
                              .arg(m_dayNum)                   // Substitute day number
                              .arg(courseId)                   // Substitute course ID
                              .arg(activePlayerIds.join(",")); // Substitute the comma-separated list of IDs

    if (query.exec(queryString))
    {
        while (query.next())
        {
            int playerId = query.value("player_id").toInt();
            int holeNum = query.value("hole_num").toInt();
            int score = query.value("score").toInt(); // Get the score

            // Store the score in the nested map
            m_scores[playerId][holeNum] = score;
        }
    }
    else
    {
        qDebug() << "ScoreTableModel::loadScores: ERROR executing query:" << query.lastError().text();
    }
}

// Helper to get PlayerInfo by row index
const PlayerInfo *ScoreTableModel::getPlayerInfo(int row) const
{
    if (row >= 0 && row < m_activePlayers.size())
    {
        return &m_activePlayers.at(row);
    }
    return nullptr; // Invalid row
}

// Helper to get the column index for a specific hole number (1-18)
// Column 0 is Player Name, so Hole 1 is column 1, Hole 2 is column 2, etc.
int ScoreTableModel::getColumnForHole(int holeNum) const
{
    if (holeNum >= 1 && holeNum <= 18)
    {
        return holeNum; // Hole 1 is column 1, Hole 2 is column 2, etc.
    }
    return -1; // Invalid hole number
}

// Helper to get the hole number (1-18) for a specific column index
// Column 0 is Player Name, so column 1 is Hole 1, column 2 is Hole 2, etc.
int ScoreTableModel::getHoleForColumn(int column) const
{
    if (column >= 1 && column <= 18)
    {
        return column; // Column 1 is Hole 1, Column 2 is Hole 2, etc.
    }
    return -1; // Invalid column index for a hole
}

// Helper to save a single score to the database
bool ScoreTableModel::saveScore(int playerId, int holeNum, int score)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
    {
        qDebug() << "ScoreTableModel::saveScore: ERROR: Invalid or closed database connection.";
        return false;
    }

    QSqlQuery query(db);
    // Use INSERT OR REPLACE INTO to either insert a new score or update an existing one
    // based on the UNIQUE constraint (player_id, course_id, hole_num, day_num)
    query.prepare("INSERT OR REPLACE INTO scores (player_id, course_id, hole_num, day_num, score) "
                  "VALUES (:pid, :cid, :hnum, :dnum, :score);");
    query.bindValue(":pid", playerId);
    query.bindValue(":cid", m_currentCourseId);
    query.bindValue(":hnum", holeNum);
    query.bindValue(":dnum", m_dayNum);
    query.bindValue(":score", score);

    if (query.exec())
    {
        // Database save successful
        return true;
    }
    else
    {
        // Database save failed
        qDebug() << "ScoreTableModel::saveScore: ERROR executing query:" << query.lastError().text();
        return false;
    }
}
