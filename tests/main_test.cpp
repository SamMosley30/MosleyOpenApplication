#include <QtTest>
#include <QApplication> // For QTest::qExec with GUI elements or event loop needs

// Include headers for all your test classes here
#include "test_example.h"
#include "test_playerdialog.h"
// #include "test_tournamentleaderboardmodel.h"
// #include "test_teamleaderboardmodel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv); // QApplication is often needed by QTest,
                                  // especially if testing anything that might
                                  // indirectly use GUI elements or signals/slots
                                  // across threads, or for event-driven tests.
                                  // For pure non-GUI logic, QCoreApplication might suffice.

    int status = 0;
    QStringList args = app.arguments(); // Get command line arguments for QTest

    qDebug() << "Starting MosleyOpen Unit Tests...";

    // --- Instantiate and run your test classes ---

    TestPlayerDialog testPlayerDialogObj;
    status |= QTest::qExec(&testPlayerDialogObj, args);

    // Example for another test class (uncomment when you create it)
    // TestTournamentLeaderboardModel testTournamentModelObj;
    // status |= QTest::qExec(&testTournamentModelObj, args);


    qDebug() << "Finished MosleyOpen Unit Tests. Exit status:" << status;
    return status;
}
