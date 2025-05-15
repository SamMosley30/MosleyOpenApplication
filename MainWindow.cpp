#include "MainWindow.h"
#include "PlayerDialog.h"
#include "CoursesDialog.h"
#include "ScoreEntryDialog.h"
#include "TournamentLeaderboardDialog.h"
#include <QtWidgets>

MainWindow::MainWindow(QSqlDatabase &db, QWidget *parent)
    : QMainWindow(parent)
    , database(db)
    , playerDialog(new PlayerDialog(db, this))
    , coursesDialog(new CoursesDialog(db, this))
    , scoreDialog(new ScoreEntryDialog(db.connectionName(), this))
    , tournamentLeaderboardDialog(new TournamentLeaderboardDialog(db.connectionName(), this))
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    auto *playersButton = new QPushButton(tr("Manage Players"), central);
    auto *coursesButton = new QPushButton(tr("Manage Courses"), central);
    auto *scoreButton = new QPushButton(tr("Manage Scores"), central);
    auto *leaderboardButton = new QPushButton(tr("Tournament Leaderboard"), central);
    layout->addWidget(playersButton);
    layout->addWidget(coursesButton);
    layout->addWidget(scoreButton);
    layout->addWidget(leaderboardButton);
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

    setWindowTitle(tr("Tournament App"));
    resize(400, 200);
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
