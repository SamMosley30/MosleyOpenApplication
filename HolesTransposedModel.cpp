#include "HolesTransposedModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

// Constructor
HolesTransposedModel::HolesTransposedModel(const QString &connectionName, QObject *parent)
    : QAbstractTableModel(parent)
    , m_connectionName(connectionName)
    , m_currentCourseId(-1) // Initialize with an invalid ID
{
    // Initialize m_holeData with 18 default HoleData structs
    // This ensures we always have 18 slots, even if a course has fewer holes (though your addCourse guarantees 18)
    m_holeData.resize(18);
    for(int i = 0; i < 18; ++i) {
        m_holeData[i].holeNum = i + 1; // Initialize hole numbers 1-18
        m_holeData[i].par = 0;       // Default values
        m_holeData[i].handicap = 0;
    }
}

// Destructor
HolesTransposedModel::~HolesTransposedModel()
{
    // No dynamic allocation of model members needed due to parent-child relationships or smart pointers/containers
}

// Helper to get the database connection by name
QSqlDatabase HolesTransposedModel::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

// === Required QAbstractTableModel methods implementation ===

// Returns the number of rows (attributes like Par, Handicap)
int HolesTransposedModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent); // Parent index is not used for a flat table model
    return 2; // We want 2 rows: one for Par, one for Handicap (add more rows for other attributes later)
}

// Returns the number of columns (holes 1 through 18)
int HolesTransposedModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent); // Parent index is not used for a flat table model
    return 18; // We want 18 columns: one for each hole
}

// Returns the data for a specific cell
QVariant HolesTransposedModel::data(const QModelIndex &index, int role) const
{
    // Check if the index is valid and within the model's bounds
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() ||
    index.column() < 0 || index.column() >= columnCount())
        {
        qDebug() << "HolesTransposedModel::data(): Invalid index requested.";
        return QVariant(); // Return an invalid QVariant for invalid indices
    }

    // We only handle DisplayRole and EditRole for now
    if (role == Qt::DisplayRole || role == Qt::EditRole) // Uncomment EditRole if implementing editing
    {
        // Map the column index (0-17) to the hole number (1-18)
        int holeNum = index.column() + 1;

        // Find the data for this hole number in our internal vector
        const HoleData* hole = getHoleByNumber(holeNum); // Use helper

        if (hole) {
            // Map the row index (0-1) to the attribute (Par or Handicap)
            if (index.row() == 0) {
                return hole->par; // Row 0 is for Par
            } else if (index.row() == 1) {
                return hole->handicap; // Row 1 is for Handicap
            }
            // Add more rows/attributes here if needed
                qDebug() << "HolesTransposedModel::data(): Requested row" << index.row() << "does not map to a known attribute.";

        } else {
            qDebug() << "HolesTransposedModel::data(): ERROR: Could not find data for Hole #" << holeNum << " in internal storage.";
        }
    }

    return QVariant(); // Return an invalid QVariant for roles/indices we don't handle
}

// Returns the header data for rows and columns
QVariant HolesTransposedModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // We only handle DisplayRole for headers
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) // Column headers (Hole 1, Hole 2, ...)
        {
            // Section is the column index (0-17)
            return QString("Hole %1").arg(section + 1);
        }
        else // Vertical headers (Par, Handicap)
        {
            // Section is the row index (0-1)
            if (section == 0) {
                return "Par";
            } else if (section == 1) {
                return "Handicap";
            }
            // Add more row headers here if needed
        }
    }

    return QVariant(); // Return an invalid QVariant for roles/orientations we don't handle
}

// Helper to find hole data by hole number (more efficient if m_holeData is sorted or a map)
// Assuming m_holeData is populated with hole numbers 1-18 in order
const HoleData* HolesTransposedModel::getHoleByNumber(int holeNum) const
{
    // Basic check: holeNum should be 1-18
    if (holeNum >= 1 && holeNum <= 18) {
        // In our current setup where m_holeData is resized to 18 and initialized with hole numbers 1-18
        // at index 0-17, the index is simply holeNum - 1.
        auto holeData = &m_holeData[holeNum-1];
        int holePar = holeData->par;
        return &m_holeData[holeNum - 1];
    }
    return nullptr; // Hole not found
}


// === Public method to load data for a specific course ===
void HolesTransposedModel::setCourseId(int courseId)
{

    // --- Determine if we need to load new data or clear ---
    bool needToLoad = false;
    bool needToClear = false;

    if (courseId <= 0) {
        // Invalid ID: Always clear existing data.
        if (m_currentCourseId > 0 || !m_holeData.isEmpty()) { // Only clear if we had data before
            needToClear = true;
            qDebug() << "HolesTransposedModel: Invalid ID received, need to clear.";
        } else {
             qDebug() << "HolesTransposedModel: Invalid ID received, but already cleared/empty.";
        }
    } else { // Valid courseId (> 0)
        if (m_currentCourseId != courseId) {
            // Different valid ID: Need to load new data.
            needToLoad = true;
        }
    }

    // --- Perform clear or load ---
    if (needToClear) {
        beginResetModel(); // Notify view data is changing
        m_holeData.clear(); // Clear internal data
        m_holeData.resize(18); // Resize and re-initialize defaults (to zeros)
        for(int i = 0; i < 18; ++i) {
           m_holeData[i].holeNum = i + 1;
           m_holeData[i].par = 0;
           m_holeData[i].handicap = 0;
       }
        m_currentCourseId = courseId; // Update to the new ID (invalid)
        endResetModel();   // Notify view data has changed
        return; // Finished if just clearing
    }

    if (needToLoad) {
        m_currentCourseId = courseId; // Update the current course ID

        QSqlDatabase db = database(); // Get the database connection
        if (!db.isValid() || !db.isOpen()) {
            qDebug() << "HolesTransposedModel: ERROR: Invalid or closed database connection when loading data.";
            // If database is bad, clear data and reset model
            beginResetModel();
            m_holeData.clear();
            m_holeData.resize(18);
             for(int i = 0; i < 18; ++i) {
                m_holeData[i].holeNum = i + 1;
                m_holeData[i].par = 0;
                m_holeData[i].handicap = 0;
            }
            endResetModel();
            return;
        }


        // Notify the view that the model's data is about to be reset
        beginResetModel();

        // Clear previous hole data and prepare to fetch new data
        m_holeData.clear();
        // Initialize with defaults BEFORE fetching to ensure 18 elements always exist
        m_holeData.resize(18);
        for(int i = 0; i < 18; ++i) {
           m_holeData[i].holeNum = i + 1;
           m_holeData[i].par = 0; // Start with defaults (zeros)
           m_holeData[i].handicap = 0;
       }


        QSqlQuery query(db);
        query.prepare("SELECT hole_num, par, handicap FROM holes WHERE course_id = :cid ORDER BY hole_num;");
        query.bindValue(":cid", m_currentCourseId);

        bool querySuccess = query.exec();

        if (querySuccess) {

            // Populate m_holeData by updating the default values with fetched data
            while (query.next()) {
                int fetchedHoleNum = query.value("hole_num").toInt();
                int fetchedPar = query.value("par").toInt();
                int fetchedHandicap = query.value("handicap").toInt();

                // Update the corresponding HoleData in the vector based on hole number
                if (fetchedHoleNum >= 1 && fetchedHoleNum <= 18) {
                     m_holeData[fetchedHoleNum - 1].par = fetchedPar;
                     m_holeData[fetchedHoleNum - 1].handicap = fetchedHandicap;
                 } else {
                      qDebug() << "HolesTransposedModel: Warning: Unexpected hole number" << fetchedHoleNum << "fetched from database.";
                 }
            }
        } else {
            qDebug() << "HolesTransposedModel: ERROR executing query to fetch holes:" << query.lastError().text();
        }

        // Notify the view that the model's data has been reset
        endResetModel();
    }

    // If neither needToClear nor needToLoad was true (i.e., same valid ID), we just return.
}

// Returns flags for each item (cell)
Qt::ItemFlags HolesTransposedModel::flags(const QModelIndex &index) const
{
    // Start with the default flags
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

    // Check if the index is valid and within the data area we want to make editable
    if (index.isValid() && index.row() >= 0 && index.row() < rowCount() &&
        index.column() >= 0 && index.column() < columnCount())
    {
        // Make the cells for Par (row 0) and Handicap (row 1) editable
        if (index.row() == 0 || index.row() == 1) {
            return defaultFlags | Qt::ItemIsEditable; // Add the editable flag
        }
    }

    return defaultFlags; // Return default flags for non-editable items
}

// Sets the data for a specific cell
bool HolesTransposedModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // Check if the index is valid and the role is EditRole
    if (!index.isValid() || role != Qt::EditRole || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= columnCount())
    {
        return false; // Data setting failed
    }

    // Get the value as an integer (assuming Par and Handicap are integers)
    bool ok;
    int intValue = value.toInt(&ok);

    // Check if the conversion to int was successful
    if (!ok) {
        qDebug() << "HolesTransposedModel::setData: Value is not a valid integer.";
        return false; // Data setting failed
    }

    // Map the column index (0-17) to the hole number (1-18)
    int holeNum = index.column() + 1;

    // Find the corresponding HoleData in our internal vector
    // Use a non-const pointer here as we intend to modify the data
    HoleData* hole = nullptr;
    if (holeNum >= 1 && holeNum <= 18) {
        // Assuming m_holeData[index] corresponds to hole number index + 1
        hole = &m_holeData[holeNum - 1];
    }


    if (hole) {
        bool dataUpdated = false;
        // Update the data based on the row index
        if (index.row() == 0) { // Row 0 is for Par
            if (hole->par != intValue) { // Only update if the value actually changed
                 hole->par = intValue;
                 dataUpdated = true;
            }
        } else if (index.row() == 1) { // Row 1 is for Handicap
             if (hole->handicap != intValue) { // Only update if the value actually changed
                hole->handicap = intValue;
                dataUpdated = true;
            }
        }
        // Add more rows/attributes here

        if (dataUpdated) {
            // Emit dataChanged signal to notify the view that the data at this index has changed
            emit dataChanged(index, index, {role}); // Notify view of the change

            QSqlDatabase db = database();
            if (db.isValid() && db.isOpen()) {
                QSqlQuery query(db);
                query.prepare("UPDATE holes SET par = :par, handicap = :hc WHERE course_id = :cid AND hole_num = :hnum;");
                query.bindValue(":par", hole->par); // Use the updated values from the struct
                query.bindValue(":hc", hole->handicap);
                query.bindValue(":cid", m_currentCourseId);
                query.bindValue(":hnum", hole->holeNum);

                if (!query.exec()) {
                     qDebug() << "HolesTransposedModel::setData: ERROR updating database:" << query.lastError().text();
                     // Handle database update failure - maybe revert the change in the model?
                     // Reverting the change in the model can be tricky here.
                     return false; // Indicate database update failed
                }
            } else {
                 qDebug() << "HolesTransposedModel::setData: ERROR: Database connection invalid or closed for update.";
                 // Database connection issue - report failure
                 return false;
            }

            return true; // Indicate data setting in the model was successful
        }
    }


    return false; // Data setting failed
}
