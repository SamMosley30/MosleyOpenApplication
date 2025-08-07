/**
 * @file ScoreEntryDialog.h
 * @brief Contains the declaration of the ScoreEntryDialog class.
 */

#ifndef SCOREENTRYDIALOG_H
#define SCOREENTRYDIALOG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QTabWidget>
#include <QComboBox>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "ScoreTableModel.h"

/**
 * @class ScoreEntryDialog
 * @brief A dialog for entering scores for each day of the tournament.
 *
 * This dialog provides a tabbed interface for entering scores for Day 1, Day 2,
 * and Day 3. Each tab contains a table view for score entry and a combo box
 * to select the course for that day.
 */
class ScoreEntryDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a ScoreEntryDialog object.
     * @param connectionName The name of the database connection to use.
     * @param parent The parent widget.
     */
    explicit ScoreEntryDialog(const QString &connectionName, QWidget *parent = nullptr);

    /**
     * @brief Destroys the ScoreEntryDialog object.
     */
    ~ScoreEntryDialog();

    /**
     * @brief Refreshes the data in the dialog.
     */
    void refresh();

private slots:
    void onDay1CourseSelected(int index);
    void onDay2CourseSelected(int index);
    void onDay3CourseSelected(int index);
    void clearData();

private:
    QString m_connectionName; ///< The name of the database connection.

    QTabWidget *tabWidget; ///< The main tab widget for days.

    // Day 1 Tab Widgets
    QWidget *day1Tab;
    QComboBox *day1CourseComboBox;
    QTableView *day1TableView;
    ScoreTableModel *day1ScoreModel;

    // Day 2 Tab Widgets
    QWidget *day2Tab;
    QComboBox *day2CourseComboBox;
    QTableView *day2TableView;
    ScoreTableModel *day2ScoreModel;

    // Day 3 Tab Widgets
    QWidget *day3Tab;
    QComboBox *day3CourseComboBox;
    QTableView *day3TableView;
    ScoreTableModel *day3ScoreModel;

    void populateCourseComboBoxes();
    QSqlDatabase database() const;
    void loadSavedCourseSelections();
    void saveCourseSelection(int dayNum, int courseId);
    int getSavedCourseSelection(int dayNum);
};

#endif // SCOREENTRYDIALOG_H
