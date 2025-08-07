/**
 * @file MainWindow.h
 * @brief Contains the declaration of the MainWindow class.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QSqlDatabase;
class PlayerDialog;
class CoursesDialog;
class ScoreEntryDialog;
class TournamentLeaderboardDialog;
class TeamAssemblyDialog;

/**
 * @class MainWindow
 * @brief The main window of the application.
 *
 * This class creates the main application window and provides access to the
 * various dialogs for managing players, courses, scores, teams, and the leaderboard.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    /**
     * @brief Constructs a MainWindow object.
     * @param db The database connection to use.
     * @param parent The parent widget.
     */
    explicit MainWindow(QSqlDatabase &db, QWidget *parent = nullptr);

private slots:
    /**
     * @brief Opens the player management dialog.
     */
    void openPlayerDialog();

    /**
     * @brief Opens the course management dialog.
     */
    void openCoursesDialog();

    /**
     * @brief Opens the score entry dialog.
     */
    void openScoreDialog();

    /**
     * @brief Opens the tournament leaderboard dialog.
     */
    void openLeaderboardDialog();

    /**
     * @brief Opens the team assembly dialog.
     */
    void openTeamAssemblyDialog();

    /**
     * @brief Archives the current database to a new file.
     */
    void archiveDatabase();

    /**
     * @brief Loads a database from an archive file.
     */
    void loadDatabaseFromArchive();

private:
    PlayerDialog *playerDialog;                     ///< The player management dialog.
    CoursesDialog *coursesDialog;                   ///< The course management dialog.
    ScoreEntryDialog *scoreDialog;                  ///< The score entry dialog.
    TournamentLeaderboardDialog *tournamentLeaderboardDialog; ///< The tournament leaderboard dialog.
    TeamAssemblyDialog *teamAssemblyDialog;         ///< The team assembly dialog.
    QSqlDatabase &database;                         ///< A reference to the database connection.
};

#endif // MAINWINDOW_H
