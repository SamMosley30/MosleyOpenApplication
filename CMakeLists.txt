cmake_minimum_required(VERSION 3.10.0)
project(MosleyOpen VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Test)
qt_standard_project_setup()

set(APP_SOURCES 
    main.cpp
    CommonStructs.h
    MainWindow.h
    MainWindow.cpp
    PlayerDialog.h
    PlayerDialog.cpp
    CoursesDialog.h
    CoursesDialog.cpp
    SpinBoxDelegate.h
    SpinBoxDelegate.cpp
    CheckBoxDelegate.h
    CheckBoxDelegate.cpp
    HolesTransposedModel.h
    HolesTransposedModel.cpp
    ScoreTableModel.h
    ScoreTableModel.cpp
    ScoreEntryDialog.h
    ScoreEntryDialog.cpp
    TournamentLeaderboardModel.h
    TournamentLeaderboardModel.cpp
    TournamentLeaderboardDialog.h
    TournamentLeaderboardDialog.cpp
    TournamentLeaderboardWidget.h
    TournamentLeaderboardWidget.cpp
    DailyLeaderboardModel.h
    DailyLeaderboardModel.cpp
    DailyLeaderboardWidget.h
    DailyLeaderboardWidget.cpp
    TeamLeaderboardModel.h
    TeamLeaderboardModel.cpp
    TeamLeaderboardWidget.h
    TeamLeaderboardWidget.cpp
    TeamAssemblyDialog.h
    TeamAssemblyDialog.cpp
    PlayerListWidget.h
    PlayerListWidget.cpp
)

qt_add_executable(MosleyOpen ${APP_SOURCES})

target_link_libraries(MosleyOpen PRIVATE Qt6::Widgets Qt6::Sql)

include(CTest)
enable_testing()

# ---- Unit Tests ----
# It's good practice to put tests in a subdirectory, e.g., "tests"
# For now, we'll assume test files are at the root or you'll adjust paths.

# Option 1: Create a separate library for your core logic if it grows,
# then link both app and tests against it.
# Option 2: (Simpler for now) Compile necessary app sources directly into the test executable.
# We'll use Option 2 for this example.

set(TEST_MAIN_SOURCES
    # This will be your test runner
    tests/main_test.cpp 
)

set(TEST_CLASS_SOURCES
    # Add your test class .cpp files here
    tests/test_example.cpp
    tests/test_playerdialog.cpp
    # tests/test_tournamentleaderboardmodel.cpp 
)

# List of your application's source files needed by the tests.
# Be selective to only include what the specific units under test and the tests themselves need.
# For model testing, you'll need the model's .cpp and .h, and any direct dependencies like CommonStructs.h
set(APP_SOURCES_FOR_TESTING
    CommonStructs.h # Assuming models use this
    # Add .cpp files of the models you want to test directly
    MainWindow.h
    MainWindow.cpp
    PlayerDialog.h
    PlayerDialog.cpp
    CoursesDialog.h
    CoursesDialog.cpp
    SpinBoxDelegate.h
    SpinBoxDelegate.cpp
    CheckBoxDelegate.h
    CheckBoxDelegate.cpp
    HolesTransposedModel.h
    HolesTransposedModel.cpp
    ScoreTableModel.h
    ScoreTableModel.cpp
    ScoreEntryDialog.h
    ScoreEntryDialog.cpp
    TournamentLeaderboardModel.h
    TournamentLeaderboardModel.cpp
    TournamentLeaderboardDialog.h
    TournamentLeaderboardDialog.cpp
    TournamentLeaderboardWidget.h
    TournamentLeaderboardWidget.cpp
    DailyLeaderboardModel.h
    DailyLeaderboardModel.cpp
    DailyLeaderboardWidget.h
    DailyLeaderboardWidget.cpp
    TeamLeaderboardModel.h
    TeamLeaderboardModel.cpp
    TeamLeaderboardWidget.h
    TeamLeaderboardWidget.cpp
    TeamAssemblyDialog.h
    TeamAssemblyDialog.cpp
    PlayerListWidget.h
    PlayerListWidget.cpp
)

# Add a new executable for your tests
qt_add_executable(MosleyOpenTests
    ${TEST_MAIN_SOURCES}
    ${TEST_CLASS_SOURCES}
    ${APP_SOURCES_FOR_TESTING} 
)

# Link against QtTest and other necessary Qt modules
target_link_libraries(MosleyOpenTests PRIVATE Qt6::Test Qt6::Sql Qt6::Core Qt6::Widgets) # Added Widgets for QApplication in main_test

# Add the test to CTest so 'make test' or 'ctest' can run it
add_test(NAME MosleyOpenUnitTests COMMAND MosleyOpenTests)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
