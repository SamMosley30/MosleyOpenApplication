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

    playerDialog = new PlayerDialog(database, this);
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
    auto *archiveButton = new QPushButton(tr("Archive Database"), central);
    auto *loadFromArchiveButton = new QPushButton(tr("Load from Archive"), central);

    layout->addWidget(playersButton);
    layout->addWidget(coursesButton);
    layout->addWidget(scoreButton);
    layout->addWidget(leaderboardButton);
    layout->addWidget(teamAssemblyButton);
    layout->addStretch();
    layout->addWidget(archiveButton);
    layout->addWidget(loadFromArchiveButton);

    central->setLayout(layout);
    setCentralWidget(central);

    connect(playersButton, &QPushButton::clicked, this, &MainWindow::openPlayerDialog);
    connect(coursesButton, &QPushButton::clicked, this, &MainWindow::openCoursesDialog);
    connect(scoreButton, &QPushButton::clicked, this, &MainWindow::openScoreDialog);
    connect(leaderboardButton, &QPushButton::clicked, this, &MainWindow::openLeaderboardDialog);
    connect(teamAssemblyButton, &QPushButton::clicked, this, &MainWindow::openTeamAssemblyDialog);
    connect(archiveButton, &QPushButton::clicked, this, &MainWindow::archiveDatabase);
    connect(loadFromArchiveButton, &QPushButton::clicked, this, &MainWindow::loadDatabaseFromArchive);

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

/**
 * @brief Archives the current database to a new file.
 */
void MainWindow::archiveDatabase()
{
    QString currentDbPath = database.databaseName();
    if (currentDbPath.isEmpty()) {
        QMessageBox::critical(this, tr("Archive Error"), tr("Could not determine the path of the current database."));
        return;
    }

    QString archivePath = QFileDialog::getSaveFileName(
        this,
        tr("Archive Database As..."),
        QDir::homePath() + "/tournament_archive.db",
        tr("Database files (*.db);;All files (*.*)"));

    if (archivePath.isEmpty()) {
        return; // User cancelled
    }

    if (QFile::exists(archivePath)) {
        if (QMessageBox::question(this, tr("Confirm Overwrite"), tr("The file %1 already exists. Do you want to overwrite it?").arg(archivePath), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        QFile::remove(archivePath);
    }

    if (QFile::copy(currentDbPath, archivePath)) {
        QMessageBox::information(this, tr("Archive Successful"), tr("Database successfully archived to %1.").arg(archivePath));
    } else {
        QMessageBox::critical(this, tr("Archive Failed"), tr("Could not copy the database file to the specified location."));
    }
}

/**
 * @brief Loads a database from an archive file.
 */
void MainWindow::loadDatabaseFromArchive()
{
    QString archivePath = QFileDialog::getOpenFileName(
        this,
        tr("Load Database from Archive"),
        QDir::homePath(),
        tr("Database files (*.db);;All files (*.*)"));

    if (archivePath.isEmpty()) {
        return; // User cancelled
    }

    QString currentDbPath = database.databaseName();
    if (currentDbPath.isEmpty()) {
        QMessageBox::critical(this, tr("Load Error"), tr("Could not determine the path of the current database."));
        return;
    }

    // Close the database connection before overwriting the file
    database.close();

    // Remove the old file before copying
    if (QFile::exists(currentDbPath)) {
        if (!QFile::remove(currentDbPath)) {
            QMessageBox::critical(this, tr("Load Error"), tr("Could not remove the old database file. Please check permissions."));
            // Try to reopen the original database
            if (!database.open()) {
                 QMessageBox::critical(this, tr("Fatal Error"), tr("Could not re-open the original database. The application might be in an unstable state."));
            }
            return;
        }
    }

    if (!QFile::copy(archivePath, currentDbPath)) {
        QMessageBox::critical(this, tr("Load Error"), tr("Could not copy the archive file to the application data directory."));
        // In case of copy failure, the original DB is gone. This is a critical state.
        // The application should probably be restarted. For now, we'll just show an error.
        return;
    }

    // Re-open the database. The connection now points to the new file.
    if (!database.open()) {
        QMessageBox::critical(this, tr("Load Error"), tr("Failed to open the new database file: %1").arg(database.lastError().text()));
        return;
    }

    // Refresh all dialogs
    playerDialog->refresh();
    coursesDialog->refresh();
    teamAssemblyDialog->refresh();
    scoreDialog->refresh();
    tournamentLeaderboardDialog->refreshLeaderboards();

    QMessageBox::information(this, tr("Load Successful"), tr("Database successfully loaded from %1.").arg(archivePath));
}
