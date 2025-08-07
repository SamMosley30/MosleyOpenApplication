/**
 * @file DatabaseManager.cpp
 * @brief Implements the DatabaseManager class.
 */

#include "DatabaseManager.h"
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QStandardPaths>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() {
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::init(const QString& dbPath) {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Database Error"), m_db.lastError().text());
        qDebug() << "DatabaseManager::init - DB Open FAILED:" << m_db.lastError().text();
        return false;
    }

    createSchema();
    return true;
}

QSqlDatabase& DatabaseManager::database() {
    return m_db;
}

void DatabaseManager::createSchema() {
    QSqlQuery q(m_db);
    q.exec(R"(
    CREATE TABLE IF NOT EXISTS players (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL UNIQUE,
        handicap INTEGER NOT NULL DEFAULT 0,
        active INTEGER NOT NULL DEFAULT 1,
        team_id INTEGER DEFAULT NULL
    )
    )");

    QSqlRecord playersRecord = m_db.record("players");
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
}
