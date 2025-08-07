/**
 * @file main.cpp
 * @brief The main entry point for the application.
 */

#include <QApplication>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include "DatabaseManager.h"
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
    QDir dataDir(dataPath);

    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }

    QString dbPath = dataPath + "/tournament.db";

    if (!QFile::exists(dbPath)) {
        QString templatePath = QCoreApplication::applicationDirPath() + "/tournament.db";
        if (!QFile::copy(templatePath, dbPath)) {
            QMessageBox::critical(nullptr, QObject::tr("Database Error"), QObject::tr("Could not create writable database."));
            return 1;
        }
        QFile::setPermissions(dbPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    }

    if (!DatabaseManager::instance().init(dbPath)) {
        return 1;
    }

    MainWindow w(DatabaseManager::instance().database());
    w.show();
    return app.exec();
}
