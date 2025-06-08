#include "test_example.h"
#include <QDebug>

TestExample::TestExample() {
    qDebug() << "TestExample object created.";
}

TestExample::~TestExample() {
    qDebug() << "TestExample object destroyed.";
}

void TestExample::initTestCase() {
    qDebug() << "TestExample: initTestCase() - Called once before all tests in TestExample.";
    // Example: Set up a database connection if all tests in this class use the same one.
    // However, for model testing, you might do DB setup in the main_test.cpp or a global fixture.
    // For this example, we'll keep it simple.
}

void TestExample::cleanupTestCase() {
    qDebug() << "TestExample: cleanupTestCase() - Called once after all tests in TestExample.";
    // Example: Close database connection, clean up global resources.
}

void TestExample::init() {
    qDebug() << "TestExample: init() - Called before a test function.";
    // Example: Create fresh objects, reset state for the upcoming test.
}

void TestExample::cleanup() {
    qDebug() << "TestExample: cleanup() - Called after a test function.";
    // Example: Delete objects created in init().
}

void TestExample::testAddition() {
    qDebug() << "TestExample: Running testAddition()...";
    int sum = 2 + 2;
    QCOMPARE(sum, 4); // Verify that sum is 4

    QVERIFY2(sum == 4, "2 + 2 should equal 4"); // Another way to verify with a message

    // Example of a failing test (uncomment to see it fail)
    // QCOMPARE(sum, 5); 
}

void TestExample::testStringComparison_data() {
    QTest::addColumn<QString>("string1");
    QTest::addColumn<QString>("string2");
    QTest::addColumn<bool>("areEqual");

    QTest::newRow("identical") << "hello" << "hello" << true;
    QTest::newRow("different") << "hello" << "world" << false;
    QTest::newRow("case_diff") << "Hello" << "hello" << false; // QCOMPARE is case-sensitive for strings
    QTest::newRow("empty")     << ""      << ""      << true;
}

void TestExample::testStringComparison() {
    QFETCH(QString, string1);
    QFETCH(QString, string2);
    QFETCH(bool, areEqual);

    qDebug() << "TestExample: Running testStringComparison() with data:" << string1 << string2 << areEqual;
    if (areEqual) {
        QCOMPARE(string1, string2);
    } else {
        QVERIFY(string1 != string2);
    }
}
