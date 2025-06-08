#ifndef TEST_EXAMPLE_H
#define TEST_EXAMPLE_H

#include <QtTest/QtTest>
#include <QObject>

// If you were testing a specific class from your app, you'd include its header here.
// For example:
// #include "../CommonStructs.h"

class TestExample : public QObject
{
    Q_OBJECT

public:
    TestExample();
    ~TestExample();

private slots:
    // Test lifecycle functions
    void initTestCase();    // Called once before all test functions in this class.
    void cleanupTestCase(); // Called once after all test functions in this class.
    void init();            // Called before each test function.
    void cleanup();         // Called after each test function.

    // Your actual test functions
    void testAddition();
    void testStringComparison_data(); // For data-driven test
    void testStringComparison();      // For data-driven test
};

#endif // TEST_EXAMPLE_H
