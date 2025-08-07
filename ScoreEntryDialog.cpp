/**
 * @file ScoreEntryDialog.cpp
 * @brief Implements the ScoreEntryDialog class.
 */

#include "ScoreEntryDialog.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <QHeaderView>

ScoreEntryDialog::ScoreEntryDialog(const QString &connectionName, QWidget *parent)
    : QDialog(parent)
    , m_connectionName(connectionName)
    , tabWidget(new QTabWidget(this))
    , day1Tab(new QWidget(this))
    , day1CourseComboBox(new QComboBox(this))
    , day1TableView(new QTableView(this))
    , day1ScoreModel(new ScoreTableModel(m_connectionName, 1, this))
    , day2Tab(new QWidget(this))
    , day2CourseComboBox(new QComboBox(this))
    , day2TableView(new QTableView(this))
    , day2ScoreModel(new ScoreTableModel(m_connectionName, 2, this))
    , day3Tab(new QWidget(this))
    , day3CourseComboBox(new QComboBox(this))
    , day3TableView(new QTableView(this))
    , day3ScoreModel(new ScoreTableModel(m_connectionName, 3, this))
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
         qDebug() << "ScoreEntryDialog: ERROR: Invalid or closed database connection passed to constructor.";
    }

    tabWidget->addTab(day1Tab, tr("Day 1"));
    tabWidget->addTab(day2Tab, tr("Day 2"));
    tabWidget->addTab(day3Tab, tr("Day 3"));

    QVBoxLayout *day1Layout = new QVBoxLayout(day1Tab);
    QHBoxLayout *day1CourseLayout = new QHBoxLayout();
    day1CourseLayout->addWidget(new QLabel(tr("Course:"), day1Tab));
    day1CourseLayout->addWidget(day1CourseComboBox);
    day1CourseLayout->addStretch();
    day1Layout->addLayout(day1CourseLayout);
    day1Layout->addWidget(day1TableView);

    QVBoxLayout *day2Layout = new QVBoxLayout(day2Tab);
    QHBoxLayout *day2CourseLayout = new QHBoxLayout();
    day2CourseLayout->addWidget(new QLabel(tr("Course:"), day2Tab));
    day2CourseLayout->addWidget(day2CourseComboBox);
    day2CourseLayout->addStretch();
    day2Layout->addLayout(day2CourseLayout);
    day2Layout->addWidget(day2TableView);

    QVBoxLayout *day3Layout = new QVBoxLayout(day3Tab);
    QHBoxLayout *day3CourseLayout = new QHBoxLayout();
    day3CourseLayout->addWidget(new QLabel(tr("Course:"), day3Tab));
    day3CourseLayout->addWidget(day3CourseComboBox);
    day3CourseLayout->addStretch();
    day3Layout->addLayout(day3CourseLayout);
    day3Layout->addWidget(day3TableView);

    day1TableView->setModel(day1ScoreModel);
    day2TableView->setModel(day2ScoreModel);
    day3TableView->setModel(day3ScoreModel);

    day1TableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for(int i = 1; i <= 18; ++i) {
        day1TableView->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
    day1TableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

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

    populateCourseComboBoxes();
    loadSavedCourseSelections();

    connect(day1CourseComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScoreEntryDialog::onDay1CourseSelected);
    connect(day2CourseComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScoreEntryDialog::onDay2CourseSelected);
    connect(day3CourseComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScoreEntryDialog::onDay3CourseSelected);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    
    QPushButton *resetButton = new QPushButton(tr("Reset Data"), this);
    QPushButton *closeButton = new QPushButton(tr("Close"), this);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(resetButton, &QPushButton::clicked, this, &ScoreEntryDialog::clearData);

    setLayout(mainLayout);
    setWindowTitle(tr("Tournament Score Entry"));
    resize(800, 600);
}

ScoreEntryDialog::~ScoreEntryDialog()
{
}

void ScoreEntryDialog::refresh()
{
    populateCourseComboBoxes();
    loadSavedCourseSelections();
}

QSqlDatabase ScoreEntryDialog::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

/**
 * @brief Populates the course combo boxes with data from the database.
 */
void ScoreEntryDialog::populateCourseComboBoxes()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "ScoreEntryDialog::populateCourseComboBoxes: ERROR: Invalid or closed database connection.";
        return;
    }

    day1CourseComboBox->clear();
    day2CourseComboBox->clear();
    day3CourseComboBox->clear();

    day1CourseComboBox->addItem(tr("-- Select Course --"), -1);
    day2CourseComboBox->addItem(tr("-- Select Course --"), -1);
    day3CourseComboBox->addItem(tr("-- Select Course --"), -1);

    QSqlQuery query(db);
    if (query.exec("SELECT id, name FROM courses ORDER BY name")) {
        while (query.next()) {
            int courseId = query.value("id").toInt();
            QString courseName = query.value("name").toString();
            day1CourseComboBox->addItem(courseName, courseId);
            day2CourseComboBox->addItem(courseName, courseId);
            day3CourseComboBox->addItem(courseName, courseId);
        }
    } else {
        qDebug() << "ScoreEntryDialog::populateCourseComboBoxes: ERROR executing query:" << query.lastError().text();
    }
}

/**
 * @brief Loads saved course selections from the settings table and sets the combo boxes.
 */
void ScoreEntryDialog::loadSavedCourseSelections()
{
    day1CourseComboBox->blockSignals(true);
    day2CourseComboBox->blockSignals(true);
    day3CourseComboBox->blockSignals(true);

    int day1CourseId = getSavedCourseSelection(1);
    int day1Index = day1CourseComboBox->findData(day1CourseId);
    if (day1Index != -1) {
        day1CourseComboBox->setCurrentIndex(day1Index);
        onDay1CourseSelected(day1Index);
    } else {
        day1CourseComboBox->setCurrentIndex(day1CourseComboBox->findData(-1));
        onDay1CourseSelected(day1CourseComboBox->findData(-1));
    }

    int day2CourseId = getSavedCourseSelection(2);
    int day2Index = day2CourseComboBox->findData(day2CourseId);
    if (day2Index != -1) {
        day2CourseComboBox->setCurrentIndex(day2Index);
        onDay2CourseSelected(day2Index);
    } else {
        day2CourseComboBox->setCurrentIndex(day2CourseComboBox->findData(-1));
         onDay2CourseSelected(day2CourseComboBox->findData(-1));
    }

    int day3CourseId = getSavedCourseSelection(3);
    int day3Index = day3CourseComboBox->findData(day3CourseId);
    if (day3Index != -1) {
        day3CourseComboBox->setCurrentIndex(day3Index);
        onDay3CourseSelected(day3Index);
    } else {
        day3CourseComboBox->setCurrentIndex(day3CourseComboBox->findData(-1));
         onDay3CourseSelected(day3CourseComboBox->findData(-1));
    }

    day1CourseComboBox->blockSignals(false);
    day2CourseComboBox->blockSignals(false);
    day3CourseComboBox->blockSignals(false);
}

/**
 * @brief Saves the selected course ID for a given day to the settings table.
 * @param dayNum The day number (1, 2, or 3).
 * @param courseId The ID of the selected course.
 */
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
    query.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (:key, :value);");
    query.bindValue(":key", key);
    query.bindValue(":value", value);

    if (!query.exec()) {
        qDebug() << "ScoreEntryDialog::saveCourseSelection: ERROR saving setting" << key << ":" << query.lastError().text();
    }
}

/**
 * @brief Retrieves the saved course ID for a given day from the settings table.
 * @param dayNum The day number (1, 2, or 3).
 * @return The saved course ID, or -1 if not found or on error.
 */
int ScoreEntryDialog::getSavedCourseSelection(int dayNum)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "ScoreEntryDialog::getSavedCourseSelection: ERROR: Invalid or closed database connection.";
        return -1;
    }

    QString key = QString("day%1_course_id").arg(dayNum);
    QSqlQuery query(db);
    query.prepare("SELECT value FROM settings WHERE key = :key;");
    query.bindValue(":key", key);

    if (query.exec() && query.next()) {
        bool ok;
        int savedValue = query.value(0).toInt(&ok);
        if (ok) {
            return savedValue;
        } else {
            qDebug() << "ScoreEntryDialog::getSavedCourseSelection: Conversion failed for setting" << key << ". Value was:" << query.value(0).toString();
            return -1;
        }
    } else {
        qDebug() << "ScoreEntryDialog::getSavedCourseSelection: Setting" << key << "not found or query failed.";
        return -1;
    }
}

/**
 * @brief Slot for when the course for Day 1 is selected.
 * @param index The index of the selected item in the combo box.
 */
void ScoreEntryDialog::onDay1CourseSelected(int index)
{
    int courseId = day1CourseComboBox->itemData(index).toInt();
    day1ScoreModel->setCourseId(courseId);
    saveCourseSelection(1, courseId);
}

/**
 * @brief Slot for when the course for Day 2 is selected.
 * @param index The index of the selected item in the combo box.
 */
void ScoreEntryDialog::onDay2CourseSelected(int index)
{
    int courseId = day2CourseComboBox->itemData(index).toInt();
    day2ScoreModel->setCourseId(courseId);
    saveCourseSelection(2, courseId);
}

/**
 * @brief Slot for when the course for Day 3 is selected.
 * @param index The index of the selected item in the combo box.
 */
void ScoreEntryDialog::onDay3CourseSelected(int index)
{
    int courseId = day3CourseComboBox->itemData(index).toInt();
    day3ScoreModel->setCourseId(courseId);
    saveCourseSelection(3, courseId);
}

/**
 * @brief Clears all score data for the currently selected day and course.
 */
void ScoreEntryDialog::clearData()
{
    int currentDayIndex = tabWidget->currentIndex();
    int currentDayNum = currentDayIndex + 1;

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
            return;
    }

    if (currentComboBox) {
        currentCourseId = currentComboBox->itemData(currentComboBox->currentIndex()).toInt();
    }

    if (currentCourseId <= 0) {
        QMessageBox::information(this, tr("Reset Scores"), tr("Please select a course before resetting scores for this day."));
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Confirm Reset"),
                                  tr("Are you sure you want to reset all scores for Day %1 on this course?\nThis action cannot be undone.").arg(currentDayNum),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QSqlDatabase db = database();
        if (!db.isValid() || !db.isOpen()) {
            qDebug() << "ScoreEntryDialog::resetScores: ERROR: Invalid or closed database connection.";
            QMessageBox::critical(this, tr("Database Error"), tr("Database connection is not available to reset scores."));
            return;
        }

        QSqlQuery query(db);
        query.prepare("DELETE FROM scores WHERE day_num = :dnum AND course_id = :cid;");
        query.bindValue(":dnum", currentDayNum);
        query.bindValue(":cid", currentCourseId);

        if (query.exec()) {
            if (currentModel) {
                currentModel->setCourseId(currentCourseId);
            }
            QMessageBox::information(this, tr("Reset Successful"), tr("Scores for Day %1 on this course have been reset.").arg(currentDayNum));
        } else {
            qDebug() << "ScoreEntryDialog::resetScores: ERROR deleting scores:" << query.lastError().text();
            QMessageBox::critical(this, tr("Database Error"), tr("Failed to reset scores:\n%1").arg(query.lastError().text()));
        }
    } else {
        qDebug() << "ScoreEntryDialog::resetScores: Reset cancelled by user.";
    }
}
