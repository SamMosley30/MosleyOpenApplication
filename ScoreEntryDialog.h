#ifndef SCOREENTRYDIALOG_H
#define SCOREENTRYDIALOG_H

#include <QDialog>
#include <QSqlDatabase> // To receive the database connection name
#include <QTabWidget>
#include <QComboBox>
#include <QTableView>
#include <QVBoxLayout> // For layout management
#include <QHBoxLayout> // For layout management
#include <QLabel>      // For labels next to combo boxes

#include "ScoreTableModel.h" // Include the ScoreTableModel header

class ScoreEntryDialog : public QDialog
{
    Q_OBJECT // Required for signals and slots

public:
    // Constructor takes the database connection name
    explicit ScoreEntryDialog(const QString &connectionName, QWidget *parent = nullptr);
    ~ScoreEntryDialog();

private slots:
    // Slots to handle course selection changes for each day
    void onDay1CourseSelected(int index);
    void onDay2CourseSelected(int index);
    void onDay3CourseSelected(int index);
    void clearData();

private:
    QString m_connectionName; // Stores the database connection name

    QTabWidget *tabWidget; // The main tab widget for days

    // Widgets for Day 1 Tab
    QWidget *day1Tab;
    QComboBox *day1CourseComboBox;
    QTableView *day1TableView;
    ScoreTableModel *day1ScoreModel; // Model for Day 1 scores

    // Widgets for Day 2 Tab
    QWidget *day2Tab;
    QComboBox *day2CourseComboBox;
    QTableView *day2TableView;
    ScoreTableModel *day2ScoreModel; // Model for Day 2 scores

    // Widgets for Day 3 Tab
    QWidget *day3Tab;
    QComboBox *day3CourseComboBox;
    QTableView *day3TableView;
    ScoreTableModel *day3ScoreModel; // Model for Day 3 scores

    // Helper method to populate course combo boxes
    void populateCourseComboBoxes();

    // Helper method to get database connection
    QSqlDatabase database() const;
    
    void loadSavedCourseSelections();
    void saveCourseSelection(int dayNum, int courseId);
    int getSavedCourseSelection(int dayNum);
};

#endif // SCOREENTRYDIALOG_H
