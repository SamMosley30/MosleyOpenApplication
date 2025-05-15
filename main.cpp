#include <QApplication>
#include <QMessageBox>
#include <QtSql>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/sqldrivers");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("tournament.db");
    if (!db.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Database Error"), db.lastError().text());
        return 1;
    }

    QSqlQuery q(db);
    q.exec(R"(
    CREATE TABLE IF NOT EXISTS players (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL UNIQUE,
        handicap INTEGER NOT NULL DEFAULT 0,
        active INTEGER NOT NULL DEFAULT 1
    )
    )");

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

    // Score Table
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
