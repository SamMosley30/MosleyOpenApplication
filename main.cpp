#include <QApplication>
#include <QMessageBox>
#include <QtSql>
#include <QDebug> // Add this for qDebug
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("Sammos");
    QCoreApplication::setApplicationName("MosleyOpen");

    // 3. This new block finds/creates the writable database path
    // Get the standard location for application data
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    qDebug() << "Writable path: " << dataPath; // DEBUG"
    QDir dataDir(dataPath);

    // Create the directory if it doesn't exist
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }

    // Define the full path for the writable database
    QString dbPath = dataPath + "/tournament.db";

    // Check if the database already exists in the writable location.
    // If not, copy it from the application's installation directory (the template).
    if (!QFile::exists(dbPath)) {
        QString templatePath = QCoreApplication::applicationDirPath() + "/tournament.db";
        if (!QFile::copy(templatePath, dbPath)) {
            QMessageBox::critical(nullptr, QObject::tr("Database Error"), QObject::tr("Could not create writable database."));
            return 1;
        }
        // Set permissions to ensure the file is writable
        QFile::setPermissions(dbPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    }

    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/sqldrivers");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE"); // This becomes the default connection
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Database Error"), db.lastError().text());
        qDebug() << "main.cpp - DB Open FAILED:" << db.lastError().text(); // DEBUG
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

    // Course tables
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

    // Teams table
    q.exec(R"(
      CREATE TABLE IF NOT EXISTS teams (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL UNIQUE
      )
    )");


    // Score Table
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

    // Settings Table
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY UNIQUE,
            value TEXT
        )
      )");

    MainWindow w(db); // Pass the QSqlDatabase object itself
    w.show();
    return app.exec();
}
