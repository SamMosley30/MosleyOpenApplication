#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QSqlDatabase;
class PlayerDialog;
class CourseDialog;
class ScoreEntryDialog;
class TournamentLeaderboardDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QSqlDatabase &db, QWidget *parent = nullptr);

private slots:
    void openPlayerDialog();
    void openCoursesDialog();
    void openScoreDialog();
    void openLeaderboardDialog();

private:
    class PlayerDialog *playerDialog;
    class CoursesDialog *coursesDialog;
    class ScoreEntryDialog *scoreDialog;
    class TournamentLeaderboardDialog *tournamentLeaderboardDialog;
    QSqlDatabase &database;
};

#endif // MAINWINDOW_H
