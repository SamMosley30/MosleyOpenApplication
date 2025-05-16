#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QSqlDatabase;
class PlayerDialog;
class CoursesDialog;
class ScoreEntryDialog;
class TournamentLeaderboardDialog;
class TeamAssemblyDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QSqlDatabase &db, QWidget *parent = nullptr);

private slots:
    void openPlayerDialog();
    void openCoursesDialog();
    void openScoreDialog();
    void openLeaderboardDialog();
    void openTeamAssemblyDialog();

private:
    class PlayerDialog *playerDialog;
    class CoursesDialog *coursesDialog;
    class ScoreEntryDialog *scoreDialog;
    class TournamentLeaderboardDialog *tournamentLeaderboardDialog;
    class TeamAssemblyDialog *teamAssemblyDialog;
    QSqlDatabase &database;
};

#endif // MAINWINDOW_H
