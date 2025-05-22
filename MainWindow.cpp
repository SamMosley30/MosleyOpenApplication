#include "MainWindow.h"
#include "PlayerDialog.h"
#include "CoursesDialog.h"
#include "ScoreEntryDialog.h"
#include "TournamentLeaderboardDialog.h"
#include "TeamAssemblyDialog.h" 
#include <QtWidgets>
#include <QSqlDatabase> // For QSqlDatabase::defaultConnection
#include <QDebug>       // For qDebug

MainWindow::MainWindow(QSqlDatabase &db, QWidget *parent) // db is a reference to the one in main
    : QMainWindow(parent)
    , database(db) // Store the reference
{
    QString connNameToPass = database.connectionName();


    playerDialog = new PlayerDialog(database, this); // Pass the QSqlDatabase reference
    coursesDialog = new CoursesDialog(database, this); // Pass the QSqlDatabase reference
    scoreDialog = new ScoreEntryDialog(connNameToPass, this); // Pass the connection name string
    tournamentLeaderboardDialog = new TournamentLeaderboardDialog(connNameToPass, this); // Pass the connection name string
    teamAssemblyDialog = new TeamAssemblyDialog(database, this); // Pass the QSqlDatabase reference


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

    connect(playersButton, &QPushButton::clicked,
            this, &MainWindow::openPlayerDialog);

    connect(coursesButton, &QPushButton::clicked,
            this, &MainWindow::openCoursesDialog);

    connect(scoreButton, &QPushButton::clicked,
            this, &MainWindow::openScoreDialog);

    connect(leaderboardButton, &QPushButton::clicked,
            this, &MainWindow::openLeaderboardDialog);

    connect(teamAssemblyButton, &QPushButton::clicked, 
            this, &MainWindow::openTeamAssemblyDialog);

    setWindowTitle(tr("Tournament App"));
    resize(400, 300); // Adjusted height for new button
}

void MainWindow::openPlayerDialog() {
    playerDialog->exec();
}

void MainWindow::openCoursesDialog() {
    coursesDialog->exec();
}

void MainWindow::openScoreDialog() {
    scoreDialog->exec();
}

void MainWindow::openLeaderboardDialog() {
    tournamentLeaderboardDialog->exec();
}

void MainWindow::openTeamAssemblyDialog() { 
    teamAssemblyDialog->exec();
}
