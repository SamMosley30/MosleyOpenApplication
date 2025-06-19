#include "ScoreEntryDialog.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <QHeaderView> // For table view header settings

// Constructor
ScoreEntryDialog::ScoreEntryDialog(const QString &connectionName, QWidget *parent)
    : QDialog(parent)
    , m_connectionName(connectionName)
    , tabWidget(new QTabWidget(this))

    // Day 1 Widgets
    , day1Tab(new QWidget(this))
    , day1CourseComboBox(new QComboBox(this))
    , day1TableView(new QTableView(this))
    , day1ScoreModel(new ScoreTableModel(m_connectionName, 1, this)) // Create model for Day 1

    // Day 2 Widgets
    , day2Tab(new QWidget(this))
    , day2CourseComboBox(new QComboBox(this))
    , day2TableView(new QTableView(this))
    , day2ScoreModel(new ScoreTableModel(m_connectionName, 2, this)) // Create model for Day 2

    // Day 3 Widgets
    , day3Tab(new QWidget(this))
    , day3CourseComboBox(new QComboBox(this))
    , day3TableView(new QTableView(this))
    , day3ScoreModel(new ScoreTableModel(m_connectionName, 3, this)) // Create model for Day 3
{
    // --- Setup Database Connection ---
    // Ensure the database connection is valid and open
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
         qDebug() << "ScoreEntryDialog: ERROR: Invalid or closed database connection passed to constructor.";
         // Handle this error appropriately - maybe disable UI elements or show a message
         // For now, we'll just log the error.
    }

    // --- Setup Tab Widget ---
    tabWidget->addTab(day1Tab, tr("Day 1"));
    tabWidget->addTab(day2Tab, tr("Day 2"));
    tabWidget->addTab(day3Tab, tr("Day 3"));

    // --- Setup Layout for Day 1 Tab ---
    QVBoxLayout *day1Layout = new QVBoxLayout(day1Tab); // Set parent for layout
    QHBoxLayout *day1CourseLayout = new QHBoxLayout(); // Layout for course selector
    day1CourseLayout->addWidget(new QLabel(tr("Course:"), day1Tab)); // Label for combo box
    day1CourseLayout->addWidget(day1CourseComboBox);
    day1CourseLayout->addStretch(); // Push widgets to the left
    day1Layout->addLayout(day1CourseLayout);
    day1Layout->addWidget(day1TableView); // Add table view below course selector

    // --- Setup Layout for Day 2 Tab ---
    QVBoxLayout *day2Layout = new QVBoxLayout(day2Tab);
    QHBoxLayout *day2CourseLayout = new QHBoxLayout();
    day2CourseLayout->addWidget(new QLabel(tr("Course:"), day2Tab));
    day2CourseLayout->addWidget(day2CourseComboBox);
    day2CourseLayout->addStretch();
    day2Layout->addLayout(day2CourseLayout);
    day2Layout->addWidget(day2TableView);

    // --- Setup Layout for Day 3 Tab ---
    QVBoxLayout *day3Layout = new QVBoxLayout(day3Tab);
    QHBoxLayout *day3CourseLayout = new QHBoxLayout();
    day3CourseLayout->addWidget(new QLabel(tr("Course:"), day3Tab));
    day3CourseLayout->addWidget(day3CourseComboBox);
    day3CourseLayout->addStretch();
    day3Layout->addLayout(day3CourseLayout);
    day3Layout->addWidget(day3TableView);

    // --- Setup Table Views ---
    // Set the custom score models for each table view
    day1TableView->setModel(day1ScoreModel);
    day2TableView->setModel(day2ScoreModel);
    day3TableView->setModel(day3ScoreModel);

    // Optional: Configure table view appearance (similar to your other table views)
    // Hide player ID column if ScoreTableModel has one internally but not for display
    // day1TableView->hideColumn(0); // Assuming Player Name is column 0
    // Set resize modes for headers
    day1TableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch); // Stretch Player Name column
    for(int i = 1; i <= 18; ++i) { // Set fixed or resize-to-content for score columns
        day1TableView->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents); // Or QHeaderView::Fixed and resizeSection()
    }
    day1TableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents); // Resize row headers (if any)

    // Apply similar settings to day2TableView and day3TableView
    day2TableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for(int i = 1; i <= 18; ++i) {
        day2TableView->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
    day2TableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    day3TableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for(int i = 1; i <= 18; ++i) {
        day3TableView->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
    day3TableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);


    // --- Populate Course Combo Boxes ---
    populateCourseComboBoxes();

    // --- Load Saved Course Selections and Initialize Models ---
    loadSavedCourseSelections();

    // --- Connect Signals and Slots ---
    // Connect course combo box selection changes to update the score models
    connect(day1CourseComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScoreEntryDialog::onDay1CourseSelected);
    connect(day2CourseComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScoreEntryDialog::onDay2CourseSelected);
    connect(day3CourseComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScoreEntryDialog::onDay3CourseSelected);

    // --- Main Layout for the Dialog ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    
    QPushButton *resetButton = new QPushButton(tr("Reset Data"), this);
    QPushButton *closeButton = new QPushButton(tr("Close"), this);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept); // Or &QDialog::close
    connect(resetButton, &QPushButton::clicked, this, &ScoreEntryDialog::clearData); // Or &QDialog::close

    setLayout(mainLayout); // Set the main layout for the dialog
    setWindowTitle(tr("Tournament Score Entry")); // Set the dialog title
    resize(800, 600); // Set a default size
}

// Destructor
ScoreEntryDialog::~ScoreEntryDialog()
{
    // Models and widgets are parented to 'this' or other widgets,
    // so they will be deleted automatically when the dialog is deleted.
}

// Helper to get database connection
QSqlDatabase ScoreEntryDialog::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

// Helper method to populate course combo boxes
void ScoreEntryDialog::populateCourseComboBoxes()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "ScoreEntryDialog::populateCourseComboBoxes: ERROR: Invalid or closed database connection.";
        return;
    }

    // Clear existing items (except potentially a placeholder)
    day1CourseComboBox->clear();
    day2CourseComboBox->clear();
    day3CourseComboBox->clear();

    // Add a default item indicating no course selected
    day1CourseComboBox->addItem(tr("-- Select Course --"), -1); // Value -1 indicates no valid course ID
    day2CourseComboBox->addItem(tr("-- Select Course --"), -1);
    day3CourseComboBox->addItem(tr("-- Select Course --"), -1);


    QSqlQuery query(db);
    // Select course ID and name, ordered by name
    if (query.exec("SELECT id, name FROM courses ORDER BY name")) {
        while (query.next()) {
            int courseId = query.value("id").toInt();
            QString courseName = query.value("name").toString();
            // Add course name to combo box, storing the ID as UserData
            day1CourseComboBox->addItem(courseName, courseId);
            day2CourseComboBox->addItem(courseName, courseId);
            day3CourseComboBox->addItem(courseName, courseId);
        }
    } else {
        qDebug() << "ScoreEntryDialog::populateCourseComboBoxes: ERROR executing query:" << query.lastError().text();
    }
}

// Loads saved course selections from the settings table and sets combo boxes
void ScoreEntryDialog::loadSavedCourseSelections()
{
    // Temporarily block signals to prevent onDayXCourseSelected slots from firing
    // when we programmatically set the combo box index.
    day1CourseComboBox->blockSignals(true);
    day2CourseComboBox->blockSignals(true);
    day3CourseComboBox->blockSignals(true);

    // Load and set selection for Day 1
    int day1CourseId = getSavedCourseSelection(1);
    int day1Index = day1CourseComboBox->findData(day1CourseId);
    if (day1Index != -1) {
        day1CourseComboBox->setCurrentIndex(day1Index);
        // Manually trigger the slot to load data for the model
        onDay1CourseSelected(day1Index);
    } else {
        // If saved ID is not found (e.g., course deleted), set to default item (-1)
        day1CourseComboBox->setCurrentIndex(day1CourseComboBox->findData(-1));
        onDay1CourseSelected(day1CourseComboBox->findData(-1)); // Trigger with default
    }

    // Load and set selection for Day 2
    int day2CourseId = getSavedCourseSelection(2);
    int day2Index = day2CourseComboBox->findData(day2CourseId);
    if (day2Index != -1) {
        day2CourseComboBox->setCurrentIndex(day2Index);
        onDay2CourseSelected(day2Index);
    } else {
        day2CourseComboBox->setCurrentIndex(day2CourseComboBox->findData(-1));
         onDay2CourseSelected(day2CourseComboBox->findData(-1));
    }

    // Load and set selection for Day 3
    int day3CourseId = getSavedCourseSelection(3);
    int day3Index = day3CourseComboBox->findData(day3CourseId);
    if (day3Index != -1) {
        day3CourseComboBox->setCurrentIndex(day3Index);
        onDay3CourseSelected(day3Index);
    } else {
        day3CourseComboBox->setCurrentIndex(day3CourseComboBox->findData(-1));
         onDay3CourseSelected(day3CourseComboBox->findData(-1));
    }

    // Unblock signals
    day1CourseComboBox->blockSignals(false);
    day2CourseComboBox->blockSignals(false);
    day3CourseComboBox->blockSignals(false);
}

// Saves the selected course ID for a given day to the settings table
void ScoreEntryDialog::saveCourseSelection(int dayNum, int courseId)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "ScoreEntryDialog::saveCourseSelection: ERROR: Invalid or closed database connection.";
        return;
    }

    QString key = QString("day%1_course_id").arg(dayNum);
    QString value = QString::number(courseId);

    QSqlQuery query(db);
    // Use INSERT OR REPLACE to add or update the setting
    query.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (:key, :value);");
    query.bindValue(":key", key);
    query.bindValue(":value", value);

    if (!query.exec())
        qDebug() << "ScoreEntryDialog::saveCourseSelection: ERROR saving setting" << key << ":" << query.lastError().text();
}

// Retrieves the saved course ID for a given day from the settings table
int ScoreEntryDialog::getSavedCourseSelection(int dayNum)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "ScoreEntryDialog::getSavedCourseSelection: ERROR: Invalid or closed database connection.";
        return -1; // Return invalid ID on error
    }

    QString key = QString("day%1_course_id").arg(dayNum);
    QSqlQuery query(db);
    query.prepare("SELECT value FROM settings WHERE key = :key;");
    query.bindValue(":key", key);

    if (query.exec() && query.next()) {
        // Return the saved value as an integer, default to -1 if conversion fails
        bool ok;
        int savedValue = query.value(0).toInt(&ok); // Get the value and check success
        if (ok) {
            return savedValue; // Return the successfully converted value
        } else {
            qDebug() << "ScoreEntryDialog::getSavedCourseSelection: Conversion failed for setting" << key << ". Value was:" << query.value(0).toString();
            return -1; // Return default invalid ID if conversion failed
        }
    } else {
        // Setting not found or query failed, return default invalid ID
        qDebug() << "ScoreEntryDialog::getSavedCourseSelection: Setting" << key << "not found or query failed.";
        return -1;
    }
}

// Slots to handle course selection changes for each day
void ScoreEntryDialog::onDay1CourseSelected(int index)
{
    // Get the selected course ID from the combo box's UserData
    int courseId = day1CourseComboBox->itemData(index).toInt();

    // Tell the Day 1 score model to load data for this course
    day1ScoreModel->setCourseId(courseId);

    // --- Save the selected course ID ---
    saveCourseSelection(1, courseId);
}

void ScoreEntryDialog::onDay2CourseSelected(int index)
{
    int courseId = day2CourseComboBox->itemData(index).toInt();
    day2ScoreModel->setCourseId(courseId);

    // --- Save the selected course ID ---
    saveCourseSelection(2, courseId);
}

void ScoreEntryDialog::onDay3CourseSelected(int index)
{
    int courseId = day3CourseComboBox->itemData(index).toInt();
    day3ScoreModel->setCourseId(courseId);

    // --- Save the selected course ID ---
    saveCourseSelection(3, courseId);
}

// New slot to handle resetting scores for the current day/course
void ScoreEntryDialog::clearData()
{
    // Determine which day is currently active
    int currentDayIndex = tabWidget->currentIndex();
    int currentDayNum = currentDayIndex + 1; // Day numbers are 1, 2, 3

    // Get the currently selected course ID for this day
    int currentCourseId = -1;
    ScoreTableModel *currentModel = nullptr;
    QComboBox *currentComboBox = nullptr;

    switch (currentDayNum) {
        case 1:
            currentComboBox = day1CourseComboBox;
            currentModel = day1ScoreModel;
            break;
        case 2:
            currentComboBox = day2CourseComboBox;
            currentModel = day2ScoreModel;
            break;
        case 3:
            currentComboBox = day3CourseComboBox;
            currentModel = day3ScoreModel;
            break;
        default:
            qDebug() << "ScoreEntryDialog::resetScores: ERROR: Invalid current day index:" << currentDayIndex;
            return; // Should not happen with 3 tabs
    }

    if (currentComboBox) {
        currentCourseId = currentComboBox->itemData(currentComboBox->currentIndex()).toInt();
    }

    // Check if a valid course is selected
    if (currentCourseId <= 0) {
        QMessageBox::information(this, tr("Reset Scores"), tr("Please select a course before resetting scores for this day."));
        return;
    }

    // Ask for confirmation before deleting data
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Confirm Reset"),
                                  tr("Are you sure you want to reset all scores for Day %1 on this course?\nThis action cannot be undone.").arg(currentDayNum),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // User confirmed, proceed with deletion
        QSqlDatabase db = database();
        if (!db.isValid() || !db.isOpen()) {
            qDebug() << "ScoreEntryDialog::resetScores: ERROR: Invalid or closed database connection.";
            QMessageBox::critical(this, tr("Database Error"), tr("Database connection is not available to reset scores."));
            return;
        }

        QSqlQuery query(db);
        // Delete scores for the specific day and course
        query.prepare("DELETE FROM scores WHERE day_num = :dnum AND course_id = :cid;");
        query.bindValue(":dnum", currentDayNum);
        query.bindValue(":cid", currentCourseId);

        if (query.exec()) {
            
            // Clear the data in the corresponding model and notify the view
            if (currentModel) {
                currentModel->setCourseId(currentCourseId); // Calling setCourseId with the same ID will clear and reload (which will now be empty)
                // Alternatively, you could add a specific clearData() method to ScoreTableModel
                // currentModel->clearData(); // If you add this method
            }

            QMessageBox::information(this, tr("Reset Successful"), tr("Scores for Day %1 on this course have been reset.").arg(currentDayNum));

        } else {
            qDebug() << "ScoreEntryDialog::resetScores: ERROR deleting scores:" << query.lastError().text();
            QMessageBox::critical(this, tr("Database Error"), tr("Failed to reset scores:\n%1").arg(query.lastError().text()));
        }
    } else {
        // User cancelled
        qDebug() << "ScoreEntryDialog::resetScores: Reset cancelled by user.";
    }
}
