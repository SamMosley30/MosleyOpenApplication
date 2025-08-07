/**
 * @file CoursesDialog.h
 * @brief Contains the declaration of the CoursesDialog class.
 */

#ifndef COURSESDIALOG_H
#define COURSESDIALOG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QTableView>
#include <QPushButton>
#include <QItemSelectionModel>

#include "HolesTransposedModel.h"

class QTableView;
class QPushButton;

/**
 * @class CoursesDialog
 * @brief A dialog for managing golf courses and their hole data.
 *
 * This dialog allows users to add and remove courses, view and edit hole information
 * (par, handicap) for each course, and export the data.
 */
class CoursesDialog : public QDialog {
    Q_OBJECT
public:
    /**
     * @brief Constructs a CoursesDialog object.
     * @param db The database connection to use.
     * @param parent The parent widget.
     */
    explicit CoursesDialog(QSqlDatabase &db, QWidget *parent = nullptr);

private slots:
    /**
     * @brief Adds a new course to the database.
     */
    void addCourse();

    /**
     * @brief Removes the selected course from the database.
     */
    void removeSelected();

    /**
     * @brief Handles the selection change in the course view.
     *
     * When a course is selected, this slot updates the holes view to display
     * the data for the selected course.
     *
     * @param current The index of the currently selected item.
     */
    void onCourseSelectionChanged(const QModelIndex &current);

    /**
     * @brief Exports the course and hole data to a CSV file.
     */
    void exportData();

private:
    QSqlTableModel *courseModel;            ///< The model for the course data.
    HolesTransposedModel *holesTransposedModel; ///< The model for the transposed hole data.
    QTableView *courseView;                 ///< The table view for displaying courses.
    QTableView *holesView;                  ///< The table view for displaying hole data.
    QPushButton *addButton;                 ///< The button for adding a new course.
    QPushButton *removeButton;              ///< The button for removing a selected course.
    QPushButton *closeButton;               ///< The button for closing the dialog.
    QPushButton *exportButton;              ///< The button for exporting data.
    QSqlDatabase &database;                 ///< A reference to the database connection.
};

#endif // COURSESDIALOG_H