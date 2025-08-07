/**
 * @file DatabaseManager.h
 * @brief Contains the declaration of the DatabaseManager class.
 */

#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>

/**
 * @class DatabaseManager
 * @brief Manages the application's database connection.
 *
 * This class is responsible for initializing the database, creating the schema,
 * and providing access to the database connection. It follows the Singleton
 * pattern to ensure that there is only one instance of the database manager.
 */
class DatabaseManager {
public:
    /**
     * @brief Gets the singleton instance of the DatabaseManager.
     * @return A reference to the singleton instance.
     */
    static DatabaseManager& instance();

    /**
     * @brief Initializes the database.
     *
     * This function sets up the database connection, copies the database file
     * if necessary, and creates the database schema.
     * @param dbPath The path to the database file.
     * @return True if the database was initialized successfully, false otherwise.
     */
    bool init(const QString& dbPath);

    /**
     * @brief Gets the database connection.
     * @return A reference to the database connection.
     */
    QSqlDatabase& database();

private:
    /**
     * @brief Private constructor to enforce the Singleton pattern.
     */
    DatabaseManager();

    /**
     * @brief Private destructor.
     */
    ~DatabaseManager();

    // Delete copy constructor and assignment operator
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /**
     * @brief Creates the database schema.
     */
    void createSchema();

    QSqlDatabase m_db; ///< The database connection.
};

#endif // DATABASEMANAGER_H
