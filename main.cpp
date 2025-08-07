/**
 * @file main.cpp
 * @brief The main entry point for the application.
 */

#include <QApplication>
#include <QMessageBox>
#include <QtSql>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include "MainWindow.h"

/**
 * @brief The main function of the application.
 *
 * This function initializes the application, sets up the database connection,
 * creates the main window, and starts the event loop.
 *
 * @param argc The number of command-line arguments.
 * @param argv The command-line arguments.
 * @return The exit code of the application.
 */
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("Sammos");
    QCoreApplication::setApplicationName("MosleyOpen");

    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    qDebug() << "Writable path: " << dataPath;
    QDir dataDir(dataPath);

    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }

    QString dbPath = dataPath + "/tournament.db";

    if (!QFile::exists(dbPath)) {
        QString templatePath = QCoreApplication::applicationDirPath() + "/tournament.db";
        qDebug() << "Template path: " << templatePath;
        if (!QFile::copy(templatePath, dbPath)) {
            QMessageBox::critical(nullptr, QObject::tr("Database Error"), QObject::tr("Could not create writable database."));
            return 1;
        }
        QFile::setPermissions(dbPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    }

    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/sqldrivers");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Database Error"), db.lastError().text());
        qDebug() << "main.cpp - DB Open FAILED:" << db.lastError().text();
        return 1;
    }

    QSqlQuery q(db);
    q.exec(R"(
    CREATE TABLE IF NOT EXISTS players (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL UNIQUE,
        handicap INTEGER NOT NULL DEFAULT 0,
        active INTEGER NOT NULL DEFAULT 1,
        team_id INTEGER DEFAULT NULL 
    )
    )");

    QSqlRecord playersRecord = db.record("players");
    if (playersRecord.indexOf("team_id") == -1) {
        qDebug() << "main.cpp - Adding team_id column to players table.";
        if (!q.exec("ALTER TABLE players ADD COLUMN team_id INTEGER DEFAULT NULL")) {
            qWarning() << "main.cpp - Failed to add team_id column to players table:" << q.lastError().text();
        }
    }

    q.exec(R"(
      CREATE TABLE IF NOT EXISTS courses (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL UNIQUE
      )
    )");
    q.exec(R"(
      CREATE TABLE IF NOT EXISTS holes (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        course_id INTEGER NOT NULL REFERENCES courses(id) ON DELETE CASCADE,
        hole_num INTEGER NOT NULL,
        par INTEGER NOT NULL,
        handicap INTEGER NOT NULL,
        UNIQUE(course_id, hole_num)
      )
    )");

    q.exec(R"(
      CREATE TABLE IF NOT EXISTS teams (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL UNIQUE
      )
    )");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS scores (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            player_id INTEGER NOT NULL,
            course_id INTEGER NOT NULL,
            hole_num INTEGER NOT NULL CHECK (hole_num >= 1 AND hole_num <= 18),
            day_num INTEGER NOT NULL CHECK (day_num >= 1 AND day_num <= 3),
            score INTEGER,
            UNIQUE (player_id, course_id, hole_num, day_num)
        )
      )");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY UNIQUE,
            value TEXT
        )
      )");

    MainWindow w(db);
    w.show();
    return app.exec();
}
