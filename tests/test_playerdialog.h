#ifndef TEST_PLAYERDIALOG_H
#define TEST_PLAYERDIALOG_H

#include <QtTest/QtTest>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlTableModel> // For verifications
#include <QTableView>     // To interact with its selection model

// Include the header for the class being tested
// Adjust path as necessary
#include "../PlayerDialog.h" 
#include "../CommonStructs.h" // If PlayerDialog uses it directly or indirectly

class TestPlayerDialog : public QObject
{
    Q_OBJECT

public:
    TestPlayerDialog();
    ~TestPlayerDialog();

private slots:
    // Test lifecycle functions
    void initTestCase();    // Called once before all tests
    void cleanupTestCase(); // Called once after all tests
    void init();            // Called before each test function
    void cleanup();         // Called after each test function

    // Test functions
    void testPlayerDialogConstructionAndModel();
    void testAddPlayer_AddsToModelAndDb();
    void testRemoveSelectedPlayer_RemovesFromModelAndDb();
    void testRemoveSelectedPlayer_NoSelection();
    // void testExportToCsv_CsvStringGeneration(); // If CSV generation is refactored

private:
    QSqlDatabase testDb;
    const QString testDbConnectionName = "playerdialog_test_connection";
    PlayerDialog *dialogInstance; 
    
    // Helper to get the QSqlTableModel from the dialog
    // This requires PlayerDialog to expose its model or its tableView.
    // For now, we assume PlayerDialog.cpp might need slight adjustment to allow this,
    // or we test purely by DB state and observing rowCount() if model is private.
    // If PlayerDialog's 'model' member is private, we can't directly access it without 'friend'.
    // Alternative: PlayerDialog could have a public getter for its model for testing purposes.
    // For now, we'll test primarily by observing DB changes and model->rowCount() via the dialog's table view.

    void setupTestDatabaseSchema();
    int getPlayerCountFromDb(); // Helper to query DB directly
    bool getPlayerDataFromDb(int playerId, QString &name, int &handicap, bool &active);
};

#endif // TEST_PLAYERDIALOG_H
