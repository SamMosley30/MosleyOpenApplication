cmake_minimum_required(VERSION 3.10.0)
project(MosleyOpen VERSION 0.1.0 LANGUAGES C CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)
qt_standard_project_setup()

qt_add_executable(MosleyOpen 
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

target_link_libraries(MosleyOpen PRIVATE Qt6::Widgets Qt6::Sql)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
