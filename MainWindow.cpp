/**
 * @file MainWindow.cpp
 * @brief Implements the MainWindow class.
 */

#include "MainWindow.h"
#include "PlayerDialog.h"
#include "CoursesDialog.h"
#include "ScoreEntryDialog.h"
#include "TournamentLeaderboardDialog.h"
#include "TeamAssemblyDialog.h"
#include "CsvExporter.h"
#include <QtWidgets>
#include <QSqlDatabase>
#include <QDebug>

/**
 * @brief Constructs a MainWindow object.
 *
 * This constructor sets up the main window UI, including buttons for accessing
 * different parts of the application. It also initializes the various dialogs.
 *
 * @param db The database connection to use.
 * @param parent The parent widget.
 */
MainWindow::MainWindow(QSqlDatabase &db, QWidget *parent)
    : QMainWindow(parent)
    , database(db)
{
    QString connNameToPass = database.connectionName();

    playerDialog = new PlayerDialog(database, new CsvExporter(this), this);
    coursesDialog = new CoursesDialog(database, this);
    scoreDialog = new ScoreEntryDialog(connNameToPass, this);
    tournamentLeaderboardDialog = new TournamentLeaderboardDialog(connNameToPass, this);
    teamAssemblyDialog = new TeamAssemblyDialog(database, this);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    auto *playersButton = new QPushButton(tr("Manage Players"), central);
    auto *coursesButton = new QPushButton(tr("Manage Courses"), central);
    auto *scoreButton = new QPushButton(tr("Manage Scores"), central);
    auto *leaderboardButton = new QPushButton(tr("Tournament Leaderboard"), central);
    auto *teamAssemblyButton = new QPushButton(tr("Assemble Teams"), central);

    layout->addWidget(playersButton);
    layout->addWidget(coursesButton);
    layout->addWidget(scoreButton);
    layout->addWidget(leaderboardButton);
    layout->addWidget(teamAssemblyButton);

    central->setLayout(layout);
    setCentralWidget(central);

    connect(playersButton, &QPushButton::clicked, this, &MainWindow::openPlayerDialog);
    connect(coursesButton, &QPushButton::clicked, this, &MainWindow::openCoursesDialog);
    connect(scoreButton, &QPushButton::clicked, this, &MainWindow::openScoreDialog);
    connect(leaderboardButton, &QPushButton::clicked, this, &MainWindow::openLeaderboardDialog);
    connect(teamAssemblyButton, &QPushButton::clicked, this, &MainWindow::openTeamAssemblyDialog);

    setWindowTitle(tr("Tournament App"));
    resize(400, 300);
}

/**
 * @brief Opens the player management dialog.
 */
void MainWindow::openPlayerDialog() {
    playerDialog->exec();
}

/**
 * @brief Opens the course management dialog.
 */
void MainWindow::openCoursesDialog() {
    coursesDialog->exec();
}

/**
 * @brief Opens the score entry dialog.
 */
void MainWindow::openScoreDialog() {
    scoreDialog->exec();
}

/**
 * @brief Opens the tournament leaderboard dialog.
 */
void MainWindow::openLeaderboardDialog() {
    tournamentLeaderboardDialog->exec();
}

/**
 * @brief Opens the team assembly dialog.
 */
void MainWindow::openTeamAssemblyDialog() {
    teamAssemblyDialog->exec();
}
