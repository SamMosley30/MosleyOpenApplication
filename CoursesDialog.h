#ifndef COURSESDIALOG_H
#define COURSESDIALOG_H

#include <QDialog>
#include <QSqlDatabase> // Need this if you store the database connection name
#include <QSqlTableModel> // Still need this for the course model
#include <QTableView>
#include <QPushButton>
#include <QItemSelectionModel> // For selection signals

#include "HolesTransposedModel.h" // Include the new model header

class QTableView;
class QPushButton;

class CoursesDialog : public QDialog {
    Q_OBJECT
public:
    explicit CoursesDialog(QSqlDatabase &db, QWidget *parent = nullptr);

private slots:
    void addCourse();
    void removeSelected();
    void onCourseSelectionChanged(const QModelIndex &current);
    void exportData();

private:
    QSqlTableModel *courseModel;
    HolesTransposedModel *holesTransposedModel; // Your custom model for transposed hole data
    QTableView *courseView;
    QTableView *holesView;
    QPushButton *addButton;
    QPushButton *removeButton;
    QPushButton *closeButton;
    QPushButton *exportButton;
    QSqlDatabase &database;
};

#endif // COURSESDIALOG_H