#include "test_playerdialog.h"
#include <QApplication>    
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QPushButton>     
#include <QItemSelectionModel>
#include <QTableView> 

TestPlayerDialog::TestPlayerDialog() : dialogInstance(nullptr) {
    qDebug() << "TestPlayerDialog object created.";
}

TestPlayerDialog::~TestPlayerDialog() {
    qDebug() << "TestPlayerDialog object destroyed.";
}

void TestPlayerDialog::initTestCase() {
    qDebug() << "TestPlayerDialog: initTestCase()";
    if (QSqlDatabase::contains(testDbConnectionName)) {
        qDebug() << "TestPlayerDialog: initTestCase - Removing pre-existing connection:" << testDbConnectionName;
        QSqlDatabase::removeDatabase(testDbConnectionName); 
    }
    testDb = QSqlDatabase::addDatabase("QSQLITE", testDbConnectionName);
    testDb.setDatabaseName(":memory:");
    if (!testDb.open()) {
        QFAIL(qPrintable(QString("TestPlayerDialog: Cannot open in-memory database. Error: %1").arg(testDb.lastError().text())));
    }
    qDebug() << "TestPlayerDialog: initTestCase - In-memory DB opened successfully on connection:" << testDb.connectionName();
    setupTestDatabaseSchema();
}

void TestPlayerDialog::cleanupTestCase() {
    qDebug() << "TestPlayerDialog: cleanupTestCase()";
    if (testDb.isOpen()) {
        qDebug() << "TestPlayerDialog: cleanupTestCase - Closing DB connection:" << testDb.connectionName();
        testDb.close();
    }
    if (QSqlDatabase::contains(testDbConnectionName)) {
         qDebug() << "TestPlayerDialog: cleanupTestCase - Connection" << testDbConnectionName << "is still valid before removal attempt.";
         QSqlDatabase::removeDatabase(testDbConnectionName);
         qDebug() << "TestPlayerDialog: cleanupTestCase - Attempted to remove connection" << testDbConnectionName;
         if (QSqlDatabase::contains(testDbConnectionName)) {
            // This warning might still appear if objects are held by Qt's event loop briefly
            qWarning() << "TestPlayerDialog: cleanupTestCase - Connection" << testDbConnectionName << "STILL EXISTS after removal attempt. This might be a Qt Test or object lifetime subtlety.";
         }
    } else {
        qDebug() << "TestPlayerDialog: cleanupTestCase - Connection" << testDbConnectionName << "was already removed or never existed for removal.";
    }
}

void TestPlayerDialog::init() {
    qDebug() << "TestPlayerDialog: init() - Test function starting.";
    if (!testDb.isOpen()) {
        qDebug() << "TestPlayerDialog: init() - DB was closed, reopening...";
        if (!testDb.open()) {
            QFAIL("TestPlayerDialog: init() - Failed to reopen DB.");
        }
    }

    QSqlQuery clearQuery(testDb);
    if (!clearQuery.exec("DELETE FROM players;")) {
        qWarning() << "TestPlayerDialog: Failed to clear players table in init():" << clearQuery.lastError().text();
    } else {
        qDebug() << "TestPlayerDialog: init() - Cleared players table.";
    }
    
    dialogInstance = new PlayerDialog(testDb); 

    QTableView *tv = dialogInstance->findChild<QTableView*>();
    QVERIFY2(tv, "TableView child not found in PlayerDialog in init().");
    if (tv && tv->model()) {
        QSqlTableModel* tableModel = qobject_cast<QSqlTableModel*>(tv->model());
        QVERIFY2(tableModel, "TableModel not found or not QSqlTableModel in init().");
        if (tableModel) {
            bool selected = tableModel->select(); 
            qDebug() << "TestPlayerDialog: init() - Dialog's model select() called. Success:" << selected << "Row count after select:" << tableModel->rowCount();
        }
    } else {
        qWarning() << "TestPlayerDialog: init() - Dialog's TableView or Model is null.";
    }
}

void TestPlayerDialog::cleanup() {
    qDebug() << "TestPlayerDialog: cleanup() - Test function finished.";
    if (dialogInstance) {
        QTableView *tv = dialogInstance->findChild<QTableView*>();
        if (tv) {
            QSqlTableModel* tableModel = qobject_cast<QSqlTableModel*>(tv->model());
            if (tableModel) {
                // tableModel->clear(); // This might be too aggressive and cause issues if model is shared or used elsewhere.
                                     // Let's rely on deleting the dialogInstance to clean up its model.
                qDebug() << "TestPlayerDialog: cleanup() - Model pointer before dialog delete:" << tableModel;
            }
            // tv->setModel(nullptr); // Detach model from view
        }
        delete dialogInstance;
        dialogInstance = nullptr;
        qDebug() << "TestPlayerDialog: cleanup() - dialogInstance deleted.";
    }
}

void TestPlayerDialog::setupTestDatabaseSchema() {
    QSqlQuery query(testDb);
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS players ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL UNIQUE, "
        "handicap INTEGER NOT NULL DEFAULT 0, "
        "active INTEGER NOT NULL DEFAULT 1, "
        "team_id INTEGER DEFAULT NULL"
        ")");
    if (!success) {
        qWarning() << "TestPlayerDialog: Failed to create players table:" << query.lastError().text();
        QFAIL("Database schema setup for players table failed.");
    } else {
        qDebug() << "TestPlayerDialog: setupTestDatabaseSchema - Players table created/verified.";
    }
}

int TestPlayerDialog::getPlayerCountFromDb() {
    QSqlQuery query(testDb); // Use the member testDb
    if (!query.exec("SELECT COUNT(*) FROM players")) {
         qWarning() << "getPlayerCountFromDb: SQL query failed:" << query.lastError().text() << "DB is open:" << testDb.isOpen();
         return -1;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    qWarning() << "getPlayerCountFromDb: Failed to count players (query.next() was false). DB is open:" << testDb.isOpen();
    return -1; 
}

bool TestPlayerDialog::getPlayerDataFromDb(int playerId, QString &name, int &handicap, bool &active) {
    QSqlQuery query(testDb); // Use the member testDb
    query.prepare("SELECT name, handicap, active FROM players WHERE id = :id");
    query.bindValue(":id", playerId);
    if (!query.exec()) {
        qWarning() << "getPlayerDataFromDb: SQL query failed for player ID" << playerId << ":" << query.lastError().text() << "DB is open:" << testDb.isOpen();
        return false;
    }
    if (query.next()) {
        name = query.value("name").toString();
        handicap = query.value("handicap").toInt();
        active = query.value("active").toBool();
        return true;
    }
    // It's not necessarily a warning if the player isn't found, it just means they don't exist.
    // qDebug() << "getPlayerDataFromDb: No data found for player ID" << playerId << ". DB is open:" << testDb.isOpen();
    return false;
}

void TestPlayerDialog::testPlayerDialogConstructionAndModel() {
    qDebug() << "TestPlayerDialog: Running testPlayerDialogConstructionAndModel()...";
    QVERIFY2(dialogInstance != nullptr, "PlayerDialog instance should be created.");
    
    QTableView *tv = dialogInstance->findChild<QTableView*>();
    QVERIFY2(tv != nullptr, "PlayerDialog should contain a QTableView.");
    QVERIFY2(tv->model() != nullptr, "TableView in PlayerDialog should have a model set.");
    
    QSqlTableModel* tableModel = qobject_cast<QSqlTableModel*>(tv->model());
    QVERIFY2(tableModel != nullptr, "Model in TableView should be a QSqlTableModel.");
    QCOMPARE(tableModel->tableName(), QString("players"));
    qDebug() << "TestPlayerDialog: testPlayerDialogConstructionAndModel - Model table name:" << tableModel->tableName();
}

void TestPlayerDialog::testAddPlayer_AddsToModelAndDb() {
    qDebug() << "TestPlayerDialog: Running testAddPlayer_AddsToModelAndDb()...";
    
    int initialDbCount = getPlayerCountFromDb();
    QCOMPARE(initialDbCount, 0); 

    qDebug() << "TestPlayerDialog: testAddPlayer - Calling dialogInstance->addPlayer()";
    dialogInstance->addPlayer(); 
    // PlayerDialog::addPlayer() now contains a qDebug for submitAll success/failure.

    // Force the model in the dialog to re-fetch from the database
    QTableView *tv = dialogInstance->findChild<QTableView*>();
    QVERIFY2(tv, "TableView not found in testAddPlayer_AddsToModelAndDb");
    QSqlTableModel* tableModel = qobject_cast<QSqlTableModel*>(tv->model());
    QVERIFY2(tableModel, "TableModel not found in testAddPlayer_AddsToModelAndDb");
    
    bool selectSuccess = tableModel->select(); // Re-fetch data from DB into the model
    qDebug() << "TestPlayerDialog: testAddPlayer - Model select() success:" << selectSuccess << "Model row count after select:" << tableModel->rowCount();

    int finalDbCount = getPlayerCountFromDb();
    qDebug() << "TestPlayerDialog: testAddPlayer - Initial DB count:" << initialDbCount << "Final DB count:" << finalDbCount;
    QCOMPARE(finalDbCount, initialDbCount + 1); 

    QSqlQuery query(testDb);
    QVERIFY2(query.exec("SELECT id, name, handicap, active FROM players WHERE name = 'New Player'"), qPrintable(QString("Query for 'New Player' failed: %1").arg(query.lastError().text())));
    QVERIFY2(query.next(), "New player 'New Player' should be found in DB after addPlayer.");

    QString name = query.value("name").toString();
    int handicap = query.value("handicap").toInt();
    bool active = query.value("active").toBool();

    QCOMPARE(name, QString("New Player"));
    QCOMPARE(handicap, 0);
    QCOMPARE(active, false); 
    qDebug() << "TestPlayerDialog: testAddPlayer_AddsToModelAndDb - Test finished.";
}

void TestPlayerDialog::testRemoveSelectedPlayer_RemovesFromModelAndDb() {
    qDebug() << "TestPlayerDialog: Running testRemoveSelectedPlayer_RemovesFromModelAndDb()...";

    // ARRANGE: Insert a player with a unique name for reliable fetching
    const QString playerNameToRemove = "PlayerToRemove_UniqueName";
    QSqlQuery insertQuery(testDb);
    insertQuery.prepare("INSERT INTO players (name, handicap, active) VALUES (:name, 10, 1)");
    insertQuery.bindValue(":name", playerNameToRemove);
    QVERIFY2(insertQuery.exec(), qPrintable(QString("Insert failed in RemoveTest: %1").arg(insertQuery.lastError().text())));
    
    int initialDbCount = getPlayerCountFromDb();
    QCOMPARE(initialDbCount, 1); 

    QTableView *tv = dialogInstance->findChild<QTableView*>();
    QVERIFY(tv);
    QSqlTableModel* tableModel = qobject_cast<QSqlTableModel*>(tv->model());
    QVERIFY(tableModel);
    QVERIFY2(tableModel->select(), "Model select() failed in RemoveTest setup."); 

    // Find the player by the unique name to get their ID and row index
    int playerIdToRemove = -1;
    int rowIndexToRemove = -1;
    for (int i = 0; i < tableModel->rowCount(); ++i) {
        if (tableModel->record(i).value("name").toString() == playerNameToRemove) {
            playerIdToRemove = tableModel->record(i).value("id").toInt();
            rowIndexToRemove = i;
            break;
        }
    }
    QVERIFY2(playerIdToRemove != -1, qPrintable(QString("Player '%1' not found in model for removal. Model row count: %2").arg(playerNameToRemove).arg(tableModel->rowCount())));
    qDebug() << "TestPlayerDialog: testRemoveSelectedPlayer - Found player" << playerNameToRemove << "at row" << rowIndexToRemove << "with ID" << playerIdToRemove;

    tv->selectionModel()->clearSelection();
    tv->selectionModel()->select(tableModel->index(rowIndexToRemove, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    QVERIFY2(tv->selectionModel()->hasSelection(), "Row should be selected in table view.");
    QCOMPARE(tv->selectionModel()->selectedRows().count(), 1);

    qDebug() << "TestPlayerDialog: testRemoveSelectedPlayer - Calling dialogInstance->removeSelected()";
    dialogInstance->removeSelected();
    // PlayerDialog::removeSelected() now has qDebug for submitAll success/failure

    // Force model sync after operation
    QVERIFY2(tableModel->select(), "Model select() failed after remove operation.");
    qDebug() << "TestPlayerDialog: testRemoveSelectedPlayer - Model row count after remove & select:" << tableModel->rowCount();


    int finalDbCount = getPlayerCountFromDb();
    qDebug() << "TestPlayerDialog: testRemoveSelectedPlayer - Initial DB count:" << initialDbCount << "Final DB count:" << finalDbCount;
    QCOMPARE(finalDbCount, 0); 

    QString name_check; int handicap_check; bool active_status_check; 
    QVERIFY2(!getPlayerDataFromDb(playerIdToRemove, name_check, handicap_check, active_status_check), 
             qPrintable(QString("Player ID %1 should have been deleted from DB.").arg(playerIdToRemove)));
    qDebug() << "TestPlayerDialog: testRemoveSelectedPlayer_RemovesFromModelAndDb - Test finished.";
}

void TestPlayerDialog::testRemoveSelectedPlayer_NoSelection() {
    qDebug() << "TestPlayerDialog: Running testRemoveSelectedPlayer_NoSelection()...";
    
    QSqlQuery insertQuery(testDb); 
    QVERIFY2(insertQuery.exec("INSERT INTO players (name, handicap, active) VALUES ('Test No Remove', 5, 1)"), 
            qPrintable(QString("INSERT failed in NoSelection test: %1").arg(insertQuery.lastError().text())));
    
    int initialDbCount = getPlayerCountFromDb();
    QCOMPARE(initialDbCount, 1);

    QTableView *tv = dialogInstance->findChild<QTableView*>();
    QVERIFY(tv);
    QSqlTableModel* tableModel = qobject_cast<QSqlTableModel*>(tv->model());
    QVERIFY(tableModel);
    QVERIFY2(tableModel->select(), "Model select() failed in NoSelection test setup."); 
    tv->selectionModel()->clearSelection(); 
    QVERIFY2(!tv->selectionModel()->hasSelection(), "No row should be selected.");

    qDebug() << "TestPlayerDialog: testRemoveSelectedPlayer_NoSelection - Calling dialogInstance->removeSelected()";
    dialogInstance->removeSelected();

    int finalDbCount = getPlayerCountFromDb();
    qDebug() << "TestPlayerDialog: testRemoveSelectedPlayer_NoSelection - Initial DB count:" << initialDbCount << "Final DB count:" << finalDbCount;
    QCOMPARE(finalDbCount, initialDbCount); 
    qDebug() << "TestPlayerDialog: testRemoveSelectedPlayer_NoSelection - Test finished.";
}
